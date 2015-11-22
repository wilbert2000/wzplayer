/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "core.h"
#include <QDir>
#include <QFileInfo>
#include <QRegExp>
#include <QTextStream>
#include <QUrl>
#include <QNetworkProxy>
#include <QTimer>

#ifdef Q_OS_OS2
#include <QEventLoop>
#endif

#include <cmath>

#include "config.h"
#include "desktop.h"
#include "discname.h"
#include "mediadata.h"
#include "extensions.h"
#include "colorutils.h"
#include "helper.h"
#include "settings/paths.h"
#include "settings/mediasettings.h"
#include "settings/preferences.h"
#include "settings/filesettings.h"
#include "settings/filesettingshash.h"
#include "settings/tvsettings.h"
#include "settings/filters.h"

#include "proc/playerprocess.h"
#include "playerwindow.h"
#include "gui/tvlist.h"

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef Q_OS_WIN
#include <windows.h> // To change app priority
#include <QSysInfo> // To get Windows version
#endif
#ifdef SCREENSAVER_OFF
#include "screensaver.h"
#endif
#endif

#ifdef YOUTUBE_SUPPORT
#include "retrieveyoutubeurl.h"
  #ifdef YT_USE_YTSIG
  #include "ytsig.h"
  #endif
#endif


using namespace Settings;

TCore::TCore(QWidget* parent, TPlayerWindow *mpw)
	: QObject(parent),
	  mdat(),
	  mset(&mdat),
	  playerwindow(mpw),
	  _state(Stopped),
	  we_are_restarting(false),
	  title(-1),
	  block_dvd_nav(false),
	  change_volume_after_unpause(false),
	  pos_max(1000)
{
	qRegisterMetaType<TCore::State>("TCore::State");

	proc = Proc::TPlayerProcess::createPlayerProcess(this, &mdat);

	connect(proc, SIGNAL(error(QProcess::ProcessError)),
			 this, SLOT(processError(QProcess::ProcessError)));

	connect(proc, SIGNAL(processExited(bool)),
			 this, SLOT(processFinished(bool)));

	connect(proc, SIGNAL(playerFullyLoaded()),
			 this, SLOT(playingStarted()));

	connect(proc, SIGNAL(receivedCurrentSec(double)),
			 this, SLOT(gotCurrentSec(double)));

	connect(proc, SIGNAL(receivedCurrentFrame(int)),
			 this, SIGNAL(showFrame(int)));

	connect(proc, SIGNAL(receivedPause()),
			 this, SLOT(gotPause()));

	connect(proc, SIGNAL(receivedBuffering()),
			 this, SIGNAL(buffering()));

	connect(proc, SIGNAL(receivedBufferingEnded()),
			 this, SLOT(displayBufferingEnded()));

	connect(proc, SIGNAL(receivedMessage(QString)),
			 this, SLOT(displayMessage(QString)));

	connect(proc, SIGNAL(receivedCacheEmptyMessage(QString)),
			 this, SIGNAL(buffering()));

	connect(proc, SIGNAL(receivedCreatingIndex(QString)),
			 this, SLOT(displayMessage(QString)));

	connect(proc, SIGNAL(receivedCreatingIndex(QString)),
			 this, SIGNAL(buffering()));

	connect(proc, SIGNAL(receivedConnectingToMessage(QString)),
			 this, SLOT(displayMessage(QString)));

	connect(proc, SIGNAL(receivedConnectingToMessage(QString)),
			 this, SIGNAL(buffering()));

	connect(proc, SIGNAL(receivedResolvingMessage(QString)),
			 this, SLOT(displayMessage(QString)));

	connect(proc, SIGNAL(receivedResolvingMessage(QString)),
			 this, SIGNAL(buffering()));

	connect(proc, SIGNAL(receivedScreenshot(QString)),
			 this, SLOT(displayScreenshotName(QString)));

	connect(proc, SIGNAL(receivedUpdatingFontCache()),
			 this, SLOT(displayUpdatingFontCache()));

	connect(proc, SIGNAL(receivedVideoOutResolution(int,int)),
			 this, SLOT(gotVideoOutResolution(int,int)));

	connect(proc, SIGNAL(receivedVO(QString)),
			 this, SLOT(gotVO(QString)));

	connect(proc, SIGNAL(receivedAO(QString)),
			 this, SLOT(gotAO(QString)));

	connect(proc, SIGNAL(receivedEndOfFile()),
			 this, SLOT(fileReachedEnd()), Qt::QueuedConnection);

	connect(proc, SIGNAL(receivedStreamTitle(QString)),
			 this, SLOT(streamTitleChanged(QString)));

	connect(proc, SIGNAL(receivedStreamTitleAndUrl(QString,QString)),
			 this, SLOT(streamTitleAndUrlChanged(QString,QString)));

	connect(this, SIGNAL(mediaLoaded()), this, SLOT(checkIfVideoIsHD()), Qt::QueuedConnection);

	connect(proc, SIGNAL(receivedVideoTrackInfo()),
			 this, SIGNAL(videoTrackInfoChanged()));
	connect(proc, SIGNAL(receivedVideoTrackChanged(int)),
			 this, SIGNAL(videoTrackChanged(int)));

	connect(proc, SIGNAL(receivedAudioTrackInfo()),
			 this, SLOT(gotAudioTrackInfo()));
	connect(proc, SIGNAL(receivedAudioTrackChanged(int)),
			 this, SLOT(gotAudioTrackChanged(int)));

	connect(proc, SIGNAL(receivedSubtitleTrackInfo()),
			 this, SLOT(gotSubtitleInfo()));
	connect(proc, SIGNAL(receivedSubtitleTrackChanged()),
			 this, SLOT(gotSubtitleChanged()));

	connect(proc, SIGNAL(receivedTitleTrackInfo()),
			 this, SIGNAL(titleTrackInfoChanged()));
	connect(proc, SIGNAL(receivedTitleTrackChanged(int)),
			 this, SIGNAL(titleTrackChanged(int)));

	connect(proc, SIGNAL(receivedChapterInfo()),
			 this, SIGNAL(chapterInfoChanged()));

	connect(proc, SIGNAL(durationChanged(double)),
			 this, SIGNAL(durationChanged(double)));

	connect(proc, SIGNAL(receivedForbiddenText()), this, SIGNAL(receivedForbidden()));

	connect(this, SIGNAL(stateChanged(TCore::State)),
			 this, SLOT(watchState(TCore::State)));

	connect(this, SIGNAL(mediaInfoChanged()), this, SLOT(sendMediaInfo()));

	// TPlayerWindow
	connect(this, SIGNAL(aboutToStartPlaying()),
			 playerwindow, SLOT(aboutToStartPlaying()));
	connect(playerwindow, SIGNAL(mouseMoved(QPoint)),
			 this, SLOT(dvdnavUpdateMousePos(QPoint)));

	playerwindow->videoLayer()->setRepaintBackground(pref->repaint_video_background);
	playerwindow->setMonitorAspect(pref->monitor_aspect_double());

#if  defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef SCREENSAVER_OFF
	// Windows or OS2 screensaver
	win_screensaver = new WinScreenSaver();
	connect(this, SIGNAL(aboutToStartPlaying()),
			 this, SLOT(disableScreensaver()));
	connect(proc, SIGNAL(processExited(bool)),
			 this, SLOT(enableScreensaver()), Qt::QueuedConnection);
	connect(proc, SIGNAL(error(QProcess::ProcessError)),
			 this, SLOT(enableScreensaver()), Qt::QueuedConnection);
#endif
#endif

#ifdef YOUTUBE_SUPPORT
	yt = new RetrieveYoutubeUrl(this);
	yt->setUseHttpsMain(pref->yt_use_https_main);
	yt->setUseHttpsVi(pref->yt_use_https_vi);

	#ifdef YT_USE_SIG
	QSettings* sigset = new QSettings(TPaths::configPath() + "/sig.ini", QSettings::IniFormat, this);
	yt->setSettings(sigset);
	#endif

	connect(yt, SIGNAL(gotPreferredUrl(const QString &, int)), this, SLOT(openYT(const QString &)));
	connect(yt, SIGNAL(connecting(QString)), this, SLOT(connectingToYT(QString)));
	connect(yt, SIGNAL(errorOcurred(int,QString)), this, SLOT(YTFailed(int,QString)));
	connect(yt, SIGNAL(noSslSupport()), this, SIGNAL(noSslSupport()));
	connect(yt, SIGNAL(signatureNotFound(const QString&)), this, SIGNAL(signatureNotFound(const QString&)));
	connect(yt, SIGNAL(gotEmptyList()), this, SLOT(YTNoVideoUrl()));
#endif

	connect(this, SIGNAL(buffering()), this, SLOT(displayBuffering()));
}

TCore::~TCore() {

	stopPlayer();
	proc->terminate();

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef SCREENSAVER_OFF
	delete win_screensaver;
#endif
#endif
}

void TCore::processError(QProcess::ProcessError error) {
	qDebug("TCore::processError: %d", error);

	// First restore normal window background
	playerwindow->playingStopped();

	emit playerFailed(error);
}

void TCore::processFinished(bool normal_exit) {
	qDebug("TCore::processFinished");

	// Restore normal window background
	playerwindow->playingStopped(!we_are_restarting);

	if (we_are_restarting) {
		qDebug("TCore::processFinished: something tells me we are restarting...");
		return;
	}

	qDebug("TCore::processFinished: entering the stopped state");
	setState(Stopped);

	if (!normal_exit) {
		int exit_code = proc->exitCodeOverride();
		qWarning("TCore::processFinished: player error (%d)", exit_code);
		playerwindow->showLogo();
		emit playerFinishedWithError(exit_code);
	}
}

void TCore::fileReachedEnd() {

	// Reset current time to 0
	gotCurrentSec(0);

	qDebug("TCore::fileReachedEnd: emit mediaEOF()");
	emit mediaEOF();
}

void TCore::setState(State s) {

	if (s != _state) {
		_state = s;
		qDebug() << "TCore::setState: set state to" << stateToString()
				 << "at" << mset.current_sec;
		qDebug() << "TCore::setState: emit stateChanged()";
		emit stateChanged(_state);
	}
}

QString TCore::stateToString() {
	if (state()==Playing) return "Playing";
	else
	if (state()==Stopped) return "Stopped";
	else
	if (state()==Paused) return "Paused";
	else
	return "Unknown";
}

// Public restart
void TCore::restart() {
	qDebug("TCore::restart");

	if (proc->isRunning()) {
		restartPlay();
	} else {
		qDebug("TCore::restart: mplayer is not running");
	}
}

void TCore::reload() {
	qDebug("TCore::reload");

	stopPlayer();
	we_are_restarting = false;

	initPlaying();
}

bool TCore::isMPlayer() {
	return proc->isMPlayer();
}

bool TCore::isMPV() {
	return proc->isMPV();
}

void TCore::saveMediaInfo() {

	if (pref->dont_remember_media_settings) {
		qDebug("TCore::saveMediaInfo: saving disabled by user");
		return;
	}
	if (mdat.filename.isEmpty()) {
		qDebug("TCore::saveMediaInfo: nothing to save");
		return;
	}
	qDebug() << "TCore::saveMediaInfo: saving settings for" << mdat.filename;
	emit showMessage(tr("Saving settings for %1").arg(mdat.filename), 0);

	if (mdat.selected_type == TMediaData::TYPE_FILE) {
		if (pref->file_settings_method.toLower() == "hash") {
			Settings::TFileSettingsHash settings(mdat.filename);
			settings.saveSettingsFor(mdat.filename, mset, proc->player());
		} else {
			Settings::TFileSettings settings;
			settings.saveSettingsFor(mdat.filename, mset, proc->player());
		}
	} else if (mdat.selected_type == TMediaData::TYPE_TV) {
		Settings::TTVSettings settings;
		settings.saveSettingsFor(mdat.filename, mset, proc->player());
	}

	emit showMessage(tr("Saved settings for %1").arg(mdat.filename), 3000);
} // saveMediaInfo

void TCore::changeFullscreenMode(bool b) {
	proc->setFullscreen(b);
}

void TCore::clearOSD() {
	displayTextOnOSD("", 0, pref->osd_level);
}

void TCore::displayTextOnOSD(QString text, int duration, int level) {
	//qDebug("TCore::displayTextOnOSD: '%s'", text.toUtf8().constData());

	if (proc->isFullyStarted()
		&& level <= pref->osd_level
		&& !mdat.noVideo()) {
		proc->showOSDText(text, duration, level);
	}
}

void TCore::setOSDPos(const QPoint &pos) {
	// qDebug("TCore::setOSDPos");

	if (proc->isFullyStarted()) {
		proc->setOSDPos(pos, pref->osd_level);
	}
}

void TCore::close() {
	qDebug("TCore::close()");

	stopPlayer();
	we_are_restarting = false;
	// Save data previous file:
	saveMediaInfo();
	// Clear media data
	mdat = TMediaData();
}

void TCore::openDisc(TDiscData &disc, bool fast_open) {

	// Change title if already playing
	if (fast_open && _state != Stopped && disc.title > 0
		&& !mset.playing_single_track) {
		bool current_url_valid;
		TDiscData current_disc = TDiscName::split(mdat.filename, &current_url_valid);
		if (current_url_valid && current_disc.device == disc.device) {
			// If it fails, it will call again with fast_open set to false
			qDebug("TCore::openDisc: trying changeTitle(%d)", disc.title);
			changeTitle(disc.title);
			return;
		}
	}

	// Finish current
	close();

	// Add device from prev if none specified
	if (disc.device.isEmpty()) {
		if (disc.protocol == "vcd" || disc.protocol == "cdda") {
			disc.device = pref->cdrom_device;
		} else if (disc.protocol == "br") {
			disc.device = pref->bluray_device;
		} else {
			disc.device = pref->dvd_device;
		}
	}

	// Test access
	if (!QFileInfo(disc.device).exists()) {
		qWarning() << "TCore::openDisc: could not access" << disc.device;
		// Forgot a "/"?
		if (QFileInfo("/" + disc.device).exists()) {
			qWarning() << "TCore::openDisc: added missing /";
			disc.device = "/" + disc.device;
		} else {
			emit showMessage(tr("File not found: %1").arg(disc.device), 10000);
			return;
		}
	}

	// Set filename and selected type
	mdat.filename = TDiscName::join(disc);
	mdat.selected_type = (TMediaData::Type) TDiscName::protocolToDisc(disc.protocol);

	// Clear settings
	mset.reset();
	if (disc.title > 0 && TMediaData::isCD(mdat.selected_type)) {
		mset.playing_single_track = true;
	}

	initPlaying();
	return;
} // openDisc

// Generic open, autodetect type
void TCore::open(QString file, int seek, bool fast_open) {
	qDebug("TCore::open: '%s'", file.toUtf8().data());

	if (file.startsWith("file:")) {
		file = QUrl(file).toLocalFile();
	}
	emit showMessage(tr("Opening %1").arg(file), 0);

	bool disc_url_valid;
	TDiscData disc = TDiscName::split(file, &disc_url_valid);
	if (disc_url_valid) {
		qDebug() << "TCore::openDisc: * identified as" << disc.protocol;
		openDisc(disc, fast_open);
		return;
	}

	QFileInfo fi(file);
	if (fi.exists()) {
		file = fi.absoluteFilePath();

		TExtensions e;
		QRegExp ext_sub(e.subtitles().forRegExp(), Qt::CaseInsensitive);
		if (ext_sub.indexIn(fi.suffix()) >= 0) {
			qDebug("TCore::open: * identified as subtitle file");
			loadSub(file);
			return;
		}

		if (fi.isDir()) {
			qDebug("TCore::open: * identified as a directory");
			qDebug("TCore::open:   checking if it contains a dvd");
			if (Helper::directoryContainsDVD(file)) {
				qDebug("TCore::open: * directory contains a dvd");
				open(TDiscName::joinDVD(file, pref->use_dvdnav), fast_open);
			} else {
				qDebug("TCore::open: * directory doesn't contain a dvd");
				qDebug("TCore::open:   opening nothing");
			}
			return;
		}

		// Local file
		qDebug("TCore::open: * identified as local file");
		openFile(file, seek);
		return;
	}

	// File does not exist
	if (file.toLower().startsWith("tv:") || file.toLower().startsWith("dvb:")) {
		qDebug("TCore::open: * identified as TV");
		openTV(file);
	} else {
		qDebug("TCore::open: * not identified, playing as stream");
		openStream(file);
	}
}

#ifdef YOUTUBE_SUPPORT
void TCore::openYT(const QString & url) {
	qDebug("TCore::openYT: %s", url.toUtf8().constData());
	openStream(url);
	yt->close();
}

void TCore::connectingToYT(QString host) {
	emit showMessage(tr("Connecting to %1").arg(host));
}

void TCore::YTFailed(int /*error_number*/, QString /*error_str*/) {
	emit showMessage(tr("Unable to retrieve the Youtube page"));
}

void TCore::YTNoVideoUrl() {
	emit showMessage(tr("Unable to locate the URL of the video"));
}
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef SCREENSAVER_OFF
void TCore::enableScreensaver() {
	qDebug("TCore::enableScreensaver");
	if (pref->turn_screensaver_off) {
		win_screensaver->enable();
	}
}

void TCore::disableScreensaver() {
	qDebug("TCore::disableScreensaver");
	if (pref->turn_screensaver_off) {
		win_screensaver->disable();
	}
}
#endif
#endif

void TCore::setExternalSubs(const QString &filename) {

	mset.current_sub_set_by_user = true;
	mset.current_sub_idx = TMediaSettings::NoneSelected;
	mset.sub.setID(TMediaSettings::NoneSelected);
	mset.sub.setType(SubData::File);
	// For mplayer assume vob if file extension idx
	if (isMPlayer()) {
		QFileInfo fi(filename);
		if (fi.suffix().toLower() == "idx") {
			mset.sub.setType(SubData::Vob);
		}
	}
	mset.sub.setFilename(filename);
}

void TCore::loadSub(const QString & sub) {
	qDebug("TCore::loadSub");

	if (!sub.isEmpty() && QFile::exists(sub)) {
		setExternalSubs(sub);
		if (!pref->fast_load_sub
			|| mset.external_subtitles_fps != TMediaSettings::SFPS_None) {
			restartPlay();
		} else {
			proc->setExternalSubtitleFile(sub);
		}
	} else {
		qWarning("TCore::loadSub: file '%s' is not valid", sub.toUtf8().constData());
	}
}

bool TCore::haveExternalSubs() {
	return mdat.subs.hasFileSubs()
		|| (mset.sub.type() == SubData::Vob && !mset.sub.filename().isEmpty());
}

void TCore::unloadSub() {

	mset.current_sub_set_by_user = false;
	mset.current_sub_idx = TMediaSettings::NoneSelected;
	mset.sub = SubData();

	restartPlay();
}

void TCore::loadAudioFile(const QString& audiofile) {

	if (!audiofile.isEmpty()) {
		mset.external_audio = audiofile;
		restartPlay();
	}
}

void TCore::unloadAudioFile() {

	if (!mset.external_audio.isEmpty()) {
		mset.external_audio = "";
		restartPlay();
	}
}

void TCore::openTV(QString channel_id) {
	qDebug("TCore::openTV: '%s'", channel_id.toUtf8().constData());

	close();

	// Use last channel if the name is just "dvb://" or "tv://"
	if ((channel_id == "dvb://") && (!pref->last_dvb_channel.isEmpty())) {
		channel_id = pref->last_dvb_channel;
	}
	else
	if ((channel_id == "tv://") && (!pref->last_tv_channel.isEmpty())) {
		channel_id = pref->last_tv_channel;
	}

	// Save last channel
	if (channel_id.startsWith("dvb://")) pref->last_dvb_channel = channel_id;
	else
	if (channel_id.startsWith("tv://")) pref->last_tv_channel = channel_id;

	mdat.filename = channel_id;
	mdat.selected_type = TMediaData::TYPE_TV;

	mset.reset();
	// Set the default deinterlacer for TV
	mset.current_deinterlacer = pref->initial_tv_deinterlace;
	// Load settings
	if (!pref->dont_remember_media_settings) {
		Settings::TTVSettings settings;
		if (settings.existSettingsFor(channel_id)) {
			settings.loadSettingsFor(channel_id, mset, proc->player());
		}
	}

	initPlaying();
}

void TCore::openStream(QString name) {
	qDebug("TCore::openStream: '%s'", name.toUtf8().data());

#ifdef YOUTUBE_SUPPORT
	if (pref->enable_yt_support) {
		// Check if the stream is a youtube url
		QString yt_full_url = yt->fullUrl(name);
		if (!yt_full_url.isEmpty()) {
			qDebug("TCore::openStream: youtube url detected: %s", yt_full_url.toLatin1().constData());
			name = yt_full_url;
			yt->setPreferredQuality((RetrieveYoutubeUrl::Quality) pref->yt_quality);
			qDebug("TCore::openStream: user_agent: '%s'", pref->yt_user_agent.toUtf8().constData());
			yt->setUserAgent(pref->yt_user_agent);
#ifdef YT_USE_YTSIG
			YTSig::setScriptFile(TPaths::configPath() + "/yt.js");
#endif
			yt->fetchPage(name);
			return;
		}
	}
#endif

	close();
	mdat.filename = name;
	mdat.selected_type = TMediaData::TYPE_STREAM;
	mset.reset();

	initPlaying();
}

void TCore::openFile(QString filename, int seek) {
	qDebug("TCore::openFile: '%s'", filename.toUtf8().data());

	close();
	mdat.filename = filename;
	mdat.selected_type = TMediaData::TYPE_FILE;
	mset.reset();

	// Check if we have info about this file
	if (!pref->dont_remember_media_settings) {
		if (pref->file_settings_method.toLower() == "hash") {
			Settings::TFileSettingsHash settings(mdat.filename);
			if (settings.existSettingsFor(mdat.filename)) {
				settings.loadSettingsFor(mdat.filename, mset, proc->player());
			}
		} else {
			Settings::TFileSettings settings;
			if (settings.existSettingsFor(mdat.filename)) {
				settings.loadSettingsFor(mdat.filename, mset, proc->player());
			}
		}

		if (pref->dont_remember_time_pos) {
			mset.current_sec = 0;
			qDebug("TCore::openFile: Time pos reset to 0");
		}
	}

	initPlaying(seek);
}


void TCore::restartPlay() {
	we_are_restarting = true;

	// For DVDNAV remember the current title, pos and menu.
	if (mdat.detected_type == TMediaData::TYPE_DVDNAV) {
		title = mdat.titles.getSelectedID();
		title_time = mset.current_sec - 10;
		title_was_menu = mdat.title_is_menu;
		TDiscData disc = TDiscName::split(mdat.filename);
		disc.title = 0;
		mdat.filename = TDiscName::join(disc);
		mdat.selected_type = TMediaData::TYPE_DVDNAV;
		qDebug() << "TCore::restartPlay: restarting" << mdat.filename;
	} else {
		title = -1;
	}

	if (proc->isRunning()) {
		stopPlayer();
	}

	initPlaying();
}

void TCore::initVolume() {

	// Keep currrent volume if no media settings are loaded.
	// restore_volume is set to true by mset.reset and set
	// to false by mset.load
	if (mset.restore_volume) {
		qDebug("TCore::initVolume: keeping current volume");
		mset.volume = mset.old_volume;
		mset.mute = mset.old_mute;
	} else if (!pref->global_volume) {
		if (mset.old_volume != mset.volume) {
			qDebug("TCore::initVolume: emit volumeChanged()");
			emit volumeChanged(mset.volume);
		}
		if (mset.old_mute != mset.mute) {
			qDebug("TCore::initVolume: emit muteChanged()");
			emit muteChanged(mset.mute);
		}
	}
}

void TCore::initMediaSettings() {
	qDebug("TCore::initMediaSettings");

	// Restore old volume or emit new volume
	initVolume();

	// Apply settings to playerwindow
	playerwindow->set(
		mset.aspectToNum((TMediaSettings::Aspect) mset.aspect_ratio_id),
		mset.zoom_factor, mset.zoom_factor_fullscreen,
		mset.pan_offset, mset.pan_offset_fullscreen);

	// Feedback and prevent artifacts waiting for redraw
	playerwindow->repaint();

	emit mediaSettingsChanged();
}

void TCore::initPlaying(int seek) {
	qDebug("TCore::initPlaying: starting time");

	time.start();
	playerwindow->hideLogo();
	if (!we_are_restarting)
		initMediaSettings();

	int start_sec = (int) mset.current_sec;
	if (seek >= 0)
		start_sec = seek;

	// Cannot seek at startup in DVDNAV.
	// See restartPlay() and restoreTitle() for DVDNAV seek.
	if (mdat.selected_type == TMediaData::TYPE_DVDNAV)
		start_sec = 0;

	// Avoid to pass the youtube page url to player
#ifdef YOUTUBE_SUPPORT
	if (pref->enable_yt_support
		&& mdat.selected_type == TMediaData::TYPE_STREAM
		&& mdat.filename == yt->origUrl()) {
		mdat.filename = yt->latestPreferredUrl();
	}
#endif

	startPlayer(mdat.filename, start_sec);
}

void TCore::dvdnavSeek() {

	if (mdat.duration > 0) {
		qDebug("TCore::dvdnavSeek: going back to %f", title_time);
		proc->seek(title_time, 2, true, false);
	} else {
		qDebug("TCore::dvdnavSeek: title duration is 0, skipping seek");
	}
	block_dvd_nav = false;
}

void TCore::dvdnavRestoreTitle() {

	// Restore title, time and menu

	int selected_title = mdat.titles.getSelectedID();
	if (title_to_select == selected_title) {
		if (title_was_menu) {
			if (!mdat.title_is_menu) {
				qDebug("TCore::dvdnavRestoreTitle: going back to menu");
				dvdnavMenu();
			}
			qDebug("TCore::dvdnavRestoreTitle: done, selected menu of title %d", title_to_select);
			block_dvd_nav = false;
			return;
		}
	}

	if (mdat.title_is_menu && mdat.duration <= 0) {
		// Changing title or seeking on a menu does not work :(
		// Risc pressing menu you don't want to press, like settings...
		if (menus_selected >= 2) {
			qDebug("TCore::dvdnavRestoreTitle: failed to leave menu, giving up");
			block_dvd_nav = false;
			return;
		}
		menus_selected++;
		qDebug("TCore::dvdnavRestoreTitle: current title %d is menu, sending select",
			   selected_title);
		dvdnavSelect();
		QTimer::singleShot(500, this, SLOT(dvdnavRestoreTitle()));
		return;
	}

	if (title_to_select == selected_title) {
		if (title_time > 0) {
			qDebug("TCore::dvdnavRestoreTitle: going back to %f", title_time);
			proc->seek(title_time, 2, true, false);
		}
		block_dvd_nav = false;
		return;
	}

	qDebug("TCore::dvdnavRestoreTitle: current title is %d, sending setTitle(%d)",
		   selected_title, title_to_select);
	proc->setTitle(title_to_select);

	if (title_was_menu) {
		qDebug("TCore::dvdnavRestoreTitle: posting dvdnavMenu");
		QTimer::singleShot(500, this, SLOT(dvdnavMenu()));
	} else if (title_time > 0) {
		qDebug("TCore::dvdnavRestoreTitle: posting seek");
		QTimer::singleShot(1000, this, SLOT(dvdnavSeek()));
		return;
	}

	block_dvd_nav = false;
}

// This is reached when a new file has just started playing
void TCore::newMediaPlayingStarted() {
	qDebug("TCore::newMediaPlayingStarted");

	mdat.initialized = true;
	mdat.list();

	// Copy the demuxer
	mset.current_demuxer = mdat.demuxer;
	mset.list();

	qDebug("TCore::newMediaPlayingStarted: emit newMediaStartedPlaying()");
	emit newMediaStartedPlaying();
}

// Slot called when signal playerFullyLoaded arrives.
void TCore::playingStarted() {
	qDebug("TCore::playingStarted");

	setState(Playing);

	if (we_are_restarting) {
		we_are_restarting = false;

		// For DVDNAV go back to where we were.
		// Need timer to give DVDNAV time to update its current state.
		if (title >= 0) {
			title_to_select = title;
			title = -1;
			menus_selected = 0;
			block_dvd_nav = true;
			qDebug("TCore::playingStarted: posting dvdnavRestoreTitle()");
			QTimer::singleShot(1000, this, SLOT(dvdnavRestoreTitle()));
		}

	} else {
		// Handle new media
		newMediaPlayingStarted();
	} 

	if (forced_titles.contains(mdat.filename)) {
		mdat.meta_data["NAME"] = forced_titles[mdat.filename];
	}

#ifdef YOUTUBE_SUPPORT
	if (pref->enable_yt_support) {
		// Change the real url with the youtube page url and set the title
		if (mdat.selected_type == TMediaData::TYPE_STREAM) {
			if (mdat.filename == yt->latestPreferredUrl()) {
				mdat.filename = yt->origUrl();
				mdat.stream_title = yt->urlTitle();
			}
		}
	}
#endif

	qDebug("TCore::playingStarted: emit mediaLoaded()");
	emit mediaLoaded();
	qDebug("TCore::playingStarted: emit mediaInfoChanged()");
	emit mediaInfoChanged();

	qDebug() << "TCore::playingStarted: done in" << time.elapsed() << "ms";
}

void TCore::stop() {
	qDebug() << "TCore::stop: current state:" << stateToString();

	State prev_state = _state;
	stopPlayer();

	// if pressed stop twice, reset video to the beginning
	if ((prev_state == Stopped || pref->reset_stop) && mset.current_sec != 0) {
		qDebug("TCore::stop: resetting current_sec %f to 0", mset.current_sec);
		gotCurrentSec(0);
	}

	emit mediaStopped();
}

void TCore::play() {
	qDebug() << "TCore::play: current state: " << stateToString();

	if (proc->isRunning()) {
		if (_state == Paused) {
			proc->setPause(false);
			setState(Playing);
		}
	} else {
		// if we're stopped, play it again
		if (!mdat.filename.isEmpty()) {
			restartPlay();
		} else {
			emit noFileToPlay();
		}
	}
}

void TCore::pause() {
	qDebug() << "TCore::pause: current state:" << stateToString();

	if (proc->isRunning() && _state != Paused) {
		proc->setPause(true);
		setState(Paused);
	}
}

void TCore::playOrPause() {
	qDebug() << "TCore::playOrPause: current state:" << stateToString();

	if (_state == Playing) {
		pause();
	} else {
		play();
	}
}

void TCore::frameStep() {
	qDebug() << "TCore::frameStep at" <<  mset.current_sec;

	if (proc->isRunning()) {
		if (_state == Paused) {
			proc->frameStep();
		} else {
			pause();
		}
	}
}

void TCore::frameBackStep() {
	qDebug() << "TCore::frameBackStep at" <<  mset.current_sec;

	if (proc->isRunning()) {
		if (_state == Paused) {
			proc->frameBackStep();
		} else {
			pause();
		}
	}
}

void TCore::screenshot() {
	qDebug("TCore::screenshot");

	if ((!pref->screenshot_directory.isEmpty()) &&
		 (QFileInfo(pref->screenshot_directory).isDir()))
	{
		proc->takeScreenshot(Proc::TPlayerProcess::Single, pref->subtitles_on_screenshots);
		qDebug("TCore::screenshot: taken screenshot");
	} else {
		qDebug("TCore::screenshot: error: directory for screenshots not valid");
		emit showMessage(tr("Screenshot NOT taken, folder not configured"));
	}
}

void TCore::screenshots() {
	qDebug("TCore::screenshots");

	if ((!pref->screenshot_directory.isEmpty()) &&
		 (QFileInfo(pref->screenshot_directory).isDir()))
	{
		proc->takeScreenshot(Proc::TPlayerProcess::Multiple, pref->subtitles_on_screenshots);
	} else {
		qDebug("TCore::screenshots: error: directory for screenshots not valid");
		emit showMessage(tr("Screenshots NOT taken, folder not configured"));
	}
}

#ifdef CAPTURE_STREAM
void TCore::switchCapturing() {
	qDebug("TCore::switchCapturing");
	proc->switchCapturing();
}
#endif

bool TCore::videoFiltersEnabled(bool displayMessage) {

#ifndef Q_OS_WIN
	QString msg;
	if (isMPlayer()) {
		if (pref->vdpau.disable_video_filters && pref->vo.startsWith("vdpau")) {
			msg = tr("Using vdpau, the video filters will be ignored");
		}
	} else {
		if (!pref->hwdec.isEmpty() && pref->hwdec != "no") {
			msg = tr("Hardware decoding is enabled, the video filters will be ignored");
		}
	}

	if (displayMessage && !msg.isEmpty()) {
		qDebug("TCore::videoFiltersEnabled: %s", msg.toUtf8().constData());
		emit showMessage(msg, 4000);
	}
#endif

	return true;
}

void TCore::startPlayer(QString file, double seek) {
	qDebug() << "TCore::startPlayer:" << file << "at" << seek;

	if (file.isEmpty()) {
		qWarning("TCore:startPlayer: file is empty!");
		return;
	}

	if (proc->isRunning()) {
		qWarning("TCore::startPlayer: MPlayer still running!");
		return;
    }

	emit showMessage(tr("Starting player..."), 5000);

#ifdef YOUTUBE_SUPPORT
	// Stop any pending request
	#if 0
	qDebug("TCore::startPlayer: yt state: %d", yt->state());
	if (yt->state() != QHttp::Unconnected) {
		//yt->abort(); /* Make the app to crash, don't know why */
	}
	#endif
	yt->close();
#endif

	// Check URL playlist
	bool url_is_playlist = false;
	if (file.endsWith("|playlist")) {
		url_is_playlist = true;
		file = file.remove("|playlist");
	} else {
		QUrl url(file);
		qDebug("TCore::startPlayer: checking if stream is a playlist");
		qDebug("TCore::startPlayer: url path: '%s'", url.path().toUtf8().constData());

		if (url.scheme().toLower() != "ffmpeg") {
			QRegExp rx("\\.ram$|\\.asx$|\\.m3u$|\\.m3u8$|\\.pls$", Qt::CaseInsensitive);
			url_is_playlist = (rx.indexIn(url.path()) != -1);
		}
	}
	qDebug("TCore::startPlayer: url_is_playlist: %d", url_is_playlist);


	// Check if a m4a file exists with the same name of file,
	// in that cause if will be used as audio
	if (pref->autoload_m4a && mset.external_audio.isEmpty()) {
		QFileInfo fi(file);
		if (fi.exists() && !fi.isDir()) {
			if (fi.suffix().toLower() == "mp4") {
				QString file2 = fi.path() + "/" + fi.completeBaseName() + ".m4a";
				//qDebug("TCore::startPlayer: file2: %s", file2.toUtf8().constData());
				if (!QFile::exists(file2)) {
					// Check for upper case
					file2 = fi.path() + "/" + fi.completeBaseName() + ".M4A";
				}
				if (QFile::exists(file2)) {
					qDebug("TCore::startPlayer: found %s, so it will be used as audio file",
						   file2.toUtf8().constData());
					mset.external_audio = file2;
				}
			}
		}
	}

	proc->clearArguments();
	proc->setExecutable(pref->playerAbsolutePath());
	proc->setFixedOptions();

	if (pref->log_verbose) {
		proc->setOption("verbose");
	}

	// Demuxer and audio and video codecs:
	if (!mset.forced_demuxer.isEmpty()) {
		proc->setOption("demuxer", mset.forced_demuxer);
	}
	if (!mset.forced_audio_codec.isEmpty()) {
		proc->setOption("ac", mset.forced_audio_codec);
	}
	if (!mset.forced_video_codec.isEmpty()) {
		proc->setOption("vc", mset.forced_video_codec);
	} else {

#ifndef Q_OS_WIN
		if (pref->vo.startsWith("vdpau")) {
			QString c;
			if (pref->vdpau.ffh264vdpau) c += "ffh264vdpau,";
			if (pref->vdpau.ffmpeg12vdpau) c += "ffmpeg12vdpau,";
			if (pref->vdpau.ffwmv3vdpau) c += "ffwmv3vdpau,";
			if (pref->vdpau.ffvc1vdpau) c += "ffvc1vdpau,";
			if (pref->vdpau.ffodivxvdpau) c += "ffodivxvdpau,";
			if (!c.isEmpty()) {
				proc->setOption("vc", c);
			}
		} else {
#endif

			if (pref->coreavc) {
				proc->setOption("vc", "coreserve,");
			}

#ifndef Q_OS_WIN
		}
#endif

	}

	if (pref->use_hwac3)
		proc->setOption("afm", "hwac3");

	if (isMPlayer()) {
		QString lavdopts;

		if (pref->h264_skip_loop_filter == TPreferences::LoopDisabled
			|| (pref->h264_skip_loop_filter == TPreferences::LoopDisabledOnHD
				&& mset.is264andHD)) {

			if (!lavdopts.isEmpty())
				lavdopts += ":";
			lavdopts += "skiploopfilter=all";
		}

		if (pref->threads > 1) {
			if (!lavdopts.isEmpty())
				lavdopts += ":";
			lavdopts += "threads=" + QString::number(pref->threads);
		}

		if (!lavdopts.isEmpty()) {
			proc->setOption("lavdopts", lavdopts);
		}
	}
	else {
		// MPV
		if (pref->h264_skip_loop_filter == TPreferences::LoopDisabled
			|| (pref->h264_skip_loop_filter == TPreferences::LoopDisabledOnHD
				&& mset.is264andHD)) {
			proc->setOption("skiploopfilter");
		}

		if (pref->threads > 1) {
			proc->setOption("threads", QString::number(pref->threads));
		}
	}

	if (!pref->hwdec.isEmpty())
		proc->setOption("hwdec", pref->hwdec);

	// Set screenshot directory
	bool screenshot_enabled = pref->use_screenshot
							  && !pref->screenshot_directory.isEmpty()
							  && QFileInfo(pref->screenshot_directory).isDir();
	if (screenshot_enabled)
		proc->setScreenshotDirectory(pref->screenshot_directory);


	proc->setOption("sub-fuzziness", pref->subfuzziness);

	if (pref->vo != "player_default") {
		if (!pref->vo.isEmpty()) {
			proc->setOption("vo", pref->vo);
		} else {

#ifdef Q_OS_WIN
			if (QSysInfo::WindowsVersion >= QSysInfo::WV_VISTA) {
				proc->setOption("vo", "direct3d,");
			} else {
				proc->setOption("vo", "directx,");
			}
#else
			proc->setOption("vo", "xv,");
#endif

		}
	}

#if USE_ADAPTER
	if (pref->adapter > -1) {
		proc->setOption("adapter", QString::number(pref->adapter));
	}
#endif

	if (pref->ao != "player_default") {
		if (!pref->ao.isEmpty()) {
			proc->setOption("ao", pref->ao);
		}
	}

#if !defined(Q_OS_WIN) && !defined(Q_OS_OS2)
	if (pref->vo.startsWith("x11")) {
		proc->setOption("zoom");
	}
#endif

	// Performance options
#ifdef Q_OS_WIN
	QString p;
	int app_p = NORMAL_PRIORITY_CLASS;
	switch (pref->priority) {
		case TPreferences::Realtime: 	p = "realtime";
										app_p = REALTIME_PRIORITY_CLASS;
										break;
		case TPreferences::High:			p = "high";
										app_p = REALTIME_PRIORITY_CLASS;
										break;
		case TPreferences::AboveNormal:	p = "abovenormal";
										app_p = HIGH_PRIORITY_CLASS;
										break;
		case TPreferences::Normal: 		p = "normal";
										app_p = ABOVE_NORMAL_PRIORITY_CLASS; 
										break;
		case TPreferences::BelowNormal: 	p = "belownormal"; break;
		case TPreferences::Idle: 		p = "idle"; break;
		default: 						p = "normal";
	}
	proc->setOption("priority", p);
	/*
	SetPriorityClass(GetCurrentProcess(), app_p);
	qDebug("TCore::startPlayer: priority of smplayer process set to %d", app_p);
	*/
#endif

	if (pref->frame_drop && pref->hard_frame_drop) {
		proc->setOption("framedrop", "decoder+vo");
	} else if (pref->frame_drop) {
		proc->setOption("framedrop", "vo");
	} else if (pref->hard_frame_drop) {
		proc->setOption("framedrop", "decoder");
	}

	if (pref->autosync) {
		proc->setOption("autosync", QString::number(pref->autosync_factor));
	}

	if (pref->use_mc) {
		proc->setOption("mc", QString::number(pref->mc_value));
	}

	proc->setOption("dr", pref->use_direct_rendering);
	proc->setOption("double", pref->use_double_buffer);

#ifdef Q_WS_X11
	proc->setOption("stop-xscreensaver", pref->disable_screensaver);
#endif

	proc->disableInput();
	proc->setOption("keepaspect", false);

#if defined(Q_OS_OS2)
#define WINIDFROMHWND(hwnd) ((hwnd) - 0x80000000UL)
	proc->setOption("wid", QString::number(WINIDFROMHWND((int) playerwindow->videoLayer()->winId())));
#else
	proc->setOption("wid", QString::number((qint64) playerwindow->videoLayer()->winId()));
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	if ((pref->vo.startsWith("directx")) || (pref->vo.startsWith("kva")) || (pref->vo.isEmpty())) {
		proc->setOption("colorkey", ColorUtils::colorToRGB(pref->color_key));
	}
#endif

	// Square pixels
	proc->setOption("monitorpixelaspect", "1");

	// OSD
	proc->setOption("osdlevel", pref->osd_level);
	if (isMPlayer()) {
		proc->setOption("osd-scale", pref->subfont_osd_scale);
	} else {
		proc->setOption("osd-scale", pref->osd_scale);
		proc->setOption("osd-scale-by-window", "no");
	}

	// Subtitles fonts
	if ((pref->use_ass_subtitles) && (pref->freetype_support)) {
		// ASS:
		proc->setOption("ass");
		proc->setOption("embeddedfonts");

		proc->setOption("ass-line-spacing", QString::number(pref->ass_line_spacing));

		proc->setOption("ass-font-scale", QString::number(mset.sub_scale_ass));

		proc->setOption("flip-hebrew", false); // It seems to be necessary to display arabic subtitles correctly when using -ass

		if (pref->enable_ass_styles) {
			QString ass_force_style;
			if (!pref->user_forced_ass_style.isEmpty()) {
				ass_force_style = pref->user_forced_ass_style;
			} else {
				ass_force_style = pref->ass_styles.toString();
			}

			if (isMPV()) {
				// MPV
				proc->setSubStyles(pref->ass_styles);
				if (pref->force_ass_styles) {
					proc->setOption("ass-force-style", ass_force_style);
				}
			} else {
				// MPlayer
				if (!pref->force_ass_styles) {
					proc->setSubStyles(pref->ass_styles, TPaths::subtitleStyleFile());
				} else {
					proc->setOption("ass-force-style", ass_force_style);
				}
			}
		}

		// Use the same font for OSD
		// deleted

		// Set the size of OSD
		// deleted
	} else {
		// NO ASS:
		if (pref->freetype_support) proc->setOption("noass");
		if (isMPV()) {
			proc->setOption("sub-scale", QString::number(mset.sub_scale_mpv));
		} else {
			proc->setOption("subfont-text-scale", QString::number(mset.sub_scale));
		}
	}

	// Subtitle encoding
	{
		QString encoding;
		if ((pref->use_enca) && (!pref->enca_lang.isEmpty())) {
			encoding = "enca:"+ pref->enca_lang;
			if (!pref->subcp.isEmpty()) {
				encoding += ":"+ pref->subcp;
			}
		}
		else
		if (!pref->subcp.isEmpty()) {
			encoding = pref->subcp;
		}

		if (!encoding.isEmpty()) {
			proc->setOption("subcp", encoding);
		}
	}

	if (mset.closed_caption_channel > 0) {
		proc->setOption("subcc", QString::number(mset.closed_caption_channel));
	}

	if (pref->use_forced_subs_only) {
		proc->setOption("forcedsubsonly");
	}

#if PROGRAM_SWITCH
	if ((mset.current_program_id != TMediaSettings::NoneSelected) /*&&
         (mset.current_video_id == TMediaSettings::NoneSelected) && 
		 (mset.current_audio_id == TMediaSettings::NoneSelected)*/)
	{
		proc->setOption("tsprog", QString::number(mset.current_program_id));
	}
	// Don't set video and audio track if using -tsprog
	else {
#endif

	if (mset.current_video_id >= 0) {
		proc->setOption("vid", QString::number(mset.current_video_id));
	}

	if (mset.external_audio.isEmpty()) {
		if (mset.current_audio_id >= 0) {
			// Workaround for MPlayer bug #1321 (http://bugzilla.mplayerhq.hu/show_bug.cgi?id=1321)
			if (mdat.audios.count() != 1) {
				proc->setOption("aid", QString::number(mset.current_audio_id));
			}
		}
	}

#if PROGRAM_SWITCH
	}
#endif

	// Subtitles
	// Setup external sub from command line or other instance
	if (!initial_subtitle.isEmpty()) {
		setExternalSubs(initial_subtitle);
		initial_subtitle = "";
	} else if (mset.current_sub_set_by_user) {
		// Selected sub when restarting
		if (mset.current_sub_idx >= 0) {
			mset.sub = mdat.subs.itemAt(mset.current_sub_idx);
		} else {
			proc->setOption("nosub");
		}
	}

	if (mset.sub.type() == SubData::Vob) {
		// Set vob subs. Mplayer only
		// External sub
		if (!mset.sub.filename().isEmpty()) {
			// Remove extension
			QFileInfo fi(mset.sub.filename());
			QString s = fi.path() +"/"+ fi.completeBaseName();
			qDebug("TCore::startPlayer: subtitle file without extension: '%s'", s.toUtf8().data());
			proc->setOption("vobsub", s);
		}
		if (mset.sub.ID() >= 0) {
			proc->setOption("vobsubid", mset.sub.ID());
		}
	} else if (mset.sub.type() == SubData::File) {
		if (!mset.sub.filename().isEmpty()) {
			proc->setOption("sub", mset.sub.filename());
			if (mset.sub.ID() >= 0) {
				proc->setOption("sid", mset.sub.ID());
			}
		}
	} else if (mset.sub.type() == SubData::Sub && mset.sub.ID() >= 0) {
		// Subs from demux when restarting or from settings local file
		proc->setOption("sid", mset.sub.ID());
	}

	// Set fps external file
	if (!mset.sub.filename().isEmpty()
		&& mset.external_subtitles_fps != TMediaSettings::SFPS_None) {

		QString fps;
		switch (mset.external_subtitles_fps) {
			case TMediaSettings::SFPS_23: fps = "23"; break;
			case TMediaSettings::SFPS_24: fps = "24"; break;
			case TMediaSettings::SFPS_25: fps = "25"; break;
			case TMediaSettings::SFPS_30: fps = "30"; break;
			case TMediaSettings::SFPS_23976: fps = "24000/1001"; break;
			case TMediaSettings::SFPS_29970: fps = "30000/1001"; break;
			default: fps = "25";
		}
		proc->setOption("subfps", fps);
	}

	if (!mset.external_audio.isEmpty()) {
		proc->setOption("audiofile", mset.external_audio);
	}

	proc->setOption("subpos", QString::number(mset.sub_pos));

	if (mset.audio_delay != 0) {
		proc->setOption("delay", QString::number((double) mset.audio_delay/1000));
	}

	if (mset.sub_delay != 0) {
		proc->setOption("subdelay", QString::number((double) mset.sub_delay/1000));
	}

	// Contrast, brightness...
	if (pref->change_video_equalizer_on_startup) {
		if (mset.contrast != 0) {
			proc->setOption("contrast", QString::number(mset.contrast));
		}
	
		if (mset.brightness != 0) {
			proc->setOption("brightness", QString::number(mset.brightness));
		}

		if (mset.hue != 0) {
			proc->setOption("hue", QString::number(mset.hue));
		}

		if (mset.saturation != 0) {
			proc->setOption("saturation", QString::number(mset.saturation));
		}

		if (mset.gamma != 0) {
			proc->setOption("gamma", QString::number(mset.gamma));
		}
	}


	if (pref->mplayer_additional_options.contains("-volume")) {
		qDebug("TCore::startPlayer: don't set volume since -volume is used");
	} else {
		int vol = getVolume();
		if (isMPV()) {
			vol = adjustVolume(vol);
		}
		proc->setOption("volume", QString::number(vol));
	}

	if (getMute()) {
		proc->setOption("mute");
	}

	if (mset.current_angle_id > 0) {
		proc->setOption("dvdangle", QString::number(mset.current_angle_id));
	}

	// TMPVProcess title and track switch code does not run nicely when caching
	// is set. Seeks inside the cache don't notify track or title changes.
	// So instead of honouring some cache settings and not others, for now,
	// declare them MPlayer only.
	if (isMPlayer()) {
		switch (mdat.selected_type) {
			case TMediaData::TYPE_FILE	: cache_size = pref->cache_for_files; break;
			case TMediaData::TYPE_DVD 	: cache_size = pref->cache_for_dvds; break;
			case TMediaData::TYPE_DVDNAV	: cache_size = 0; break;
			case TMediaData::TYPE_STREAM	: cache_size = pref->cache_for_streams; break;
			case TMediaData::TYPE_VCD 	: cache_size = pref->cache_for_vcds; break;
			case TMediaData::TYPE_CDDA	: cache_size = pref->cache_for_audiocds; break;
			case TMediaData::TYPE_TV		: cache_size = pref->cache_for_tv; break;
			case TMediaData::TYPE_BLURAY	: cache_size = pref->cache_for_dvds; break; // FIXME: use cache for bluray?
			default: cache_size = 0;
		} // switch

		proc->setOption("cache", QString::number(cache_size));
	} else {
		cache_size = -1;
	}

	if (mset.speed != 1.0) {
		proc->setOption("speed", QString::number(mset.speed));
	}

	if (mdat.selected_type != TMediaData::TYPE_TV) {
		// Play A - B
		if ((mset.A_marker > -1) && (mset.B_marker > mset.A_marker)) {
			proc->setOption("ss", QString::number(mset.A_marker));
			proc->setOption("endpos", QString::number(mset.B_marker - mset.A_marker));
		}
		else
		// If seek < 5 it's better to allow the video to start from the beginning
		if ((seek >= 5) && (!mset.loop)) {
			proc->setOption("ss", QString::number(seek));
		}
	}

	if (pref->use_idx) {
		proc->setOption("idx");
	}

	if (mdat.selected_type == TMediaData::TYPE_STREAM) {
		if (pref->prefer_ipv4) {
			proc->setOption("prefer-ipv4");
		} else {
			proc->setOption("prefer-ipv6");
		}
	}

	if (pref->use_correct_pts != TPreferences::Detect) {
		proc->setOption("correct-pts", (pref->use_correct_pts == TPreferences::Enabled));
	}

	bool force_noslices = false;

	if (!videoFiltersEnabled(true))
		goto end_video_filters;

	// Video filters:
	// Phase
	if (mset.phase_filter) {
		proc->addVF("phase", "A");
	}

	// Deinterlace
	if (mset.current_deinterlacer != TMediaSettings::NoDeinterlace) {
		switch (mset.current_deinterlacer) {
			case TMediaSettings::L5: 		proc->addVF("l5"); break;
			case TMediaSettings::Yadif: 	proc->addVF("yadif"); break;
			case TMediaSettings::LB:		proc->addVF("lb"); break;
			case TMediaSettings::Yadif_1:	proc->addVF("yadif", "1"); break;
			case TMediaSettings::Kerndeint:	proc->addVF("kerndeint", "5"); break;
		}
	}

	// 3D stereo
	if (mset.stereo3d_in != "none" && !mset.stereo3d_out.isEmpty()) {
		proc->addStereo3DFilter(mset.stereo3d_in, mset.stereo3d_out);
	}

	// Denoise
	if (mset.current_denoiser != TMediaSettings::NoDenoise) {
		if (mset.current_denoiser==TMediaSettings::DenoiseSoft) {
			proc->addVF("hqdn3d", pref->filters.item("denoise_soft").options());
		} else {
			proc->addVF("hqdn3d", pref->filters.item("denoise_normal").options());
		}
	}

	// Unsharp
	if (mset.current_unsharp != 0) {
		if (mset.current_unsharp == 1) {
			proc->addVF("blur", pref->filters.item("blur").options());
		} else {
			proc->addVF("sharpen", pref->filters.item("sharpen").options());
		}
	}

	// Deblock
	if (mset.deblock_filter) {
		proc->addVF("deblock", pref->filters.item("deblock").options());
	}

	// Dering
	if (mset.dering_filter) {
		proc->addVF("dering");
	}

	// Gradfun
	if (mset.gradfun_filter) {
		proc->addVF("gradfun", pref->filters.item("gradfun").options());
	}

	// Upscale
	if (mset.upscaling_filter) {
		int width = TDesktop::size(playerwindow).width();
		proc->setOption("sws", "9");
		proc->addVF("scale", QString::number(width) + ":-2");
	}

	// Addnoise
	if (mset.noise_filter) {
		proc->addVF("noise", pref->filters.item("noise").options());
	}

	// Postprocessing
	if (mset.postprocessing_filter) {
		proc->addVF("postprocessing");
		proc->setOption("autoq", QString::number(pref->autoq));
	}

	// Letterbox (expand)
	if ((mset.add_letterbox) || (pref->fullscreen && pref->add_blackborders_on_fullscreen)) {
		proc->addVF("expand", QString("aspect=%1").arg(TDesktop::aspectRatio(playerwindow)));
	}

	// Software equalizer
	if ((pref->use_soft_video_eq)) {
		proc->addVF("eq2");
		proc->addVF("hue");
		if ((pref->vo == "gl") || (pref->vo == "gl2") || (pref->vo == "gl_tiled")
#ifdef Q_OS_WIN
             || (pref->vo == "directx:noaccel")
#endif
		   )
		{
			proc->addVF("scale");
		}
	}

	// Additional video filters, supplied by user
	// File
	if (!mset.mplayer_additional_video_filters.isEmpty()) {
		proc->setOption("vf-add", mset.mplayer_additional_video_filters);
	}
	// Global
	if (!pref->mplayer_additional_video_filters.isEmpty()) {
		proc->setOption("vf-add", pref->mplayer_additional_video_filters);
	}

	// Filters for subtitles on screenshots
	if ((screenshot_enabled) && (pref->subtitles_on_screenshots)) 
	{
		if (pref->use_ass_subtitles) {
			proc->addVF("subs_on_screenshots", "ass");
		} else {
			proc->addVF("subs_on_screenshots");
			force_noslices = true;
		}
	}

	// Rotate
	if (mset.rotate != TMediaSettings::NoRotate) {
		proc->addVF("rotate", QString::number(mset.rotate));
	}

	// Flip
	if (mset.flip) {
		proc->addVF("flip");
	}

	// Mirror
	if (mset.mirror) {
		proc->addVF("mirror");
	}

	// Screenshots
	if (screenshot_enabled) {
		proc->addVF("screenshot");
	}

#ifndef Q_OS_WIN
	end_video_filters:
#endif

#ifdef MPV_SUPPORT
	// Template for screenshots (only works with mpv)
	if (screenshot_enabled) {
		if (!pref->screenshot_template.isEmpty()) {
			proc->setOption("screenshot_template", pref->screenshot_template);
		}
		if (!pref->screenshot_format.isEmpty()) {
			proc->setOption("screenshot_format", pref->screenshot_format);
		}
	}
#endif

	// slices
	if ((pref->use_slices) && (!force_noslices)) {
		proc->setOption("slices", true);
	} else {
		proc->setOption("slices", false);
	}


	// Audio channels
	if (mset.audio_use_channels != 0) {
		proc->setOption("channels", QString::number(mset.audio_use_channels));
	}

	if (!pref->use_hwac3) {

		// Audio filters
#ifdef MPLAYER_SUPPORT
		if (mset.karaoke_filter) {
			proc->addAF("karaoke");
		}
#endif

		// Stereo mode
		if (mset.stereo_mode != 0) {
			switch (mset.stereo_mode) {
				case TMediaSettings::Left: proc->addAF("channels", "2:2:0:1:0:0"); break;
				case TMediaSettings::Right: proc->addAF("channels", "2:2:1:0:1:1"); break;
				case TMediaSettings::Mono: proc->addAF("pan", "1:0.5:0.5"); break;
				case TMediaSettings::Reverse: proc->addAF("channels", "2:2:0:1:1:0"); break;
			}
		}

#ifdef MPLAYER_SUPPORT
		if (mset.extrastereo_filter) {
			proc->addAF("extrastereo");
		}
#endif

		if (mset.volnorm_filter) {
			proc->addAF("volnorm", pref->filters.item("volnorm").options());
		}

		bool use_scaletempo = pref->use_scaletempo == TPreferences::Enabled;
		if (pref->use_scaletempo == TPreferences::Detect) {
			use_scaletempo = true;
		}
		if (use_scaletempo) {
			proc->addAF("scaletempo");
		}

		// Audio equalizer
		if (pref->use_audio_equalizer) {
			TAudioEqualizerList l = pref->global_audio_equalizer ? pref->audio_equalizer : mset.audio_equalizer;
			proc->addAF("equalizer", Helper::equalizerListToString(l));
		}

		// Additional audio filters, supplied by user
		// File
		if (!pref->mplayer_additional_audio_filters.isEmpty()) {
			proc->setOption("af-add", pref->mplayer_additional_audio_filters);
		}
		// Global
		if (!mset.mplayer_additional_audio_filters.isEmpty()) {
			proc->setOption("af-add", mset.mplayer_additional_audio_filters);
		}
	} else {
		// Don't use audio filters if using the S/PDIF output
			qDebug("TCore::startPlayer: audio filters are disabled when using the S/PDIF output!");
	}

	if (pref->use_soft_vol) {
		proc->setOption("softvol");
		proc->setOption("softvol-max", QString::number(pref->softvol_max));
	}

#ifndef Q_OS_WIN
	if (isMPV() && file.startsWith("dvb:")) {
		QString channels_file = Gui::TTVList::findChannelsFile();
		qDebug("TCore::startPlayer: channels_file: %s", channels_file.toUtf8().constData());
		if (!channels_file.isEmpty()) proc->setChannelsFile(channels_file);
	}
#endif

#ifdef CAPTURE_STREAM
	// Set the capture directory
	proc->setCaptureDirectory(pref->capture_directory);
#endif

	// Load edl file
	if (pref->use_edl_files) {
		QString edl_f;
		QFileInfo f(file);
		QString basename = f.path() + "/" + f.completeBaseName();

		//qDebug("TCore::startPlayer: file basename: '%s'", basename.toUtf8().data());

		if (QFile::exists(basename+".edl")) 
			edl_f = basename+".edl";
		else
		if (QFile::exists(basename+".EDL")) 
			edl_f = basename+".EDL";

		qDebug("TCore::startPlayer: edl file: '%s'", edl_f.toUtf8().data());
		if (!edl_f.isEmpty()) {
			proc->setOption("edl", edl_f);
		}
	}

	// Additional options supplied by the user
	// File
	if (!mset.mplayer_additional_options.isEmpty()) {
		QStringList args = Proc::TProcess::splitArguments(mset.mplayer_additional_options);
		for (int n = 0; n < args.count(); n++) {
			QString arg = args[n].simplified();
			if (!arg.isEmpty()) {
				proc->addUserOption(arg);
			}
		}
	}

	// Global
	if (!pref->mplayer_additional_options.isEmpty()) {
		QStringList args = Proc::TProcess::splitArguments(pref->mplayer_additional_options);
		for (int n = 0; n < args.count(); n++) {
			QString arg = args[n].simplified();
			if (!arg.isEmpty()) {
				qDebug("arg %d: %s", n, arg.toUtf8().constData());
				proc->addUserOption(arg);
			}
		}

	}

	// Last checks for the file
	if (isMPlayer()) {
		proc->setMedia(file, pref->use_playlist_option ? url_is_playlist : false);
	} else {
		proc->setMedia(file, false); // Don't use playlist with mpv
	}

	// It seems the loop option must be after the filename
	if (mset.loop) {
		proc->setOption("loop", "0");
	}

	emit aboutToStartPlaying();

	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	if (pref->use_proxy
		&& pref->proxy_type == QNetworkProxy::HttpProxy
		&& !pref->proxy_host.isEmpty()) {
		QString proxy = QString("http://%1:%2@%3:%4").arg(pref->proxy_username).arg(pref->proxy_password).arg(pref->proxy_host).arg(pref->proxy_port);
		env.insert("http_proxy", proxy);
	}

#ifdef Q_OS_WIN
	if (!pref->use_windowsfontdir) {
		env.insert("FONTCONFIG_FILE", TPaths::configPath() + "/fonts.conf");
	}
#endif

	proc->setProcessEnvironment(env);

	if (!proc->startPlayer()) {
		// TODO: error handling
		qWarning("TCore::startPlayer: player process didn't start");
	}
} //startPlayer()

void TCore::stopPlayer() {

	if (!proc->isRunning()) {
		qDebug("TCore::stopPlayer: player not running");
		return;
	}
	qDebug("TCore::stopPlayer");

	// If set high enough the OS will detect the "not responding state" and popup a dialog
	int timeout = pref->time_to_kill_mplayer;
	if (timeout < 5000) {
		qDebug("TCore::stopPlayer: timeout %d too small, adjusting it to 5000 ms", timeout);
		timeout = 5000;
	}

#ifdef Q_OS_OS2
	QEventLoop eventLoop;

	connect(proc, SIGNAL(processExited(bool)), &eventLoop, SLOT(quit()));

	proc->quit(0);

	QTimer::singleShot(timeout, &eventLoop, SLOT(quit()));
	eventLoop.exec(QEventLoop::ExcludeUserInputEvents);

	if (proc->isRunning()) {
		qWarning("TCore::stopPlayer: player didn't finish. Killing it...");
		proc->kill();
	}
#else
	proc->quit(0);

	qDebug("TCore::stopPlayer: Waiting %d ms for player to finish...", timeout);
	if (!proc->waitForFinished(timeout)) {
		qWarning("TCore::stopPlayer: player process did not finish in %d ms. Killing it...",
				 timeout);
		proc->kill();
	}
#endif

	qDebug("TCore::stopPlayer: Finished. (I hope)");
}

void TCore::goToPosition(int pos) {
	qDebug("TCore::goToPosition: %d/%d", pos, pos_max);

	if (pos < 0) pos = 0;
	else if (pos >= pos_max) pos = pos_max - 1;

	if (pref->relative_seeking || mdat.duration <= 0) {
		goToPos((double) pos / ((double) pos_max / 100));
	} else {
		goToSec(mdat.duration * pos / pos_max);
	}
}

void TCore::goToPos(double perc) {
	qDebug("TCore::goToPos: per: %f", perc);
	seek_cmd(perc, 1);
}

void TCore::goToSec(double sec) {
	qDebug("TCore::goToSec: %f", sec);
	seek_cmd(sec, 2);
}

void TCore::seek(int secs) {
	qDebug("TCore::seek: seek relative %d secs", secs);

	// Something to seek?
	if (qAbs(secs) > 0.01)
		seek_cmd(secs, 0);
}

void TCore::seek_cmd(double secs, int mode) {

	//seek <value> [type]
	//    Seek to some place in the movie.
	//        0 is a relative seek of +/- <value> seconds (default).
	//        1 is a seek to <value> % in the movie.
	//        2 is a seek to an absolute position of <value> seconds.
	if (mode != 0) {
		if (secs < 0)
			secs = 0;
		if (mode == 2) {
			if (mdat.duration > 0 && secs >= mdat.duration) {
				if (mdat.video_fps > 0)
					secs = mdat.duration - (1.0 / mdat.video_fps);
				else secs = mdat.duration - 0.1;
			}
		}
	}

	if (proc->isFullyStarted())
		proc->seek(secs, mode, pref->precise_seeking, _state == Paused);
}

void TCore::sforward() {
	qDebug("TCore::sforward");
	seek(pref->seeking1); // +10s
}

void TCore::srewind() {
	qDebug("TCore::srewind");
	seek(-pref->seeking1); // -10s
}


void TCore::forward() {
	qDebug("TCore::forward");
	seek(pref->seeking2); // +1m
}


void TCore::rewind() {
	qDebug("TCore::rewind");
	seek(-pref->seeking2); // -1m
}


void TCore::fastforward() {
	qDebug("TCore::fastforward");
	seek(pref->seeking3); // +10m
}


void TCore::fastrewind() {
	qDebug("TCore::fastrewind");
	seek(-pref->seeking3); // -10m
}

void TCore::forward(int secs) {
	qDebug("TCore::forward: %d", secs);
	seek(secs);
}

void TCore::rewind(int secs) {
	qDebug("TCore::rewind: %d", secs);
	seek(-secs);
}

#ifdef MPV_SUPPORT
void TCore::seekToNextSub() {
	qDebug("TCore::seekToNextSub");
	proc->seekSub(1);
}

void TCore::seekToPrevSub() {
	qDebug("TCore::seekToPrevSub");
	proc->seekSub(-1);
}
#endif

void TCore::wheelUp(TPreferences::WheelFunction function) {
	qDebug("TCore::wheelUp");

	if (function == TPreferences::DoNothing)
		function = (TPreferences::WheelFunction) pref->wheel_function;
	switch (function) {
		case TPreferences::Volume : incVolume(); break;
		case TPreferences::Zoom : incZoom(); break;
		case TPreferences::Seeking : pref->wheel_function_seeking_reverse ? rewind(pref->seeking4) : forward(pref->seeking4); break;
		case TPreferences::ChangeSpeed : incSpeed10(); break;
		default : {} // do nothing
	}
}

void TCore::wheelDown(TPreferences::WheelFunction function) {
	qDebug("TCore::wheelDown");

	if (function == TPreferences::DoNothing)
		function = (TPreferences::WheelFunction) pref->wheel_function;
	switch (function) {
		case TPreferences::Volume : decVolume(); break;
		case TPreferences::Zoom : decZoom(); break;
		case TPreferences::Seeking : pref->wheel_function_seeking_reverse ? forward(pref->seeking4) : rewind(pref->seeking4); break;
		case TPreferences::ChangeSpeed : decSpeed10(); break;
		default : {} // do nothing
	}
}

void TCore::setAMarker() {
	setAMarker((int)mset.current_sec);
}

void TCore::setAMarker(int sec) {
	qDebug("TCore::setAMarker: %d", sec);

	mset.A_marker = sec;
	displayMessage(tr("\"A\" marker set to %1").arg(Helper::formatTime(sec)));

	if (mset.B_marker > mset.A_marker) {
		if (proc->isRunning()) restartPlay();
	}

	emit ABMarkersChanged();
}

void TCore::setBMarker() {
	setBMarker((int)mset.current_sec);
}

void TCore::setBMarker(int sec) {
	qDebug("TCore::setBMarker: %d", sec);

	mset.B_marker = sec;
	displayMessage(tr("\"B\" marker set to %1").arg(Helper::formatTime(sec)));

	if ((mset.A_marker > -1) && (mset.A_marker < mset.B_marker)) {
		if (proc->isRunning()) restartPlay();
	}

	emit ABMarkersChanged();
}

void TCore::clearABMarkers() {
	qDebug("TCore::clearABMarkers");

	if ((mset.A_marker != -1) || (mset.B_marker != -1)) {
		mset.A_marker = -1;
		mset.B_marker = -1;
		displayMessage(tr("A-B markers cleared"));
		if (proc->isRunning())
			restartPlay();
	}

	emit ABMarkersChanged();
}

void TCore::toggleRepeat(bool b) {
	qDebug("TCore::toggleRepeat: %d", b);

	if (mset.loop != b) {
		mset.loop = b;
		int v = -1; // no loop
		if (mset.loop)
			v = 0; // infinite loop
		proc->setLoop(v);
	}
}

// Audio filters
#ifdef MPLAYER_SUPPORT
void TCore::toggleKaraoke() {
	toggleKaraoke(!mset.karaoke_filter);
}

void TCore::toggleKaraoke(bool b) {
	qDebug("TCore::toggleKaraoke: %d", b);
	if (b != mset.karaoke_filter) {
		mset.karaoke_filter = b;
		proc->enableKaraoke(b);
	}
}

void TCore::toggleExtrastereo() {
	toggleExtrastereo(!mset.extrastereo_filter);
}

void TCore::toggleExtrastereo(bool b) {
	qDebug("TCore::toggleExtrastereo: %d", b);
	if (b != mset.extrastereo_filter) {
		mset.extrastereo_filter = b;
		proc->enableExtrastereo(b);
	}
}
#endif

void TCore::toggleVolnorm() {
	toggleVolnorm(!mset.volnorm_filter);
}

void TCore::toggleVolnorm(bool b) {
	qDebug("TCore::toggleVolnorm: %d", b);

	if (b != mset.volnorm_filter) {
		mset.volnorm_filter = b;
		QString f = pref->filters.item("volnorm").filter();
		proc->enableVolnorm(b, pref->filters.item("volnorm").options());
	}
}

void TCore::setAudioChannels(int channels) {
	qDebug("TCore::setAudioChannels:%d", channels);

	if (channels != mset.audio_use_channels) {
		mset.audio_use_channels = channels;
		if (proc->isRunning())
			restartPlay();
	}
}

void TCore::setStereoMode(int mode) {
	qDebug("TCore::setStereoMode:%d", mode);
	if (mode != mset.stereo_mode) {
		mset.stereo_mode = mode;
		if (proc->isRunning())
			restartPlay();
	}
}


// Video filters

#define CHANGE_VF(Filter, Enable, Option) \
	if (isMPV()) { \
		proc->changeVF(Filter, Enable, Option); \
	} else { \
		restartPlay(); \
	}

void TCore::toggleFlip() {
	qDebug("TCore::toggleFlip");
	toggleFlip(!mset.flip);
}

void TCore::toggleFlip(bool b) {
	qDebug("TCore::toggleFlip: %d", b);
	if (mset.flip != b) {
		mset.flip = b;
		CHANGE_VF("flip", b, QVariant());
	}
}

void TCore::toggleMirror() {
	qDebug("TCore::toggleMirror");
	toggleMirror(!mset.mirror);
}

void TCore::toggleMirror(bool b) {
	qDebug("TCore::toggleMirror: %d", b);
	if (mset.mirror != b) {
		mset.mirror = b;
		CHANGE_VF("mirror", b, QVariant());
	}
}

void TCore::toggleAutophase() {
	toggleAutophase(!mset.phase_filter);
}

void TCore::toggleAutophase(bool b) {
	qDebug("TCore::toggleAutophase: %d", b);
	if (b != mset.phase_filter) {
		mset.phase_filter = b;
		CHANGE_VF("phase", b, "A");
	}
}

void TCore::toggleDeblock() {
	toggleDeblock(!mset.deblock_filter);
}

void TCore::toggleDeblock(bool b) {
	qDebug("TCore::toggleDeblock: %d", b);
	if (b != mset.deblock_filter) {
		mset.deblock_filter = b;
		CHANGE_VF("deblock", b, pref->filters.item("deblock").options());
	}
}

void TCore::toggleDering() {
	toggleDering(!mset.dering_filter);
}

void TCore::toggleDering(bool b) {
	qDebug("TCore::toggleDering: %d", b);
	if (b != mset.dering_filter) {
		mset.dering_filter = b;
		CHANGE_VF("dering", b, QVariant());
	}
}

void TCore::toggleGradfun() {
	toggleGradfun(!mset.gradfun_filter);
}

void TCore::toggleGradfun(bool b) {
	qDebug("TCore::toggleGradfun: %d", b);
	if (b != mset.gradfun_filter) {
		mset.gradfun_filter = b;
		CHANGE_VF("gradfun", b, pref->filters.item("gradfun").options());
	}
}

void TCore::toggleNoise() {
	toggleNoise(!mset.noise_filter);
}

void TCore::toggleNoise(bool b) {
	qDebug("TCore::toggleNoise: %d", b);
	if (b != mset.noise_filter) {
		mset.noise_filter = b;
		CHANGE_VF("noise", b, QVariant());
	}
}

void TCore::togglePostprocessing() {
	togglePostprocessing(!mset.postprocessing_filter);
}

void TCore::togglePostprocessing(bool b) {
	qDebug("TCore::togglePostprocessing: %d", b);
	if (b != mset.postprocessing_filter) {
		mset.postprocessing_filter = b;
		CHANGE_VF("postprocessing", b, QVariant());
	}
}

void TCore::changeDenoise(int id) {
	qDebug("TCore::changeDenoise: %d", id);
	if (id != mset.current_denoiser) {
		if (isMPlayer()) {
			mset.current_denoiser = id;
			restartPlay();
		} else {
			// MPV
			QString dsoft = pref->filters.item("denoise_soft").options();
			QString dnormal = pref->filters.item("denoise_normal").options();
			// Remove previous filter
			switch (mset.current_denoiser) {
				case TMediaSettings::DenoiseSoft: proc->changeVF("hqdn3d", false, dsoft); break;
				case TMediaSettings::DenoiseNormal: proc->changeVF("hqdn3d", false, dnormal); break;
			}
			// New filter
			mset.current_denoiser = id;
			switch (mset.current_denoiser) {
				case TMediaSettings::DenoiseSoft: proc->changeVF("hqdn3d", true, dsoft); break;
				case TMediaSettings::DenoiseNormal: proc->changeVF("hqdn3d", true, dnormal); break;
			}
		}
	}
}

void TCore::changeUnsharp(int id) {
	qDebug("TCore::changeUnsharp: %d", id);
	if (id != mset.current_unsharp) {
		if (isMPlayer()) {
			mset.current_unsharp = id;
			restartPlay();
		} else {
			// MPV
			// Remove previous filter
			switch (mset.current_unsharp) {
				// Current is blur
				case 1: proc->changeVF("blur", false); break;
				// Current if sharpen
				case 2: proc->changeVF("sharpen", false); break;
			}
			// New filter
			mset.current_unsharp = id;
			switch (mset.current_unsharp) {
				case 1: proc->changeVF("blur", true); break;
				case 2: proc->changeVF("sharpen", true); break;
			}
		}
	}
}

void TCore::changeUpscale(bool b) {
	qDebug("TCore::changeUpscale: %d", b);
	if (mset.upscaling_filter != b) {
		mset.upscaling_filter = b;
		int width = TDesktop::size(playerwindow).width();
		CHANGE_VF("scale", b, QString::number(width) + ":-2");
	}
}

void TCore::changeStereo3d(const QString & in, const QString & out) {
	qDebug("TCore::changeStereo3d: in: %s out: %s", in.toUtf8().constData(), out.toUtf8().constData());

	if ((mset.stereo3d_in != in) || (mset.stereo3d_out != out)) {
		if (isMPlayer()) {
			mset.stereo3d_in = in;
			mset.stereo3d_out = out;
			restartPlay();
		} else {
			// Remove previous filter
			if (mset.stereo3d_in != "none" && !mset.stereo3d_out.isEmpty()) {
				proc->changeStereo3DFilter(false, mset.stereo3d_in, mset.stereo3d_out);
			}

			// New filter
			mset.stereo3d_in = in;
			mset.stereo3d_out = out;
			if (mset.stereo3d_in != "none" && !mset.stereo3d_out.isEmpty()) {
				proc->changeStereo3DFilter(true, mset.stereo3d_in, mset.stereo3d_out);
			}
		}
	}
}

void TCore::setBrightness(int value) {
	qDebug("TCore::setBrightness: %d", value);

	if (value > 100) value = 100;
	if (value < -100) value = -100;

	if (value != mset.brightness) {
		proc->setBrightness(value);
		mset.brightness = value;
		displayMessage(tr("Brightness: %1").arg(value));
		emit videoEqualizerNeedsUpdate();
	}
}


void TCore::setContrast(int value) {
	qDebug("TCore::setContrast: %d", value);

	if (value > 100) value = 100;
	if (value < -100) value = -100;

	if (value != mset.contrast) {
		proc->setContrast(value);
		mset.contrast = value;
		displayMessage(tr("Contrast: %1").arg(value));
		emit videoEqualizerNeedsUpdate();
	}
}

void TCore::setGamma(int value) {
	qDebug("TCore::setGamma: %d", value);

	if (value > 100) value = 100;
	if (value < -100) value = -100;

	if (value != mset.gamma) {
		proc->setGamma(value);
		mset.gamma= value;
		displayMessage(tr("Gamma: %1").arg(value));
		emit videoEqualizerNeedsUpdate();
	}
}

void TCore::setHue(int value) {
	qDebug("TCore::setHue: %d", value);

	if (value > 100) value = 100;
	if (value < -100) value = -100;

	if (value != mset.hue) {
		proc->setHue(value);
		mset.hue = value;
		displayMessage(tr("Hue: %1").arg(value));
		emit videoEqualizerNeedsUpdate();
	}
}

void TCore::setSaturation(int value) {
	qDebug("TCore::setSaturation: %d", value);

	if (value > 100) value = 100;
	if (value < -100) value = -100;

	if (value != mset.saturation) {
		proc->setSaturation(value);
		mset.saturation = value;
		displayMessage(tr("Saturation: %1").arg(value));
		emit videoEqualizerNeedsUpdate();
	}
}

void TCore::incBrightness() {
	setBrightness(mset.brightness + pref->min_step);
}

void TCore::decBrightness() {
	setBrightness(mset.brightness - pref->min_step);
}

void TCore::incContrast() {
	setContrast(mset.contrast + pref->min_step);
}

void TCore::decContrast() {
	setContrast(mset.contrast - pref->min_step);
}

void TCore::incGamma() {
	setGamma(mset.gamma + pref->min_step);
}

void TCore::decGamma() {
	setGamma(mset.gamma - pref->min_step);
}

void TCore::incHue() {
	setHue(mset.hue + pref->min_step);
}

void TCore::decHue() {
	setHue(mset.hue - pref->min_step);
}

void TCore::incSaturation() {
	setSaturation(mset.saturation + pref->min_step);
}

void TCore::decSaturation() {
	setSaturation(mset.saturation - pref->min_step);
}

void TCore::setSpeed(double value) {
	qDebug("TCore::setSpeed: %f", value);

	if (value < 0.10) value = 0.10;
	if (value > 100) value = 100;

	mset.speed = value;
	proc->setSpeed(value);

	displayMessage(tr("Speed: %1").arg(value));
}

void TCore::incSpeed10() {
	qDebug("TCore::incSpeed10");
	setSpeed((double) mset.speed + 0.1);
}

void TCore::decSpeed10() {
	qDebug("TCore::decSpeed10");
	setSpeed((double) mset.speed - 0.1);
}

void TCore::incSpeed4() {
	qDebug("TCore::incSpeed4");
	setSpeed((double) mset.speed + 0.04);
}

void TCore::decSpeed4() {
	qDebug("TCore::decSpeed4");
	setSpeed((double) mset.speed - 0.04);
}

void TCore::incSpeed1() {
	qDebug("TCore::incSpeed1");
	setSpeed((double) mset.speed + 0.01);
}

void TCore::decSpeed1() {
	qDebug("TCore::decSpeed1");
	setSpeed((double) mset.speed - 0.01);
}

void TCore::doubleSpeed() {
	qDebug("TCore::doubleSpeed");
	setSpeed((double) mset.speed * 2);
}

void TCore::halveSpeed() {
	qDebug("TCore::halveSpeed");
	setSpeed((double) mset.speed / 2);
}

void TCore::normalSpeed() {
	setSpeed(1);
}

int TCore::adjustVolume(int volume) {

	if (pref->use_soft_vol) {
		int max = pref->softvol_max;
		if (max < 100) max = 100;
		return volume * max / 100;
	}

	return volume;
}

int TCore::getVolume() {
	return pref->global_volume ? pref->volume : mset.volume;
}

void TCore::setVolume(int volume, bool force) {
	qDebug("TCore::setVolume: %d", volume);

	if (volume > 100) volume = 100;
	if (volume < 0) volume = 0;
	if (!force && volume == getVolume())
		return;

	if (isMPV()) {
		if (proc->isRunning()) {
			int vol = adjustVolume(volume);
			proc->setVolume(vol);
		}
	} else {
		// MPlayer
		if (state() == Paused) {
			// Change volume later, after quiting pause
			change_volume_after_unpause = true;
		} else if (proc->isRunning()) {
			proc->setVolume(volume);
		}
	}

	bool mute_changed;
	if (pref->global_volume) {
		pref->volume = volume;
		mute_changed = pref->mute;
		pref->mute = false;
	} else {
		mset.volume = volume;
		mute_changed = mset.mute;
		mset.mute = false;
	}

	displayMessage(tr("Volume: %1").arg(volume));
	if (mute_changed)
		emit muteChanged(false);
	emit volumeChanged(volume);
}

bool TCore::getMute() {
	return pref->global_volume ? pref->mute : mset.mute;
}

void TCore::mute(bool b) {
	qDebug("TCore::mute: %d", b);

	proc->mute(b);
	if (pref->global_volume) {
		pref->mute = b;
	} else {
		mset.mute = b;
	}
	emit muteChanged(b);
}

void TCore::incVolume() {
	qDebug("TCore::incVolume");
	setVolume(getVolume() + pref->min_step);
}

void TCore::decVolume() {
	qDebug("TCore::incVolume");
	setVolume(getVolume() - pref->min_step);
}

void TCore::setSubDelay(int delay) {
	qDebug("TCore::setSubDelay: %d", delay);

	mset.sub_delay = delay;
	proc->setSubDelay((double) mset.sub_delay/1000);
	displayMessage(tr("Subtitle delay: %1 ms").arg(delay));
}

void TCore::incSubDelay() {
	qDebug("TCore::incSubDelay");
	setSubDelay(mset.sub_delay + 100);
}

void TCore::decSubDelay() {
	qDebug("TCore::decSubDelay");
	setSubDelay(mset.sub_delay - 100);
}

void TCore::setAudioDelay(int delay) {
	qDebug("TCore::setAudioDelay: %d", delay);
	mset.audio_delay = delay;
	proc->setAudioDelay((double) mset.audio_delay/1000);
	displayMessage(tr("Audio delay: %1 ms").arg(delay));
}

void TCore::incAudioDelay() {
	qDebug("TCore::incAudioDelay");
	setAudioDelay(mset.audio_delay + 100);
}

void TCore::decAudioDelay() {
	qDebug("TCore::decAudioDelay");
	setAudioDelay(mset.audio_delay - 100);
}

void TCore::incSubPos() {
	qDebug("TCore::incSubPos");

	mset.sub_pos++;
	if (mset.sub_pos > 100) mset.sub_pos = 100;
	proc->setSubPos(mset.sub_pos);
}

void TCore::decSubPos() {
	qDebug("TCore::decSubPos");

	mset.sub_pos--;
	if (mset.sub_pos < 0) mset.sub_pos = 0;
	proc->setSubPos(mset.sub_pos);
}

void TCore::changeSubScale(double value) {
	qDebug("TCore::changeSubScale: %f", value);

	if (value < 0) value = 0;

	if (pref->use_ass_subtitles) {
		if (value != mset.sub_scale_ass) {
			mset.sub_scale_ass = value;
			proc->setSubScale(mset.sub_scale_ass);
		}
	} else if (isMPV()) {
		if (value != mset.sub_scale_mpv) {
			mset.sub_scale_mpv = value;
			proc->setSubScale(value);
		}
	} else if (value != mset.sub_scale) {
		mset.sub_scale = value;
		proc->setSubScale(value);
	}

	displayMessage(tr("Font scale: %1").arg(value));
}

void TCore::incSubScale() {
	double step = 0.20;

	if (pref->use_ass_subtitles) {
		changeSubScale(mset.sub_scale_ass + step);
	} else if (isMPV()) {
		changeSubScale(mset.sub_scale_mpv + step);
	} else {
		changeSubScale(mset.sub_scale + step);
	}
}

void TCore::decSubScale() {
	double step = 0.20;

	if (pref->use_ass_subtitles) {
		changeSubScale(mset.sub_scale_ass - step);
	} else if (isMPV()) {
		changeSubScale(mset.sub_scale_mpv - step);
	} else {
		changeSubScale(mset.sub_scale - step);
	}
}

void TCore::changeOSDScale(double value) {
	qDebug("TCore::changeOSDScale: %f", value);

	if (value < 0) value = 0;

	if (isMPlayer()) {
		if (value != pref->subfont_osd_scale) {
			pref->subfont_osd_scale = value;
			if (proc->isRunning())
				restartPlay();
		}
	} else {
		if (value != pref->osd_scale) {
			pref->osd_scale = value;
			if (proc->isRunning())
				proc->setOSDScale(pref->osd_scale);
		}
	}
}

void TCore::incOSDScale() {
	if (isMPlayer()) {
		changeOSDScale(pref->subfont_osd_scale + 1);
	} else {
		changeOSDScale(pref->osd_scale + 0.10);
	}
}

void TCore::decOSDScale() {
	if (isMPlayer()) {
		changeOSDScale(pref->subfont_osd_scale - 1);
	} else {
		changeOSDScale(pref->osd_scale - 0.10);
	}
}

void TCore::incSubStep() {
	qDebug("TCore::incSubStep");
	proc->setSubStep(+1);
}

void TCore::decSubStep() {
	qDebug("TCore::decSubStep");
	proc->setSubStep(-1);
}

void TCore::changeExternalSubFPS(int fps_id) {
	qDebug("TCore::setExternalSubFPS: %d", fps_id);

	mset.external_subtitles_fps = fps_id;
	if (haveExternalSubs()) {
		restartPlay();
	}
}

// Audio equalizer functions
void TCore::setAudioEqualizer(const TAudioEqualizerList& values, bool restart) {

	if (pref->global_audio_equalizer) {
		pref->audio_equalizer = values;
	} else {
		mset.audio_equalizer = values;
	}

	if (restart) {
		restartPlay();
	} else {
		proc->setAudioEqualizer(Helper::equalizerListToString(values));
	}
}

void TCore::setAudioEq(int eq, int value) {
	qDebug("TCore::setAudioEq: eq %d value %d", eq, value);

	if (pref->global_audio_equalizer) {
		pref->audio_equalizer[eq] = value;
		setAudioEqualizer(pref->audio_equalizer);
	} else {
		mset.audio_equalizer[eq] = value;
		setAudioEqualizer(mset.audio_equalizer);
	}
}

void TCore::setAudioEq0(int value) {
	setAudioEq(0, value);
}

void TCore::setAudioEq1(int value) {
	setAudioEq(1, value);
}

void TCore::setAudioEq2(int value) {
	setAudioEq(2, value);
}

void TCore::setAudioEq3(int value) {
	setAudioEq(3, value);
}

void TCore::setAudioEq4(int value) {
	setAudioEq(4, value);
}

void TCore::setAudioEq5(int value) {
	setAudioEq(5, value);
}

void TCore::setAudioEq6(int value) {
	setAudioEq(6, value);
}

void TCore::setAudioEq7(int value) {
	setAudioEq(7, value);
}

void TCore::setAudioEq8(int value) {
	setAudioEq(8, value);
}

void TCore::setAudioEq9(int value) {
	setAudioEq(9, value);
}

void TCore::gotCurrentSec(double sec) {

	mset.current_sec = sec;

	// Update GUI once per second
	static double last_second = -11.0;
	double f = floor(sec);
	if (f == last_second)
		return;
	last_second = f;

	// Let the world know what a beautiful time it is
	emit showTime(sec);

	// Emit posChanged:
	int pos = 0;
	if (mset.current_sec > 0 && mdat.duration > 0.1) {
		pos = qRound((mset.current_sec * pos_max) / mdat.duration);
		if (pos >= pos_max) pos = pos_max - 1;
	}
	emit positionChanged(pos);

	// Check chapter
	if (mdat.chapters.count() <= 0 || mset.playing_single_track) {
		return;
	}
	int chapter_id = mdat.chapters.getSelectedID();
	if (chapter_id >= 0) {
		Maps::TChapterData& chapter = mdat.chapters[chapter_id];
		if (sec >= chapter.getStart()) {
			if (sec < chapter.getEnd()) {
				// Chapter is current
				return;
			}
			if (chapter.getEnd() == -1) {
				// No end set
				if (chapter_id - mdat.chapters.firstID() + 1
					>= mdat.chapters.count()) {
					// chapter is last and current
					return;
				}
				if (sec < mdat.chapters[chapter_id + 1].getStart()) {
					// Chapter is current
					return;
				}
			}
		}
	}

	int new_chapter_id = mdat.chapters.idForTime(sec);
	if (new_chapter_id != chapter_id) {
		mdat.chapters.setSelectedID(new_chapter_id);
		qDebug("TCore:gotCurrentSec: emit chapterChanged(%d)", new_chapter_id);
		emit chapterChanged(new_chapter_id);
	}
}

void TCore::gotPause() {
	setState(Paused);
}

void TCore::changeDeinterlace(int ID) {
	qDebug("TCore::changeDeinterlace: %d", ID);

	if (ID != mset.current_deinterlacer) {
		if (isMPlayer()) {
			mset.current_deinterlacer = ID;
			restartPlay();
		} else {
			// MPV
			// Remove previous filter
			switch (mset.current_deinterlacer) {
				case TMediaSettings::L5:		proc->changeVF("l5", false); break;
				case TMediaSettings::Yadif:		proc->changeVF("yadif", false); break;
				case TMediaSettings::LB:		proc->changeVF("lb", false); break;
				case TMediaSettings::Yadif_1:	proc->changeVF("yadif", false, "1"); break;
				case TMediaSettings::Kerndeint:	proc->changeVF("kerndeint", false, "5"); break;
			}
			mset.current_deinterlacer = ID;
			// New filter
			switch (mset.current_deinterlacer) {
				case TMediaSettings::L5:		proc->changeVF("l5", true); break;
				case TMediaSettings::Yadif:		proc->changeVF("yadif", true); break;
				case TMediaSettings::LB:		proc->changeVF("lb", true); break;
				case TMediaSettings::Yadif_1:	proc->changeVF("yadif", true, "1"); break;
				case TMediaSettings::Kerndeint:	proc->changeVF("kerndeint", true, "5"); break;
			}
		}
	}
}

void TCore::changeVideoTrack(int id) {
	qDebug("TCore::changeVideoTrack: id: %d", id);

	// TODO: restore fast change after overwriting video out fixed
	if (id != mset.current_video_id) {
		mset.current_video_id = id;
		if (id >= 0 && id != mdat.videos.getSelectedID()) {
			restartPlay();
		}
	}
}

void TCore::nextVideoTrack() {
	qDebug("TCore::nextVideoTrack");

	changeVideoTrack(mdat.videos.nextID(mdat.videos.getSelectedID()));
}

void TCore::changeAudioTrack(int id) {
	qDebug("TCore::changeAudio: id: %d", id);

	if (id != mset.current_audio_id) {
		mset.current_audio_id = id;
		if (id >= 0 && id != mdat.audios.getSelectedID()) {
			proc->setAudio(id);
			mdat.audios.setSelectedID(id);
			emit audioTrackChanged(id);

			// Workaround for a mplayer problem in windows,
			// volume is too loud after changing audio.

			// Workaround too for a mplayer problem in linux,
			// the volume is reduced if using -softvol-max.

			if (isMPlayer()) {
				if (pref->mplayer_additional_options.contains("-volume")) {
					qDebug("TCore::changeAudio: don't set volume since -volume is used");
				} else if (pref->global_volume) {
					setVolume(pref->volume, true);
					if (pref->mute) mute(true);
				} else {
					setVolume(mset.volume, true);
					if (mset.mute) mute(true); // if muted, mute again
				}
			}
		}
	}
}

void TCore::nextAudioTrack() {
	qDebug("TCore::nextAudioTrack");

	changeAudioTrack(mdat.audios.nextID(mdat.audios.getSelectedID()));
}

// Note: changeSubtitle is by index, not ID
void TCore::changeSubtitle(int idx, bool selected_by_user) {
	qDebug("TCore::changeSubtitle: idx %d", idx);

	if (selected_by_user)
		mset.current_sub_set_by_user = true;

	if (idx >= 0 && idx < mdat.subs.count()) {
		mset.current_sub_idx = idx;
		SubData sub = mdat.subs.itemAt(idx);
		if (sub.ID() != mdat.subs.selectedID()
			|| sub.type() != mdat.subs.selectedType()) {
			proc->setSubtitle(sub.type(), sub.ID());
		}
	} else {
		mset.current_sub_idx = TMediaSettings::SubNone;
		if (mdat.subs.selectedID() >= 0) {
			proc->disableSubtitles();
		}
	}
}

void TCore::nextSubtitle() {
	qDebug("TCore::nextSubtitle");

	changeSubtitle(mdat.subs.nextID());
}

#ifdef MPV_SUPPORT
void TCore::changeSecondarySubtitle(int idx) {
	// MPV only
	qDebug("TCore::changeSecondarySubtitle: idx %d", idx);

	if (idx >= 0 && idx < mdat.subs.count()) {
		mset.current_secondary_sub_idx = idx;
		int id = mdat.subs.itemAt(idx).ID();
		if (id != mdat.subs.selectedSecondaryID()) {
			proc->setSecondarySubtitle(id);
		}
	} else {
		mset.current_secondary_sub_idx = TMediaSettings::SubNone;
		if (mdat.subs.selectedSecondaryID() >= 0) {
			proc->disableSecondarySubtitles();
		}
	}
}
#endif

void TCore::changeTitleLeaveMenu() {

	if (mdat.title_is_menu) {
		if (menus_selected >= 2) {
			qDebug("TCore::changeTitleLeaveMenu: failed to leave menu, giving up");
			block_dvd_nav = false;
			return;
		}
		qDebug("TCore::changeTitleLeaveMenu: still on menu, sending select");
		dvdnavSelect();
		menus_selected++;
		QTimer::singleShot(500, this, SLOT(changeTitleLeaveMenu()));
		return;
	}

	qDebug("TCore::changeTitleLeaveMenu: left menu, setting title");

	proc->setTitle(title_to_select);
	block_dvd_nav = false;
}

void TCore::changeTitle(int title) {
	qDebug("TCore::changeTitle: title %d", title);

	if (proc->isRunning()) {
		// Handle CDs with the chapter commands
		if (TMediaData::isCD(mdat.detected_type)) {
			changeChapter(title - mdat.titles.firstID() + mdat.chapters.firstID());
			return;
		}
		// Handle DVDNAV with title command
		if (mdat.detected_type == TMediaData::TYPE_DVDNAV && cache_size == 0) {
			if (mdat.title_is_menu && mdat.duration <= 0) {
				// Changing title on a menu does not work :(
				// Risc pressing menus you don't want to press, like settings...
				qDebug("TCore::changeTitle: trying to leave menu");
				dvdnavSelect();
				block_dvd_nav = true;
				menus_selected = 1;
				title_to_select = title;
				QTimer::singleShot(1000, this, SLOT(changeTitleLeaveMenu()));
			} else {
				qDebug("TCore::changeTitle: switching title through proc");
				proc->setTitle(title);
			}
			return;
		}
	}

	// Start/restart
	TDiscData disc = TDiscName::split(mdat.filename);
	disc.title = title;
	openDisc(disc, false);
}

void TCore::changeChapter(int id) {
	qDebug("TCore::changeChapter: ID: %d", id);

	if (id >= mdat.chapters.firstID()) {
		proc->setChapter(id);
	}
}

void TCore::prevChapter() {
	qDebug("TCore::prevChapter");

	// Can use mplayer: seek_chapter -1 mpv: add chapter -1
	int id = mdat.chapters.idForTime(mset.current_sec, false);
	changeChapter(mdat.chapters.previousID(id));
}

void TCore::nextChapter() {
	qDebug("TCore::nextChapter");

	// Can use mplayer: seek_chapter 1 mpv: add chapter 1
	int id = mdat.chapters.idForTime(mset.current_sec, false);
	changeChapter(mdat.chapters.nextID(id));
}

void TCore::changeAngle(int ID) {
	qDebug("TCore::changeAngle: ID: %d", ID);

	if (ID != mset.current_angle_id) {
		mset.current_angle_id = ID;
		restartPlay();
	}
}

#if PROGRAM_SWITCH
void TCore::changeProgram(int ID) {
	qDebug("TCore::changeProgram: %d", ID);

	if (ID != mset.current_program_id) {
		mset.current_program_id = ID;
		proc->setTSProgram(ID);

		/*
		mset.current_video_id = TMediaSettings::NoneSelected;
		mset.current_audio_id = TMediaSettings::NoneSelected;

		*/
	}
}

void TCore::nextProgram() {
	qDebug("TCore::nextProgram");
	// Not implemented yet
}
#endif

void TCore::changeAspectRatio(int id) {
	qDebug("TCore::changeAspectRatio: %d", id);

	mset.aspect_ratio_id = id;
	double asp = mset.aspectToNum((TMediaSettings::Aspect) id);

	// Set aspect video window, don't update video window
	playerwindow->setAspect(asp, false);
	// Resize with new aspect, normally updates video window
	emit needResize(mset.win_width, mset.win_height);
	// Adjust video window if resize canceled
	playerwindow->updateVideoWindow();

	emit aspectRatioChanged(id);

	QString asp_name = TMediaSettings::aspectToString((TMediaSettings::Aspect) mset.aspect_ratio_id);
	displayMessage(tr("Aspect ratio: %1").arg(asp_name));
}

void TCore::nextAspectRatio() {

	// Ordered list
	QList<int> s;
	s << TMediaSettings::AspectNone 
	  << TMediaSettings::AspectAuto
	  << TMediaSettings::Aspect11	// 1
	  << TMediaSettings::Aspect54	// 1.25
	  << TMediaSettings::Aspect43	// 1.33
	  << TMediaSettings::Aspect118	// 1.37
	  << TMediaSettings::Aspect1410	// 1.4
	  << TMediaSettings::Aspect32	// 1.5
	  << TMediaSettings::Aspect149	// 1.55
	  << TMediaSettings::Aspect1610	// 1.6
	  << TMediaSettings::Aspect169	// 1.77
	  << TMediaSettings::Aspect235;	// 2.35

	int i = s.indexOf(mset.aspect_ratio_id) + 1;
	if (i >= s.count())
		i = 0;
	int new_aspect_id = s[i];

	changeAspectRatio(new_aspect_id);
}

void TCore::nextWheelFunction() {
	int a = pref->wheel_function;

	bool done = false;
	if(((int) pref->wheel_function_cycle)==0)
		return;
	while(!done){
		// get next a

		a = a*2;
		if(a==32)
			a = 2;
		// See if we are done
		if (pref->wheel_function_cycle.testFlag((TPreferences::WheelFunction)a))
			done = true;
	}
	pref->wheel_function = a;
	QString m = "";
	switch(a){
	case TPreferences::Seeking:
		m = tr("Mouse wheel seeks now");
		break;
	case TPreferences::Volume:
		m = tr("Mouse wheel changes volume now");
		break;
	case TPreferences::Zoom:
		m = tr("Mouse wheel changes zoom level now");
		break;
	case TPreferences::ChangeSpeed:
		m = tr("Mouse wheel changes speed now");
		break;
	}
	displayMessage(m);
}

void TCore::changeLetterbox(bool b) {
	qDebug("TCore::changeLetterbox: %d", b);

	if (mset.add_letterbox != b) {
		mset.add_letterbox = b;
		CHANGE_VF("letterbox", b, TDesktop::aspectRatio(playerwindow));
	}
}

void TCore::changeLetterboxOnFullscreen(bool b) {
	qDebug("TCore::changeLetterboxOnFullscreen: %d", b);
	CHANGE_VF("letterbox", b, TDesktop::aspectRatio(playerwindow));
}

void TCore::changeOSDLevel(int level) {
	qDebug("TCore::changeOSDLevel: %d", level);

	pref->osd_level = (TPreferences::OSDLevel) level;
	if (proc->isRunning())
		proc->setOSDLevel(level);
	emit osdLevelChanged(level);
}

void TCore::nextOSDLevel() {

	int level;
	if (pref->osd_level >= TPreferences::SeekTimerTotal) {
		level = TPreferences::None;
	} else {
		level = pref->osd_level + 1;
	}
	changeOSDLevel(level);
}

void TCore::changeRotate(int r) {
	qDebug("TCore::changeRotate: %d", r);

	if (mset.rotate != r) {
		if (isMPlayer()) {
			mset.rotate = r;
			restartPlay();
		} else {
			// MPV
			// Remove previous filter
			switch (mset.rotate) {
				case TMediaSettings::Clockwise_flip: proc->changeVF("rotate", false, TMediaSettings::Clockwise_flip); break;
				case TMediaSettings::Clockwise: proc->changeVF("rotate", false, TMediaSettings::Clockwise); break;
				case TMediaSettings::Counterclockwise: proc->changeVF("rotate", false, TMediaSettings::Counterclockwise); break;
				case TMediaSettings::Counterclockwise_flip: proc->changeVF("rotate", false, TMediaSettings::Counterclockwise_flip); break;
			}
			mset.rotate = r;
			// New filter
			switch (mset.rotate) {
				case TMediaSettings::Clockwise_flip: proc->changeVF("rotate", true, TMediaSettings::Clockwise_flip); break;
				case TMediaSettings::Clockwise: proc->changeVF("rotate", true, TMediaSettings::Clockwise); break;
				case TMediaSettings::Counterclockwise: proc->changeVF("rotate", true, TMediaSettings::Counterclockwise); break;
				case TMediaSettings::Counterclockwise_flip: proc->changeVF("rotate", true, TMediaSettings::Counterclockwise_flip); break;
			}
		}
	}
}

#if USE_ADAPTER
void TCore::changeAdapter(int n) {
	qDebug("TCore::changeScreen: %d", n);

	if (pref->adapter != n) {
		pref->adapter = n;
		restartPlay();
	}
}
#endif

void TCore::changeSize(int percentage) {
	qDebug("TCore::changeSize: %d", percentage);

	if (!pref->fullscreen) {
		pref->size_factor = (double) percentage / 100;
		emit needResize(mset.win_width, mset.win_height);
		displayMessage(tr("Size %1%").arg(QString::number(percentage)));
	}
}

void TCore::getZoomFromPlayerWindow() {
	mset.zoom_factor = playerwindow->zoomNormalScreen();
	mset.zoom_factor_fullscreen = playerwindow->zoomFullScreen();
}

void TCore::getPanFromPlayerWindow() {
	mset.pan_offset = playerwindow->panNormalScreen();
	mset.pan_offset_fullscreen = playerwindow->panFullScreen();
}

void TCore::changeZoom(double factor) {
	qDebug("TCore::changeZoom: %f", factor);

	// Kept between min and max by playerwindow->setZoom()
	// Hence reread of factors
	playerwindow->setZoom(factor);
	getZoomFromPlayerWindow();

	displayMessage(tr("Zoom: %1").arg(playerwindow->zoom()));
}

void TCore::resetZoomAndPan() {
	qDebug("TCore::resetZoomAndPan");

	// Reset zoom and pan of video window
	playerwindow->resetZoomAndPan();
	// Reread modified settings
	getZoomFromPlayerWindow();
	getPanFromPlayerWindow();

	displayMessage(tr("Zoom and pan reset"));
}

void TCore::pan(int dx, int dy) {
	qDebug("TCore::pan");

	playerwindow->moveVideo(dx, dy);
	getPanFromPlayerWindow();

	QPoint current_pan = playerwindow->pan();
	displayMessage(tr("Pan (%1, %2)").arg(QString::number(current_pan.x())).arg(QString::number(current_pan.y())));
}


const int PAN_STEP = 8;

void TCore::panLeft() {

	pan(PAN_STEP, 0);
}

void TCore::panRight() {
	pan(-PAN_STEP, 0);
}

void TCore::panUp() {
	pan(0, PAN_STEP);
}

void TCore::panDown() {
	pan(0, -PAN_STEP);
}

void TCore::autoZoom() {
	double video_aspect = mset.aspectToNum((TMediaSettings::Aspect) mset.aspect_ratio_id);

	if (video_aspect <= 0) {
		QSize w = playerwindow->videoLayer()->size();
		video_aspect = (double) w.width() / w.height();
	}

	double screen_aspect = TDesktop::aspectRatio(playerwindow);
	double zoom_factor;

	if (video_aspect > screen_aspect)
		zoom_factor = video_aspect / screen_aspect;
	else
		zoom_factor = screen_aspect / video_aspect;

	qDebug("TCore::autoZoom: video_aspect: %f", video_aspect);
	qDebug("TCore::autoZoom: screen_aspect: %f", screen_aspect);
	qDebug("TCore::autoZoom: zoom_factor: %f", zoom_factor);

	changeZoom(zoom_factor);
}

void TCore::autoZoomFromLetterbox(double aspect) {
	qDebug("TCore::autoZoomFromLetterbox: %f", aspect);

	// Probably there's a much easy way to do this, but I'm not good with maths...

	QSize desktop =  TDesktop::size(playerwindow);

	double video_aspect = mset.aspectToNum((TMediaSettings::Aspect) mset.aspect_ratio_id);

	if (video_aspect <= 0) {
		QSize w = playerwindow->videoLayer()->size();
		video_aspect = (double) w.width() / w.height();
	}

	// Calculate size of the video in fullscreen
	QSize video;
	video.setHeight(desktop.height());;
	video.setWidth((int) (video.height() * video_aspect));
	if (video.width() > desktop.width()) {
		video.setWidth(desktop.width());;
		video.setHeight((int) (video.width() / video_aspect));
	}

	qDebug("TCore::autoZoomFromLetterbox: max. size of video: %d %d", video.width(), video.height());

	// Calculate the size of the actual video inside the letterbox
	QSize actual_video;
	actual_video.setWidth(video.width());
	actual_video.setHeight((int) (actual_video.width() / aspect));

	qDebug("TCore::autoZoomFromLetterbox: calculated size of actual video for aspect %f: %d %d", aspect, actual_video.width(), actual_video.height());

	double zoom_factor = (double) desktop.height() / actual_video.height();

	qDebug("TCore::autoZoomFromLetterbox: calculated zoom factor: %f", zoom_factor);
	changeZoom(zoom_factor);	
}

void TCore::autoZoomFor169() {
	autoZoomFromLetterbox((double) 16 / 9);
}

void TCore::autoZoomFor235() {
	autoZoomFromLetterbox(2.35);
}


const double ZOOM_STEP = 0.05;

void TCore::incZoom() {
	qDebug("TCore::incZoom");
	changeZoom(playerwindow->zoom() + ZOOM_STEP);
}

void TCore::decZoom() {
	qDebug("TCore::decZoom");
	changeZoom(playerwindow->zoom() - ZOOM_STEP);
}

void TCore::showFilenameOnOSD() {
	proc->showFilenameOnOSD();
}

void TCore::showTimeOnOSD() {
	proc->showTimeOnOSD();
}

void TCore::toggleDeinterlace() {
	qDebug("TCore::toggleDeinterlace");
	proc->toggleDeinterlace();
}

void TCore::changeUseCustomSubStyle(bool b) {
	qDebug("TCore::changeUseCustomSubStyle: %d", b);

	if (pref->enable_ass_styles != b) {
		pref->enable_ass_styles = b;
		if (proc->isRunning())
			restartPlay();
	}
}

void TCore::toggleForcedSubsOnly(bool b) {
	qDebug("TCore::toggleForcedSubsOnly: %d", b);

	if (pref->use_forced_subs_only != b) {
		pref->use_forced_subs_only = b;
		if (proc->isRunning())
			proc->setSubForcedOnly(b);
	}
}

void TCore::changeClosedCaptionChannel(int c) {
	qDebug("TCore::changeClosedCaptionChannel: %d", c);

	if (c != mset.closed_caption_channel) {
		mset.closed_caption_channel = c;
		if (proc->isRunning())
			restartPlay();
	}
}

/*
void TCore::nextClosedCaptionChannel() {
	int c = mset.closed_caption_channel;
	c++;
	if (c > 4) c = 0;
	changeClosedCaptionChannel(c);
}

void TCore::prevClosedCaptionChannel() {
	int c = mset.closed_caption_channel;
	c--;
	if (c < 0) c = 4;
	changeClosedCaptionChannel(c);
}
*/

// dvdnav buttons
void TCore::dvdnavUp() {
	qDebug("TCore::dvdnavUp");
	proc->discButtonPressed("up");
}

void TCore::dvdnavDown() {
	qDebug("TCore::dvdnavDown");
	proc->discButtonPressed("down");
}

void TCore::dvdnavLeft() {
	qDebug("TCore::dvdnavLeft");
	proc->discButtonPressed("left");
}

void TCore::dvdnavRight() {
	qDebug("TCore::dvdnavRight");
	proc->discButtonPressed("right");
}

void TCore::dvdnavMenu() {
	qDebug("TCore::dvdnavMenu");
	proc->discButtonPressed("menu");
}

void TCore::dvdnavPrev() {
	qDebug("TCore::dvdnavPrev");
	proc->discButtonPressed("prev");
}

// Slot called by action dvdnav_select
void TCore::dvdnavSelect() {
	qDebug("TCore::dvdnavSelect");

	if (mdat.detected_type == TMediaData::TYPE_DVDNAV) {
		if (_state == Paused) {
			play();
		}
		if (_state == Playing) {
			proc->discButtonPressed("select");
		}
	}
}

// Slot called by action dvdnav_mouse and Gui::TBase when left mouse clicked
void TCore::dvdnavMouse() {
	qDebug("TCore::dvdnavMouse");

	if (mdat.detected_type == TMediaData::TYPE_DVDNAV) {
		if (_state == Paused) {
			play();
		}
		if (_state == Playing) {
			// Give a last update on the mouse position
			QPoint pos = playerwindow->videoLayer()->mapFromGlobal(QCursor::pos());
			dvdnavUpdateMousePos(pos);
			// Click
			proc->discButtonPressed("mouse");
		}
	}
}

// Slot called by playerwindow to pass mouse move local to video
void TCore::dvdnavUpdateMousePos(QPoint pos) {

	if (mdat.detected_type == TMediaData::TYPE_DVDNAV && !block_dvd_nav) {
		// MPlayer won't act if paused. Play if menu not animated.
		if (_state == Paused && mdat.title_is_menu && mdat.duration == 0) {
			play();
		}
		if (_state == Playing) {
			proc->discSetMousePos(pos.x(), pos.y());
		}
	}
}

void TCore::displayMessage(QString text, int duration, int osd_level) {
	//qDebug("TCore::displayMessage");

	emit showMessage(text);
	displayTextOnOSD(text, duration, osd_level);
}

void TCore::displayScreenshotName(QString filename) {
	qDebug("TCore::displayScreenshotName: %s", filename.toUtf8().constData());

	QFileInfo fi(filename);
	QString text = tr("Screenshot saved as %1").arg(fi.fileName());
	displayMessage(text);
}

void TCore::displayUpdatingFontCache() {
	qDebug("TCore::displayUpdatingFontCache");
	emit showMessage(tr("Updating the font cache. This may take some seconds..."));
}

void TCore::displayBuffering() {
	emit showMessage(tr("Buffering..."));
}

void TCore::displayBufferingEnded() {
	emit showMessage(tr("Playing from %1").arg(Helper::formatTime(mset.current_sec)));
}

void TCore::gotVideoOutResolution(int w, int h) {
	qDebug("TCore::gotVideoOutResolution: %d x %d", w, h);

	// w x h is output resolution selected by player with aspect and filters applied
	mset.win_width = w;
	mset.win_height = h;
	playerwindow->setResolution(w, h);
	if (mset.aspect_ratio_id == TMediaSettings::AspectAuto) {
		// Set aspect to w/h. false = do not update video window.
		playerwindow->setAspect(mset.win_aspect(), false);
	}
	if (!we_are_restarting)
		emit videoOutResolutionChanged(w, h);

	// If resize is canceled adjust new video to old size
	playerwindow->updateVideoWindow();
}

void TCore::gotVO(QString vo) {
	qDebug("TCore::gotVO: '%s'", vo.toUtf8().data());

	// TODO: check saving and use vo/ao against sys changes
	if (pref->vo.isEmpty()) {
		qDebug("TCore::gotVO: saving vo");
		pref->vo = vo;
	}
}

void TCore::gotAO(QString ao) {
	qDebug("TCore::gotAO: '%s'", ao.toUtf8().data());

	// TODO: check saving and use vo/ao against sys changes
	if (pref->ao.isEmpty()) {
		qDebug("TCore::gotAO: saving ao");
		pref->ao = ao;
	}
}

void TCore::streamTitleChanged(QString title) {
	mdat.stream_title = title;
	emit mediaInfoChanged();
}

void TCore::streamTitleAndUrlChanged(QString title, QString url) {
	mdat.stream_title = title;
	mdat.stream_url = url;
	emit mediaInfoChanged();
}

void TCore::sendMediaInfo() {
	qDebug("TCore::sendMediaInfo");
	emit mediaPlaying(mdat.filename, mdat.displayName(pref->show_tag_in_window_title));
}

//!  Called when the state changes
void TCore::watchState(TCore::State state) {

	// Delayed volume change
	if (isMPlayer() && (state == Playing) && change_volume_after_unpause) {
		qDebug("TCore::watchState: delayed volume change");
		change_volume_after_unpause = false;
		proc->setVolume(getVolume());
	}
}

void TCore::checkIfVideoIsHD() {
	qDebug("TCore::checkIfVideoIsHD");

	// Check if the video is in HD and uses ffh264 codec.
	if ((mdat.video_codec=="ffh264") && (mset.win_height >= pref->HD_height)) {
		qDebug("TCore::checkIfVideoIsHD: video == ffh264 and height >= %d", pref->HD_height);
		if (!mset.is264andHD) {
			mset.is264andHD = true;
			if (pref->h264_skip_loop_filter == TPreferences::LoopDisabledOnHD) {
				qDebug("TCore::checkIfVideoIsHD: we're about to restart the video");
				restartPlay();
			}
		}
	} else {
		mset.is264andHD = false;
		// FIXME: if the video was previously marked as HD, and now it's not
		// then the video should restart too.
	}
}

bool TCore::setPreferredAudio() {
	qDebug("TCore::setPreferredAudio");

	mset.preferred_language_audio_set = true;

	if (!pref->audio_lang.isEmpty()) {
		int wanted_id = mdat.audios.findLangID(pref->audio_lang);
		if (wanted_id >= 0 && wanted_id != mdat.audios.getSelectedID()) {
			mset.current_audio_id = TMediaSettings::NoneSelected;
			changeAudioTrack(wanted_id);
			return true;
		}
	}

	qDebug("TCore::setPreferredAudio: no player overrides");
	return false;
}

void TCore::gotAudioTrackInfo() {
	qDebug("TCore::gotAudioTrackInfo");

	// First update the tracks, so a possible track change will arrive
	// on defined actions
	emit audioTrackInfoChanged();

	// Check if one of the audio tracks matches the preferred language.
	// If there is no audio selected, we have to wait for the audio to be
	// selected and set the preferred language in gotAudioTrackChanged()
	if (mdat.audios.getSelectedID() >= 0) {
		setPreferredAudio();
	} else {
		// Let gotAudioTrackChanged() have a look at it
		mset.preferred_language_audio_set = false;
	}

}

void TCore::gotAudioTrackChanged(int id) {
	qDebug("TCore::gotAudioTrackChanged: id %d", id);

	// Check if one of the audio tracks matches the preferred language.
	if (!mset.preferred_language_audio_set && mdat.audios.count() > 0) {
		if (setPreferredAudio()) {
			// audioTrackChanged() already emitted, so return
			return;
		}
	}

	emit audioTrackChanged(id);
}

void TCore::selectPreferredSub() {

	int wanted_sub_idx = pref->initial_subtitle_track - 1;
	wanted_sub_idx = mdat.subs.selectOne(pref->language, wanted_sub_idx);
	if (wanted_sub_idx >= 0) {
		qDebug("TCore::selectPreferredSub: selecting subtitle idx %d", wanted_sub_idx);
		changeSubtitle(wanted_sub_idx, false);
	} else {
		qDebug("TCore::selectPreferredSub: no player overrides");
	}
}

void TCore::gotSubtitleInfo() {
	qDebug("TCore::gotSubtitleInfo");

	// Need to set current_sub_idx, the subtitle action group checks on it.
	mset.current_sub_idx = mdat.subs.findSelectedIdx();
	emit subtitleInfoChanged();

	if (pref->autoload_sub && mdat.subs.count() > 0) {
		int wanted_sub_idx = pref->initial_subtitle_track - 1;
		wanted_sub_idx = mdat.subs.selectOne(pref->language, wanted_sub_idx);
		if (wanted_sub_idx >= 0) {
			if (isMPlayer()) {
				// mplayers selected sub will not yet be updated, que the update.
				qDebug("TCore::gotSubtitleInfo: posting update subtitle idx %d", wanted_sub_idx);
				QTimer::singleShot(500, this, SLOT(selectPreferredSub()));
			} else {
				qDebug("TCore::gotSubtitleInfo: selecting subtitle idx %d", wanted_sub_idx);
				changeSubtitle(wanted_sub_idx, false);
			}
			return;
		}
	}

	qDebug("TCore::gotSubtitleInfo: no player overrides");
}

// Called when player changed subtitle track
void TCore::gotSubtitleChanged() {
	qDebug("TCore::gotSubtitleChanged");

	// Need to set current_sub_idx, the subtitle group checks on it.
	int selected_idx = mdat.subs.findSelectedIdx();
	mset.current_sub_idx = selected_idx;

	emit subtitleTrackChanged(selected_idx);
}

#include "moc_core.cpp"
