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

#ifdef Q_OS_OS2
#include <QEventLoop>
#endif

#include <cmath>

#include "mplayerwindow.h"
#include "desktopinfo.h"
#include "helper.h"
#include "paths.h"
#include "preferences.h"
#include "global.h"
#include "config.h"
#include "mplayerversion.h"
#include "constants.h"
#include "colorutils.h"
#include "discname.h"
#include "extensions.h"
#include "filters.h"
#include "tvlist.h"

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef Q_OS_WIN
#include <windows.h> // To change app priority
#include <QSysInfo> // To get Windows version
#endif
#ifdef SCREENSAVER_OFF
#include "screensaver.h"
#endif
#endif

#ifndef NO_USE_INI_FILES
#include "filesettings.h"
#include "filesettingshash.h"
#include "tvsettings.h"
#endif

#ifdef YOUTUBE_SUPPORT
#include "retrieveyoutubeurl.h"
  #ifdef YT_USE_YTSIG
  #include "ytsig.h"
  #endif
#endif

using namespace Global;

Core::Core(MplayerWindow *mpw, QWidget* parent , int position_max)
	: QObject( parent ),
	  pos_max(position_max)
{
	qRegisterMetaType<Core::State>("Core::State");

	mplayerwindow = mpw;

	_state = Stopped;

	we_are_restarting = false;
	change_volume_after_unpause = false;

#if DVDNAV_SUPPORT
	dvdnav_title_is_menu = true; // Enabled by default for compatibility with previous versions of mplayer
#endif

#ifndef NO_USE_INI_FILES
	// Create file_settings
	file_settings = 0;
	changeFileSettingsMethod(pref->file_settings_method);

	// TV settings
	tv_settings = new TVSettings(Paths::iniPath());
#endif

	proc = PlayerProcess::createPlayerProcess(pref->mplayer_bin, &mdat);

	connect( proc, SIGNAL(error(QProcess::ProcessError)),
			 mplayerwindow, SLOT(playingStopped()) );

	connect( proc, SIGNAL(receivedVideoOutResolution(int,int)),
			 this, SLOT(gotVideoOutResolution(int,int)) );

	connect( proc, SIGNAL(receivedCurrentSec(double)),
             this, SLOT(changeCurrentSec(double)) );

	connect( proc, SIGNAL(receivedCurrentFrame(int)),
             this, SIGNAL(showFrame(int)) );

	connect( proc, SIGNAL(playerFullyLoaded()),
			 this, SLOT(playingStarted()), Qt::QueuedConnection );

	connect( proc, SIGNAL(receivedPause()),
			 this, SLOT(changePause()) );

	connect( proc, SIGNAL(processExited(bool)),
			 this, SLOT(processFinished(bool)));


	connect( proc, SIGNAL(lineAvailable(QString)),
             this, SIGNAL(logLineAvailable(QString)) );

	connect( proc, SIGNAL(receivedCacheMessage(QString)),
			 this, SLOT(displayMessage(QString)) );

	/*
	connect( proc, SIGNAL(receivedCacheMessage(QString)),
			 this, SIGNAL(buffering()));
	*/

	connect( proc, SIGNAL(receivedBuffering()),
			 this, SIGNAL(buffering()));

	connect( proc, SIGNAL(receivedCacheEmptyMessage(QString)),
			 this, SIGNAL(buffering()));

	connect( proc, SIGNAL(receivedCreatingIndex(QString)),
			 this, SLOT(displayMessage(QString)) );

	connect( proc, SIGNAL(receivedCreatingIndex(QString)),
			 this, SIGNAL(buffering()));

	connect( proc, SIGNAL(receivedConnectingToMessage(QString)),
			 this, SLOT(displayMessage(QString)) );

	connect( proc, SIGNAL(receivedConnectingToMessage(QString)),
			 this, SIGNAL(buffering()));

	connect( proc, SIGNAL(receivedResolvingMessage(QString)),
			 this, SLOT(displayMessage(QString)) );

	connect( proc, SIGNAL(receivedResolvingMessage(QString)),
			 this, SIGNAL(buffering()));

	connect( proc, SIGNAL(receivedScreenshot(QString)),
             this, SLOT(displayScreenshotName(QString)) );

	connect( proc, SIGNAL(receivedUpdatingFontCache()),
             this, SLOT(displayUpdatingFontCache()) );

	connect( proc, SIGNAL(receivedScanningFont(QString)),
			 this, SLOT(displayMessage(QString)) );

	connect( proc, SIGNAL(receivedVO(QString)),
             this, SLOT(gotVO(QString)) );

	connect( proc, SIGNAL(receivedAO(QString)),
             this, SLOT(gotAO(QString)) );

	connect( proc, SIGNAL(receivedEndOfFile()),
             this, SLOT(fileReachedEnd()), Qt::QueuedConnection );

	connect( proc, SIGNAL(receivedStreamTitle(QString)),
             this, SLOT(streamTitleChanged(QString)) );

	connect( proc, SIGNAL(receivedStreamTitleAndUrl(QString,QString)),
             this, SLOT(streamTitleAndUrlChanged(QString,QString)) );

	connect( proc, SIGNAL(failedToParseMplayerVersion(QString)),
             this, SIGNAL(failedToParseMplayerVersion(QString)) );

	connect( this, SIGNAL(mediaLoaded()), this, SLOT(checkIfVideoIsHD()), Qt::QueuedConnection );

	connect( proc, SIGNAL(videoTracksChanged()),
			 this, SLOT(updateVideoTracks()), Qt::QueuedConnection );

	connect( proc, SIGNAL(audioTracksChanged()),
			 this, SLOT(updateAudioTracks()), Qt::QueuedConnection );

	connect( proc, SIGNAL(subtitleTracksChanged()),
			 this, SLOT(updateSubtitleTracks()), Qt::QueuedConnection );

	connect( proc, SIGNAL(durationChanged(double)),
			 this, SIGNAL(newDuration(double)));

#if DVDNAV_SUPPORT
	connect( proc, SIGNAL(receivedDVDTitle(int)), 
             this, SLOT(dvdTitleChanged(int)), Qt::QueuedConnection );
	connect( proc, SIGNAL(receivedTitleIsMenu()),
             this, SLOT(dvdTitleIsMenu()) );
	connect( proc, SIGNAL(receivedTitleIsMovie()),
             this, SLOT(dvdTitleIsMovie()) );
#endif

	connect( proc, SIGNAL(receivedForbiddenText()), this, SIGNAL(receivedForbidden()) );

	connect( this, SIGNAL(stateChanged(Core::State)), 
	         this, SLOT(watchState(Core::State)) );

	connect( this, SIGNAL(mediaInfoChanged()), this, SLOT(sendMediaInfo()) );

	connect( proc, SIGNAL(error(QProcess::ProcessError)), 
             this, SIGNAL(mplayerFailed(QProcess::ProcessError)) );

	//pref->load();
	mset.reset();

	// Mplayerwindow
	connect( this, SIGNAL(aboutToStartPlaying()),
			 mplayerwindow, SLOT(aboutToStartPlaying()) );

#if DVDNAV_SUPPORT
	connect( mplayerwindow, SIGNAL(mouseMoved(QPoint)),
             this, SLOT(dvdnavUpdateMousePos(QPoint)) );
#endif

#if REPAINT_BACKGROUND_OPTION
	mplayerwindow->videoLayer()->setRepaintBackground(pref->repaint_video_background);
#endif
	mplayerwindow->setMonitorAspect( pref->monitor_aspect_double() );

#if  defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef SCREENSAVER_OFF
	// Windows or OS2 screensaver
	win_screensaver = new WinScreenSaver();
	connect( this, SIGNAL(aboutToStartPlaying()),
			 this, SLOT(disableScreensaver()) );
	connect( proc, SIGNAL(processExited(bool)),
			 this, SLOT(enableScreensaver()), Qt::QueuedConnection );
	connect( proc, SIGNAL(error(QProcess::ProcessError)),
			 this, SLOT(enableScreensaver()), Qt::QueuedConnection );
#endif
#endif

#ifdef YOUTUBE_SUPPORT
	yt = new RetrieveYoutubeUrl(this);
	yt->setUseHttpsMain(pref->yt_use_https_main);
	yt->setUseHttpsVi(pref->yt_use_https_vi);

	#ifdef YT_USE_SIG
	QSettings * sigset = new QSettings(Paths::configPath() + "/sig.ini", QSettings::IniFormat, this);
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

Core::~Core() {

	stopPlayer();
	proc->terminate();
	delete proc;

#ifndef NO_USE_INI_FILES
	delete file_settings;
	delete tv_settings;
#endif

#if  defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef SCREENSAVER_OFF
	delete win_screensaver;
#endif
#endif

#ifdef YOUTUBE_SUPPORT
	delete yt;
#endif
}

#ifndef NO_USE_INI_FILES 
void Core::changeFileSettingsMethod(QString method) {
	qDebug("Core::changeFileSettingsMethod: %s", method.toUtf8().constData());
	if (file_settings) delete file_settings;

	if (method.toLower() == "hash")
		file_settings = new FileSettingsHash(Paths::iniPath());
	else
		file_settings = new FileSettings(Paths::iniPath());
}
#endif

void Core::setState(State s) {
	if (s != _state) {
		_state = s;
		qDebug() << "Core::setState: set state to" << stateToString();
		qDebug() << "Core::setState: emit stateChanged()";
		emit stateChanged(_state);
	}
}

QString Core::stateToString() {
	if (state()==Playing) return "Playing";
	else
	if (state()==Stopped) return "Stopped";
	else
	if (state()==Paused) return "Paused";
	else
	return "Unknown";
}

// Public restart
void Core::restart() {
	qDebug("Core::restart");

	if (proc->isRunning()) {
		restartPlay();
	} else {
		qDebug("Core::restart: mplayer is not running");
	}
}

void Core::reload() {
	qDebug("Core::reload");

	stopPlayer();
	we_are_restarting = false;

	initPlaying();
}

#ifndef NO_USE_INI_FILES
void Core::saveMediaInfo() {
	qDebug("Core::saveMediaInfo");

	if (pref->dont_remember_media_settings) {
		qDebug("Core::saveMediaInfo: not saving settings, disabled by user");
		return;
	}

	if ( (mdat.selected_type == MediaData::TYPE_FILE) && (!mdat.filename.isEmpty()) ) {
		file_settings->saveSettingsFor(mdat.filename, mset, proc->player());
	}
	else
	if ( (mdat.selected_type == MediaData::TYPE_TV) && (!mdat.filename.isEmpty()) ) {
		tv_settings->saveSettingsFor(mdat.filename, mset, proc->player());
	}
}
#endif // NO_USE_INI_FILES

void Core::initializeMenus() {
	qDebug("Core::initializeMenus");

	emit menusNeedInitialize();
}


void Core::updateWidgets() {
	qDebug("Core::updateWidgets");

	emit widgetsNeedUpdate();
}


void Core::changeFullscreenMode(bool b) {
	proc->setFullscreen(b);
}

void Core::clearOSD(int level) {

	displayTextOnOSD("", 0, level);
}

void Core::displayTextOnOSD(QString text, int duration, int level) {
	//qDebug("Core::displayTextOnOSD: '%s'", text.toUtf8().constData());

	if (proc->isFullyStarted() && level <= pref->osd_level) {
		proc->showOSDText(text, duration, level);
	}
}

void Core::close() {
	qDebug("Core::close()");

	stopPlayer();

	we_are_restarting = false;

	// Save data of previous file:
#ifndef NO_USE_INI_FILES
	saveMediaInfo();
#endif

	mdat.reset();
}

void Core::openDisc(DiscData &disc, bool fast_open) {
	// Disc

	// Change title if disc already playing
	if (fast_open && disc.title > 0 && _state != Stopped) {
		bool current_url_valid;
		DiscData current_disc = DiscName::split(mdat.filename, &current_url_valid);
		if (current_url_valid && current_disc.device == disc.device) {
			// If it fails, it will call again with fast_open set to false
			qDebug("Core::openDisc: trying changeTitle(%d)", disc.title);
			changeTitle(disc.title);
			return;
		}
	}

	close();

	// Add devices from prev if none specified
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
		qWarning() << "Core::openDisc: could not access" << disc.device;
		// TODO:
		return;
	}

	// Clean filename and set selected type
	mdat.filename = DiscName::join(disc);
	mdat.selected_type = MediaData::stringToType(disc.protocol);

	// Clear settings
	mset.reset();
	mset.current_title_id = disc.title;
	// TODO: check seek from open

	initPlaying();
	return;

}

// Generic open, autodetect type
void Core::open(QString file, int seek, bool fast_open) {
	qDebug("Core::open: '%s'", file.toUtf8().data());

	if (file.startsWith("file:")) {
		file = QUrl(file).toLocalFile();
	}

	bool disc_url_valid;
	DiscData disc = DiscName::split(file, &disc_url_valid);
	if (disc_url_valid) {
		qDebug() << "Core::openDisc: * identified as" << disc.protocol;
		openDisc(disc, fast_open);
		return;
	}

	QFileInfo fi(file);
	if (fi.exists()) {
		file = fi.absoluteFilePath();

		Extensions e;
		QRegExp ext_sub(e.subtitles().forRegExp(), Qt::CaseInsensitive);
		if (ext_sub.indexIn(fi.suffix()) >= 0) {
			qDebug("Core::open: * identified as subtitle file");
			loadSub(file);
			return;
		}

		if (fi.isDir()) {
			qDebug("Core::open: * identified as a directory");
			qDebug("Core::open:   checking if it contains a dvd");
			if (Helper::directoryContainsDVD(file)) {
				qDebug("Core::open: * directory contains a dvd");
	#if DVDNAV_SUPPORT
				open(DiscName::joinDVD(file, pref->use_dvdnav), fast_open);
	#else
				open(DiscName::joinDVD(file, false), fast_open);
	#endif
			} else {
				qDebug("Core::open: * directory doesn't contain a dvd");
				qDebug("Core::open:   opening nothing");
			}
			return;
		}

		// Local file
		qDebug("Core::open: * identified as local file");
		openFile(file, seek);
		return;
	}

	// File does not exist
	if (file.toLower().startsWith("tv:") || file.toLower().startsWith("dvb:")) {
		qDebug("Core::open: * identified as TV");
		openTV(file);
	} else {
		qDebug("Core::open: * not identified, playing as stream");
		openStream(file);
	}
}


#ifdef YOUTUBE_SUPPORT
void Core::openYT(const QString & url) {
	qDebug("Core::openYT: %s", url.toUtf8().constData());
	openStream(url);
	yt->close();
}

void Core::connectingToYT(QString host) {
	emit showMessage( tr("Connecting to %1").arg(host) );
}

void Core::YTFailed(int /*error_number*/, QString /*error_str*/) {
	emit showMessage( tr("Unable to retrieve the Youtube page") );
}

void Core::YTNoVideoUrl() {
	emit showMessage( tr("Unable to locate the URL of the video") );
}
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef SCREENSAVER_OFF
void Core::enableScreensaver() {
	qDebug("Core::enableScreensaver");
	if (pref->turn_screensaver_off) {
		win_screensaver->enable();
	}
}

void Core::disableScreensaver() {
	qDebug("Core::disableScreensaver");
	if (pref->turn_screensaver_off) {
		win_screensaver->disable();
	}
}
#endif
#endif

void Core::setExternalSubs(const QString &filename) {

	mset.current_sub_idx = MediaSettings::NoneSelected;
	mset.sub.setID(MediaSettings::NoneSelected);
	mset.sub.setType(SubData::File);
	if (proc->isMPlayer()) {
		QFileInfo fi(filename);
		if (fi.suffix().toLower() == "idx") {
			mset.sub.setType(SubData::Vob);
		}
	}
	mset.sub.setFilename(filename);
}

void Core::loadSub(const QString & sub ) {
	qDebug("Core::loadSub");

	if ( (!sub.isEmpty()) && (QFile::exists(sub)) ) {
		setExternalSubs(sub);
		if (!pref->fast_load_sub
			|| mset.external_subtitles_fps != MediaSettings::SFPS_None) {
			restartPlay();
		} else {
			proc->setExternalSubtitleFile(sub);
		}
	} else {
		qWarning("Core::loadSub: file '%s' is not valid", sub.toUtf8().constData());
	}
}

void Core::unloadSub() {

	mset.current_sub_idx = MediaSettings::NoneSelected;
	mset.sub = SubData();

	restartPlay();
}

void Core::loadAudioFile(const QString & audiofile) {
	if (!audiofile.isEmpty()) {
		mset.external_audio = audiofile;
		restartPlay();
	}
}

void Core::unloadAudioFile() {
	if (!mset.external_audio.isEmpty()) {
		mset.external_audio = "";
		restartPlay();
	}
}

void Core::openTV(QString channel_id) {
	qDebug("Core::openTV: '%s'", channel_id.toUtf8().constData());

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
	mdat.selected_type = MediaData::TYPE_TV;

	mset.reset();

	// Set the default deinterlacer for TV
	mset.current_deinterlacer = pref->initial_tv_deinterlace;

#ifndef NO_USE_INI_FILES
	if (!pref->dont_remember_media_settings) {
		// Check if we already have info about this file
		if (tv_settings->existSettingsFor(channel_id)) {
			qDebug("Core::openTV: we have settings for this file!!!");

			// In this case we read info from config
			tv_settings->loadSettingsFor(channel_id, mset, proc->player());
			qDebug("Core::openTV: media settings read");
		}
	}
#endif

	initPlaying();
}

void Core::openStream(QString name) {
	qDebug("Core::openStream: '%s'", name.toUtf8().data());

#ifdef YOUTUBE_SUPPORT
	if (pref->enable_yt_support) {
		// Check if the stream is a youtube url
		QString yt_full_url = yt->fullUrl(name);
		if (!yt_full_url.isEmpty()) {
			qDebug("Core::openStream: youtube url detected: %s", yt_full_url.toLatin1().constData());
			name = yt_full_url;
			yt->setPreferredQuality( (RetrieveYoutubeUrl::Quality) pref->yt_quality );
			qDebug("Core::openStream: user_agent: '%s'", pref->yt_user_agent.toUtf8().constData());
			/*if (!pref->yt_user_agent.isEmpty()) yt->setUserAgent(pref->yt_user_agent); */
			yt->setUserAgent(pref->yt_user_agent);
			#ifdef YT_USE_YTSIG
			YTSig::setScriptFile( Paths::configPath() + "/yt.js" );
			#endif
			yt->fetchPage(name);
			return;
		}
	}
#endif

	close();
	mdat.filename = name;
	mdat.selected_type = MediaData::TYPE_STREAM;

	mset.reset();

	initPlaying();
}

void Core::openFile(QString filename, int seek) {
	qDebug("Core::openFile: '%s'", filename.toUtf8().data());

	close();

	mdat.filename = filename;
	mdat.selected_type = MediaData::TYPE_FILE;

	int old_volume = mset.volume;
	mset.reset();

#ifndef NO_USE_INI_FILES
	// Check if we already have info about this file
	if (pref->dont_remember_media_settings || !file_settings->existSettingsFor(filename)) {
		mset.volume = old_volume;
	} else {
		qDebug("Core::playNewFile: We have settings for this file!!!");

		file_settings->loadSettingsFor(filename, mset, proc->player());
		qDebug("Core::playNewFile: Media settings read");

		if (pref->dont_remember_time_pos) {
			mset.current_sec = 0;
			qDebug("Core::playNewFile: Time pos reset to 0");
		}
	}
#else
	// Recover volume
	mset.volume = old_volume;
#endif // NO_USE_INI_FILES

	// Apply settings to mplayerwindow
	mplayerwindow->set(
		mset.aspectToNum((MediaSettings::Aspect) mset.aspect_ratio_id),
		mset.zoom_factor, mset.zoom_factor_fullscreen,
		mset.pan_offset, mset.pan_offset_fullscreen);

	qDebug("Core::playNewFile: volume: %d, old_volume: %d", mset.volume, old_volume);
	initPlaying(seek);
}


void Core::restartPlay() {
	we_are_restarting = true;
	initPlaying();
}

void Core::initPlaying(int seek) {
	qDebug("Core::initPlaying");

	mplayerwindow->hideLogo();
	// Feedback and prevent potential artifacts waiting for redraw
	mplayerwindow->repaint();
	qDebug("Core::initPlaying: entered the black hole");

	if (proc->isRunning()) {
		stopPlayer();
	}

	int start_sec = (int) mset.current_sec;
	if (seek > -1) start_sec = seek;

#ifdef YOUTUBE_SUPPORT
	if (pref->enable_yt_support) {
		// Avoid to pass to mplayer the youtube page url
		if (mdat.selected_type == MediaData::TYPE_STREAM) {
			if (mdat.filename == yt->origUrl()) {
				mdat.filename = yt->latestPreferredUrl();
			}
		}
	}
#endif

	startPlayer( mdat.filename, start_sec );
}

// Called by newMediaPlaying
void Core::initAudioTracks() {
	qDebug("Core::initAudioTracks");

	// Check if one of the audio tracks matches the users preferred language.
	// TODO: This will disable audio when:
	// Track 0: sound track, no language, starting at 0
	// Track 1: speach, language english, starting at 5 min
	// Track 2: speach, language german, starting at 5 min
	// For now disabled
	/*
	if (mdat.audios.count() > 0 && !pref->audio_lang.isEmpty()) {
		int wanted_id = mdat.audios.findLangID(pref->audio_lang);
		if (wanted_id >= 0 && wanted_id != mdat.audios.selectedID()) {
			changeAudioTrack(wanted_id, false); // Don't allow restart
		}
	}
	*/
}

// Called by newMediaPlaying
void Core::initSubs() {
	qDebug("Core::initSubs");

	if (pref->autoload_sub) {
		// TODO: this is not reliable. Subs can contain all kind of things,
		// like graphics etc. Just selecting the first matching language is
		// asking for trouble. For now disabled.
		/*
		if (mdat.subs.count() > 0 && mdat.subs.selectedID() < 0) {
			// Nothing selected, check preferred language
			wanted_idx = mdat.subs.selectOne(pref->language,
											 pref->initial_subtitle_track - 1);
			changeSubtitleTrack(wanted_idx);
		}
		*/
	}
}

// This is reached when a new video has just started playing
void Core::newMediaPlaying() {
	qDebug("Core::newMediaPlaying");

	// Copy the demuxer
	mset.current_demuxer = mdat.demuxer;

	initAudioTracks();
	initSubs();

	mdat.initialized = true;
	mdat.list();
	mset.list();

	qDebug("Core::newMediaPlaying: emit mediaStartPlay()");
	emit mediaStartPlay();
}

// Slot called when queued signal playerFullyLoaded arrives.
void Core::playingStarted() {
	qDebug("Core::playingStarted");

	setState(Playing);

	if (we_are_restarting) {
		we_are_restarting = false;
	} else {
		newMediaPlaying();
	} 

	if (forced_titles.contains(mdat.filename)) {
		mdat.meta_data["NAME"] = forced_titles[mdat.filename];
	}

#ifdef YOUTUBE_SUPPORT
	if (pref->enable_yt_support) {
		// Change the real url with the youtube page url and set the title
		if (mdat.selected_type == MediaData::TYPE_STREAM) {
			if (mdat.filename == yt->latestPreferredUrl()) {
				mdat.filename = yt->origUrl();
				mdat.stream_title = yt->urlTitle();
			}
		}
	}
#endif

	if (pref->mplayer_additional_options.contains("-volume")) {
		qDebug("Core::playingStarted: don't set volume since -volume is used");
	} else {
		int vol = (pref->global_volume ? pref->volume : mset.volume);
		volumeChanged(vol);
		if (pref->mute) mute(true);
	}

	initSubs();

	we_are_restarting = false;

	// TODO:
#if 0
	// Hack to be sure that the equalizers are up to date
	emit videoEqualizerNeedsUpdate();
	emit audioEqualizerNeedsUpdate();
#endif

	// A-B marker
	emit ABMarkersChanged(mset.A_marker, mset.B_marker);

	qDebug("Core::playingStarted: emit mediaLoaded()");
	emit mediaLoaded();
	qDebug("Core::playingStarted: emit mediaInfoChanged()");
	emit mediaInfoChanged();

	initializeMenus();
	updateWidgets();

	qDebug("Core::playingStarted: done");
}

void Core::stop()
{
	qDebug() << "Core::stop: current state:" << stateToString();

	State prev_state = _state;
	stopPlayer();

	// if pressed stop twice, reset video to the beginning
	if ((prev_state == Stopped || pref->reset_stop) && mset.current_sec != 0) {
		qDebug("Core::stop: resetting current_sec %f to 0", mset.current_sec);
		gotCurrentSec(0);
	}

	emit mediaStoppedByUser();
}

void Core::play() {
	qDebug() << "Core::play: current state: " << stateToString();

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

void Core::pause() {
	qDebug() << "Core::pause: current state:" << stateToString();

	if (proc->isRunning() && _state != Paused) {
		proc->setPause(true);
		setState(Paused);
	}
}

void Core::playOrPause() {
	qDebug() << "Core::playOrPause: current state:" << stateToString();

	if (_state == Playing) {
		pause();
	} else {
		play();
	}
}

void Core::frameStep() {
	qDebug("Core::frameStep");

	if (proc->isRunning()) {
		if (_state == Paused) {
			proc->frameStep();
		} else {
			pause();
		}
	}
}

void Core::frameBackStep() {
	qDebug("Core::frameBackStep");

	if (proc->isRunning()) {
		if (_state == Paused) {
			proc->frameBackStep();
		} else {
			pause();
		}
	}
}

void Core::screenshot() {
	qDebug("Core::screenshot");

	if ( (!pref->screenshot_directory.isEmpty()) && 
         (QFileInfo(pref->screenshot_directory).isDir()) ) 
	{
		proc->takeScreenshot(PlayerProcess::Single, pref->subtitles_on_screenshots);
		qDebug("Core::screenshot: taken screenshot");
	} else {
		qDebug("Core::screenshot: error: directory for screenshots not valid");
		emit showMessage( tr("Screenshot NOT taken, folder not configured") );
	}
}

void Core::screenshots() {
	qDebug("Core::screenshots");

	if ( (!pref->screenshot_directory.isEmpty()) && 
         (QFileInfo(pref->screenshot_directory).isDir()) ) 
	{
		proc->takeScreenshot(PlayerProcess::Multiple, pref->subtitles_on_screenshots);
	} else {
		qDebug("Core::screenshots: error: directory for screenshots not valid");
		emit showMessage( tr("Screenshots NOT taken, folder not configured") );
	}
}

void Core::processFinished(bool normal_exit) {
	qDebug("Core::processFinished");

	// First restore normal window background
	mplayerwindow->playingStopped();

	if (normal_exit) {
		if (we_are_restarting) {
			qDebug("Core::processFinished: something tells me we are restarting...");
		} else {
			qDebug("Core::processFinished: player finished, entering the stopped state");
			setState(Stopped);
		}
	} else {
		mplayerwindow->showLogo();

		int exit_code = proc->exitCode();
		qWarning("Core::processFinished: player crash or error (%d), entering the stopped state.",
				 exit_code);
		setState(Stopped);

		qDebug("Core::processFinished: emit playerFinishedWithError(%d)", exit_code);
		emit playerFinishedWithError(exit_code);
	}
}

void Core::fileReachedEnd() {
	qDebug("Core::fileReachedEnd");

	// If we're at the end of the movie, reset to 0
	mset.current_sec = 0;
	updateWidgets();

	qDebug("Core::fileReachedEnd: emit mediaFinished()");
	emit mediaFinished();
}

void Core::startPlayer( QString file, double seek ) {
	qDebug("Core::startPlayer");

	if (file.isEmpty()) {
		qWarning("Core:startPlayer: file is empty!");
		return;
	}

	if (proc->isRunning()) {
		qWarning("Core::startPlayer: MPlayer still running!");
		return;
    }

	emit showMessage(tr("Starting player..."), 5000);

#ifdef YOUTUBE_SUPPORT
	// Stop any pending request
	#if 0
	qDebug("Core::startPlayer: yt state: %d", yt->state());
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
		qDebug("Core::startPlayer: checking if stream is a playlist");
		qDebug("Core::startPlayer: url path: '%s'", url.path().toUtf8().constData());

		if (url.scheme().toLower() != "ffmpeg") {
			QRegExp rx("\\.ram$|\\.asx$|\\.m3u$|\\.m3u8$|\\.pls$", Qt::CaseInsensitive);
			url_is_playlist = (rx.indexIn(url.path()) != -1);
		}
	}
	qDebug("Core::startPlayer: url_is_playlist: %d", url_is_playlist);


	// Check if a m4a file exists with the same name of file, in that cause if will be used as audio
	if (pref->autoload_m4a && mset.external_audio.isEmpty()) {
		QFileInfo fi(file);
		if (fi.exists() && !fi.isDir()) {
			if (fi.suffix().toLower() == "mp4") {
				QString file2 = fi.path() + "/" + fi.completeBaseName() + ".m4a";
				//qDebug("Core::startPlayer: file2: %s", file2.toUtf8().constData());
				if (!QFile::exists(file2)) {
					// Check for upper case
					file2 = fi.path() + "/" + fi.completeBaseName() + ".M4A";
				}
				if (QFile::exists(file2)) {
					qDebug("Core::startPlayer: found %s, so it will be used as audio file", file2.toUtf8().constData());
					mset.external_audio = file2;
				}
			}
		}
	}


	bool screenshot_enabled = ( (pref->use_screenshot) && 
                                (!pref->screenshot_directory.isEmpty()) && 
                                (QFileInfo(pref->screenshot_directory).isDir()) );

	proc->clearArguments();

	// Set working directory to screenshot directory
	if (screenshot_enabled) {
		qDebug("Core::startPlayer: setting working directory to '%s'", pref->screenshot_directory.toUtf8().data());
		proc->setWorkingDirectory( pref->screenshot_directory );
	}

	// Use absolute path, otherwise after changing to the screenshot directory
	// the mplayer path might not be found if it's a relative path
	// (seems to be necessary only for linux)
	QString mplayer_bin = pref->mplayer_bin;
	QFileInfo fi(mplayer_bin);
    if (fi.exists() && fi.isExecutable() && !fi.isDir()) {
        mplayer_bin = fi.absoluteFilePath();
	}

	if (fi.baseName().toLower() == "mplayer2") {
		if (!pref->mplayer_is_mplayer2) {
			qDebug("Core::startPlayer: this seems mplayer2");
			pref->mplayer_is_mplayer2 = true;
		}
	}

	proc->setExecutable(mplayer_bin);
	proc->setFixedOptions();

#ifdef LOG_MPLAYER
	if (pref->verbose_log) {
		proc->setOption("verbose");
	}
#endif

	if (pref->fullscreen && pref->use_mplayer_window) {
		proc->setOption("fs", true);
	} else {
		// No mplayer fullscreen mode
		proc->setOption("fs", false);
	}

#if !ALLOW_DEMUXER_CODEC_CHANGE
	if (pref->use_lavf_demuxer) {
		proc->setOption("demuxer", "lavf");
	}
#else
	// Demuxer and audio and video codecs:
	if (!mset.forced_demuxer.isEmpty()) {
		proc->setOption("demuxer", mset.forced_demuxer);
	}
	if (!mset.forced_audio_codec.isEmpty()) {
		proc->setOption("ac", mset.forced_audio_codec);
	}
	if (!mset.forced_video_codec.isEmpty()) {
		proc->setOption("vc", mset.forced_video_codec);
	}
	else
#endif
	{
		#ifndef Q_OS_WIN
		/* if (pref->vo.startsWith("x11")) { */ // My card doesn't support vdpau, I use x11 to test
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
		}
		else {
		#endif
			if (pref->coreavc) {
				proc->setOption("vc", "coreserve,");
			}
		#ifndef Q_OS_WIN
		}
		#endif
	}

	if (pref->use_hwac3) {
		proc->setOption("afm", "hwac3");
	}


	if (proc->isMPlayer()) {
		// MPlayer
		QString lavdopts;

		if ( (pref->h264_skip_loop_filter == Preferences::LoopDisabled) || 
	         ((pref->h264_skip_loop_filter == Preferences::LoopDisabledOnHD) && 
	          (mset.is264andHD)) )
		{
			if (!lavdopts.isEmpty()) lavdopts += ":";
			lavdopts += "skiploopfilter=all";
		}

		if (pref->threads > 1) {
			if (!lavdopts.isEmpty()) lavdopts += ":";
			lavdopts += "threads=" + QString::number(pref->threads);
		}

		if (!lavdopts.isEmpty()) {
			proc->setOption("lavdopts", lavdopts);
		}
	}
	else {
		// MPV
		if ( (pref->h264_skip_loop_filter == Preferences::LoopDisabled) || 
	         ((pref->h264_skip_loop_filter == Preferences::LoopDisabledOnHD) && 
	          (mset.is264andHD)) )
		{
			proc->setOption("skiploopfilter");
		}

		if (pref->threads > 1) {
			proc->setOption("threads", QString::number(pref->threads));
		}
	}

	if (!pref->hwdec.isEmpty()) proc->setOption("hwdec", pref->hwdec);

	proc->setOption("sub-fuzziness", pref->subfuzziness);

	if (pref->vo != "player_default") {
		if (!pref->vo.isEmpty()) {
			proc->setOption("vo", pref->vo );
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
			proc->setOption("ao", pref->ao );
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
		case Preferences::Realtime: 	p = "realtime"; 
										app_p = REALTIME_PRIORITY_CLASS;
										break;
		case Preferences::High:			p = "high"; 
										app_p = REALTIME_PRIORITY_CLASS;
										break;
		case Preferences::AboveNormal:	p = "abovenormal"; 
										app_p = HIGH_PRIORITY_CLASS;
										break;
		case Preferences::Normal: 		p = "normal"; 
										app_p = ABOVE_NORMAL_PRIORITY_CLASS; 
										break;
		case Preferences::BelowNormal: 	p = "belownormal"; break;
		case Preferences::Idle: 		p = "idle"; break;
		default: 						p = "normal";
	}
	proc->setOption("priority", p);
	/*
	SetPriorityClass(GetCurrentProcess(), app_p);
	qDebug("Core::startPlayer: priority of smplayer process set to %d", app_p);
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

	if (!pref->use_mplayer_window) {
		proc->disableInput();
		proc->setOption("keepaspect", false);

#if defined(Q_OS_OS2)
		#define WINIDFROMHWND(hwnd) ( ( hwnd ) - 0x80000000UL )
		proc->setOption("wid", QString::number( WINIDFROMHWND( (int) mplayerwindow->videoLayer()->winId() ) ));
#else
		proc->setOption("wid", QString::number( (qint64) mplayerwindow->videoLayer()->winId() ) );
#endif

#if USE_COLORKEY
		#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
		if ((pref->vo.startsWith("directx")) || (pref->vo.startsWith("kva")) || (pref->vo.isEmpty())) {
			proc->setOption("colorkey", ColorUtils::colorToRGB(pref->color_key));
		}
		#endif
#endif

		// Square pixels
		proc->setOption("monitorpixelaspect", "1");
	} else {
		// no -wid
		proc->setOption("keepaspect", true);
		if (!pref->monitor_aspect.isEmpty()) {
			proc->setOption("monitoraspect", pref->monitor_aspect);
		}
	}

	// OSD
	proc->setOption("osdlevel", pref->osd_level);
	if (proc->isMPlayer()) {
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

		if (!pref->mplayer_is_mplayer2) {
			proc->setOption("flip-hebrew",false); // It seems to be necessary to display arabic subtitles correctly when using -ass
		}

		if (pref->enable_ass_styles) {
			QString ass_force_style;
			if (!pref->user_forced_ass_style.isEmpty()) {
				ass_force_style = pref->user_forced_ass_style;
			} else {
				ass_force_style = pref->ass_styles.toString();
			}

			if (proc->isMPV()) {
				// MPV
				proc->setSubStyles(pref->ass_styles);
				if (pref->force_ass_styles) {
					proc->setOption("ass-force-style", ass_force_style);
				}
			} else {
				// MPlayer
				if (!pref->force_ass_styles) {
					proc->setSubStyles(pref->ass_styles, Paths::subtitleStyleFile());
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
		if (proc->isMPV()) {
			proc->setOption("sub-scale", QString::number(mset.sub_scale_mpv));
		} else {
			proc->setOption("subfont-text-scale", QString::number(mset.sub_scale));
		}
	}

	// Subtitle encoding
	{
		QString encoding;
		if ( (pref->use_enca) && (!pref->enca_lang.isEmpty()) ) {
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
	if ( (mset.current_program_id != MediaSettings::NoneSelected) /*&& 
         (mset.current_video_id == MediaSettings::NoneSelected) && 
         (mset.current_audio_id == MediaSettings::NoneSelected)*/ )
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
	} else if (mset.current_sub_idx >= 0) {
		// Selected sub when restarting
		mset.sub = mdat.subs.itemAt(mset.current_sub_idx);
	}

	if (mset.sub.type() == SubData::Vob) {
		// Set vob subs. Mplayer only
		// External sub
		if (!mset.sub.filename().isEmpty()) {
			// Remove extension
			QFileInfo fi(mset.sub.filename());
			QString s = fi.path() +"/"+ fi.completeBaseName();
			qDebug("Core::startPlayer: subtitle file without extension: '%s'", s.toUtf8().data());
			proc->setOption("vobsub", s);
		}
		if (mset.sub.ID() >= 0) {
			proc->setOption("sub_vob", mset.sub.ID());
		}
	} else if (mset.sub.type() == SubData::File) {
		if (!mset.sub.filename().isEmpty()) {
			proc->setOption("sub", mset.sub.filename());
			if (mset.sub.ID() >= 0) {
				proc->setOption("sub_file", mset.sub.ID());
			}
		}
	} else if (mset.sub.type() == SubData::Sub && mset.sub.ID() >= 0) {
		// Subs from demux when restarting or from settings local file
		proc->setOption("sub_demux", mset.sub.ID());
	}

	// Set fps external file
	if (!mset.sub.filename().isEmpty()
		&& mset.external_subtitles_fps != MediaSettings::SFPS_None) {

		QString fps;
		switch (mset.external_subtitles_fps) {
			case MediaSettings::SFPS_23: fps = "23"; break;
			case MediaSettings::SFPS_24: fps = "24"; break;
			case MediaSettings::SFPS_25: fps = "25"; break;
			case MediaSettings::SFPS_30: fps = "30"; break;
			case MediaSettings::SFPS_23976: fps = "24000/1001"; break;
			case MediaSettings::SFPS_29970: fps = "30000/1001"; break;
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
		qDebug("Core::startPlayer: don't set volume since -volume is used");
	} else {
		int vol = (pref->global_volume ? pref->volume : mset.volume);
		if (proc->isMPV()) {
			vol = adjustVolume(vol, pref->use_soft_vol ? pref->softvol_max : 100);
		}
		proc->setOption("volume", QString::number(vol));
	}

	if (mset.current_angle_id > 0) {
		proc->setOption("dvdangle", QString::number( mset.current_angle_id));
	}

	// TODO: cache 0 for .iso?
	switch (mdat.selected_type) {
		case MediaData::TYPE_FILE	: cache_size = pref->cache_for_files; break;
		case MediaData::TYPE_DVD 	: cache_size = pref->cache_for_dvds; break;
		case MediaData::TYPE_DVDNAV	: cache_size = 0; break;
		case MediaData::TYPE_STREAM	: cache_size = pref->cache_for_streams; break;
		case MediaData::TYPE_VCD 	: cache_size = pref->cache_for_vcds; break;
		case MediaData::TYPE_CDDA	: cache_size = pref->cache_for_audiocds; break;
		case MediaData::TYPE_TV		: cache_size = pref->cache_for_tv; break;
		case MediaData::TYPE_BLURAY	: cache_size = pref->cache_for_dvds; break; // FIXME: use cache for bluray?
		default: cache_size = 0;
	}

	proc->setOption("cache", QString::number(cache_size));

	if (mset.speed != 1.0) {
		proc->setOption("speed", QString::number(mset.speed));
	}

	if (mdat.selected_type != MediaData::TYPE_TV) {
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

	if (mdat.selected_type == MediaData::TYPE_STREAM) {
		if (pref->prefer_ipv4) {
			proc->setOption("prefer-ipv4");
		} else {
			proc->setOption("prefer-ipv6");
		}
	}

	if (pref->use_correct_pts != Preferences::Detect) {
		proc->setOption("correct-pts", (pref->use_correct_pts == Preferences::Enabled));
	}

	bool force_noslices = false;

#ifndef Q_OS_WIN
	if (proc->isMPlayer()) {
		if ((pref->vdpau.disable_video_filters) && (pref->vo.startsWith("vdpau"))) {
			qDebug("Core::startPlayer: using vdpau, video filters are ignored");
			goto end_video_filters;
		}
	} else {
		// MPV
		if (!pref->hwdec.isEmpty() && pref->hwdec != "no") {
			qDebug("Core::startPlayer: hardware decoding is enabled. The video filters will be ignored");
			goto end_video_filters;
		}
	}
#endif

	// Video filters:
	// Phase
	if (mset.phase_filter) {
		proc->addVF("phase", "A");
	}

	// Deinterlace
	if (mset.current_deinterlacer != MediaSettings::NoDeinterlace) {
		switch (mset.current_deinterlacer) {
			case MediaSettings::L5: 		proc->addVF("l5"); break;
			case MediaSettings::Yadif: 		proc->addVF("yadif"); break;
			case MediaSettings::LB:			proc->addVF("lb"); break;
			case MediaSettings::Yadif_1:	proc->addVF("yadif", "1"); break;
			case MediaSettings::Kerndeint:	proc->addVF("kerndeint", "5"); break;
		}
	}

	// 3D stereo
	if (mset.stereo3d_in != "none" && !mset.stereo3d_out.isEmpty()) {
		proc->addStereo3DFilter(mset.stereo3d_in, mset.stereo3d_out);
	}

	// Denoise
	if (mset.current_denoiser != MediaSettings::NoDenoise) {
		if (mset.current_denoiser==MediaSettings::DenoiseSoft) {
			proc->addVF("hqdn3d", pref->filters->item("denoise_soft").options());
		} else {
			proc->addVF("hqdn3d", pref->filters->item("denoise_normal").options());
		}
	}

	// Unsharp
	if (mset.current_unsharp != 0) {
		if (mset.current_unsharp == 1) {
			proc->addVF("blur", pref->filters->item("blur").options());
		} else {
			proc->addVF("sharpen", pref->filters->item("sharpen").options());
		}
	}

	// Deblock
	if (mset.deblock_filter) {
		proc->addVF("deblock", pref->filters->item("deblock").options());
	}

	// Dering
	if (mset.dering_filter) {
		proc->addVF("dering");
	}

	// Gradfun
	if (mset.gradfun_filter) {
		proc->addVF("gradfun", pref->filters->item("gradfun").options());
	}

	// Upscale
	if (mset.upscaling_filter) {
		int width = DesktopInfo::desktop_size(mplayerwindow).width();
		proc->setOption("sws", "9");
		proc->addVF("scale", QString::number(width) + ":-2");
	}

	// Addnoise
	if (mset.noise_filter) {
		proc->addVF("noise", pref->filters->item("noise").options());
	}

	// Postprocessing
	if (mset.postprocessing_filter) {
		proc->addVF("postprocessing");
		proc->setOption("autoq", QString::number(pref->autoq));
	}

	// Letterbox (expand)
	if ((mset.add_letterbox) || (pref->fullscreen && pref->add_blackborders_on_fullscreen)) {
		proc->addVF("expand", QString("aspect=%1").arg( DesktopInfo::desktop_aspectRatio(mplayerwindow)));
	}

	// Software equalizer
	if ( (pref->use_soft_video_eq) ) {
		proc->addVF("eq2");
		proc->addVF("hue");
		if ( (pref->vo == "gl") || (pref->vo == "gl2") || (pref->vo == "gl_tiled")
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
	if ( !mset.mplayer_additional_video_filters.isEmpty() ) {
		proc->setOption("vf-add", mset.mplayer_additional_video_filters);
	}
	// Global
	if ( !pref->mplayer_additional_video_filters.isEmpty() ) {
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
	if (mset.rotate != MediaSettings::NoRotate) {
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
	if ((screenshot_enabled) && (!pref->screenshot_template.isEmpty())) {
		proc->setOption("screenshot_template", pref->screenshot_template);
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
		if (mset.karaoke_filter) {
			proc->addAF("karaoke");
		}

		// Stereo mode
		if (mset.stereo_mode != 0) {
			switch (mset.stereo_mode) {
				case MediaSettings::Left: proc->addAF("channels", "2:2:0:1:0:0"); break;
				case MediaSettings::Right: proc->addAF("channels", "2:2:1:0:1:1"); break;
				case MediaSettings::Mono: proc->addAF("pan", "1:0.5:0.5"); break;
				case MediaSettings::Reverse: proc->addAF("channels", "2:2:0:1:1:0"); break;
			}
		}

		if (mset.extrastereo_filter) {
			proc->addAF("extrastereo");
		}

		if (mset.volnorm_filter) {
			proc->addAF("volnorm", pref->filters->item("volnorm").options());
		}

		bool use_scaletempo = pref->use_scaletempo == Preferences::Enabled;
		if (pref->use_scaletempo == Preferences::Detect) {
			use_scaletempo = true;
		}
		if (use_scaletempo) {
			proc->addAF("scaletempo");
		}

		// Audio equalizer
		if (pref->use_audio_equalizer) {
			AudioEqualizerList l = pref->global_audio_equalizer ? pref->audio_equalizer : mset.audio_equalizer;
			proc->addAF("equalizer", Helper::equalizerListToString(l));
		}

		// Additional audio filters, supplied by user
		// File
		if ( !pref->mplayer_additional_audio_filters.isEmpty() ) {
			proc->setOption("af-add", pref->mplayer_additional_audio_filters);
		}
		// Global
		if ( !mset.mplayer_additional_audio_filters.isEmpty() ) {
			proc->setOption("af-add", mset.mplayer_additional_audio_filters);
		}
	} else {
		// Don't use audio filters if using the S/PDIF output
			qDebug("Core::startPlayer: audio filters are disabled when using the S/PDIF output!");
	}

	if (pref->use_soft_vol) {
		proc->setOption("softvol");
		proc->setOption("softvol-max", QString::number(pref->softvol_max));
	}

#ifdef MPV_SUPPORT
	proc->setOption("enable_streaming_sites_support", pref->enable_streaming_sites);
#endif

#ifndef Q_OS_WIN
	if (proc->isMPV() && file.startsWith("dvb:")) {
		QString channels_file = TVList::findChannelsFile();
		qDebug("Core::startPlayer: channels_file: %s", channels_file.toUtf8().constData());
		if (!channels_file.isEmpty()) proc->setChannelsFile(channels_file);
	}
#endif

	// Load edl file
	if (pref->use_edl_files) {
		QString edl_f;
		QFileInfo f(file);
		QString basename = f.path() + "/" + f.completeBaseName();

		//qDebug("Core::startPlayer: file basename: '%s'", basename.toUtf8().data());

		if (QFile::exists(basename+".edl")) 
			edl_f = basename+".edl";
		else
		if (QFile::exists(basename+".EDL")) 
			edl_f = basename+".EDL";

		qDebug("Core::startPlayer: edl file: '%s'", edl_f.toUtf8().data());
		if (!edl_f.isEmpty()) {
			proc->setOption("edl", edl_f);
		}
	}

	// Additional options supplied by the user
	// File
	if (!mset.mplayer_additional_options.isEmpty()) {
		QStringList args = MyProcess::splitArguments(mset.mplayer_additional_options);
		for (int n = 0; n < args.count(); n++) {
			QString arg = args[n].simplified();
			if (!arg.isEmpty()) {
				proc->addUserOption(arg);
			}
		}
	}

	// Global
	if (!pref->mplayer_additional_options.isEmpty()) {
		QString additional_options = pref->mplayer_additional_options;
		// mplayer2 doesn't support -fontconfig and -nofontconfig
		if (pref->mplayer_is_mplayer2) {
			additional_options.replace("-fontconfig", "");
			additional_options.replace("-nofontconfig", "");
		}
		QStringList args = MyProcess::splitArguments(additional_options);
		for (int n = 0; n < args.count(); n++) {
			QString arg = args[n].simplified();
			if (!arg.isEmpty()) {
				qDebug("arg %d: %s", n, arg.toUtf8().constData());
				proc->addUserOption(arg);
			}
		}

	}

	// Last checks for the file

	// Open https URLs with ffmpeg
	if (proc->isMPlayer() && file.startsWith("https")) {
		file = "ffmpeg://" + file;
	}

/*
#if DVDNAV_SUPPORT
	if (proc->isMPV() && file.startsWith("dvdnav:")) {
		// Hack to open the DVD menu with MPV
		file = "dvd://menu";
	}
#endif
*/

#ifdef Q_OS_WIN
	if (pref->use_short_pathnames) {
		file = Helper::shortPathName(file);
	}
#endif

	proc->setMedia(file, proc->isMPlayer() ? url_is_playlist : false); // Don't use playlist with mpv

	// It seems the loop option must be after the filename
	if (mset.loop) {
		proc->setOption("loop", "0");
	}

	emit aboutToStartPlaying();

	QString commandline = proc->arguments().join(" ");
	qDebug("Core::startPlayer: command: '%s'", commandline.toUtf8().data());

	//Log command
	QString line_for_log = commandline + "\n";
	emit logLineAvailable(line_for_log);

	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	if ((pref->use_proxy) && (pref->proxy_type == QNetworkProxy::HttpProxy) && (!pref->proxy_host.isEmpty())) {
		QString proxy = QString("http://%1:%2@%3:%4").arg(pref->proxy_username).arg(pref->proxy_password).arg(pref->proxy_host).arg(pref->proxy_port);
		env.insert("http_proxy", proxy);
	}
	//qDebug("Core::startPlayer: env: %s", env.toStringList().join(",").toUtf8().constData());
	#ifdef Q_OS_WIN
	if (!pref->use_windowsfontdir) {
		env.insert("FONTCONFIG_FILE", Paths::configPath() + "/fonts.conf");
	}
	#endif
	proc->setProcessEnvironment(env);

	if ( !proc->startPlayer() ) {
		// TODO: error handling
		qWarning("Core::startPlayer: mplayer process didn't start");
	}

}

void Core::stopPlayer() {

	if (!proc->isRunning()) {
		qDebug("Core::stopPlayer: player not running");
		return;
	}
	qDebug("Core::stopPlayer");

	// If set high enough the OS will detect the "not responding state" and popup a dialog
	int timeout = pref->time_to_kill_mplayer;
	if (timeout < 4000) {
		qDebug("Core::stopPlayer: timeout %d much too small, adjusting it to 4000 ms", timeout);
		timeout = 4000;
	}

#ifdef Q_OS_OS2
	QEventLoop eventLoop;

	connect(proc, SIGNAL(processExited(bool)), &eventLoop, SLOT(quit()));

	proc->quit();

	QTimer::singleShot(timeout, &eventLoop, SLOT(quit()));
	eventLoop.exec(QEventLoop::ExcludeUserInputEvents);

	if (proc->isRunning()) {
		qWarning("Core::stopPlayer: player didn't finish. Killing it...");
		proc->kill();
	}
#else
	proc->quit();

	qDebug("Core::stopPlayer: Waiting %d ms for player to finish...", timeout);
	if (!proc->waitForFinished(timeout)) {
		qWarning("Core::stopPlayer: player process did not finish in %d ms. Killing it...",
				 timeout);
		proc->kill();
	}
#endif

	qDebug("Core::stopPlayer: Finished. (I hope)");
}

void Core::goToPosition(int pos) {
	qDebug("Core::goToPosition: %d/%d", pos, pos_max);

	if (pos < 0) pos = 0;
	else if (pos >= pos_max) pos = pos_max - 1;

	if (pref->relative_seeking || mdat.duration <= 0) {
		goToPos((double) pos / ((double) pos_max / 100));
	} else {
		goToSec(mdat.duration * pos / pos_max);
	}
}

void Core::goToPos(double perc) {
	qDebug("Core::goToPos: per: %f", perc);
	seek_cmd(perc, 1);
}

void Core::goToSec( double sec ) {
	qDebug("Core::goToSec: %f", sec);
	seek_cmd(sec, 2);
}

void Core::seek(int secs) {
	qDebug("Core::seek: seek relative %d secs", secs);

	// Something to seek?
	if (qAbs(secs) > 0.01)
		seek_cmd(secs, 0);
}

void Core::seek_cmd(double secs, int mode) {

	if (secs < 0 && mode != 0)
		secs = 0;
	if (mdat.duration > 0 && secs >= mdat.duration) {
		if (mdat.video_fps > 0)
			secs = mdat.duration - (1.0 / mdat.video_fps);
		else secs = mdat.duration - 0.1;
	}

	if (proc->isFullyStarted())
		proc->seek(secs, mode, pref->precise_seeking, _state == Paused);
}

void Core::sforward() {
	qDebug("Core::sforward");
	seek( pref->seeking1 ); // +10s
}

void Core::srewind() {
	qDebug("Core::srewind");
	seek( -pref->seeking1 ); // -10s
}


void Core::forward() {
	qDebug("Core::forward");
	seek( pref->seeking2 ); // +1m
}


void Core::rewind() {
	qDebug("Core::rewind");
	seek( -pref->seeking2 ); // -1m
}


void Core::fastforward() {
	qDebug("Core::fastforward");
	seek( pref->seeking3 ); // +10m
}


void Core::fastrewind() {
	qDebug("Core::fastrewind");
	seek( -pref->seeking3 ); // -10m
}

void Core::forward(int secs) {
	qDebug("Core::forward: %d", secs);
	seek(secs);
}

void Core::rewind(int secs) {
	qDebug("Core::rewind: %d", secs);
	seek(-secs);
}

void Core::wheelUp(Preferences::WheelFunction function) {
	qDebug("Core::wheelUp");

	if (function == Preferences::DoNothing)
		function = (Preferences::WheelFunction) pref->wheel_function;
	switch (function) {
		case Preferences::Volume : incVolume(); break;
		case Preferences::Zoom : incZoom(); break;
		case Preferences::Seeking : pref->wheel_function_seeking_reverse ? rewind( pref->seeking4 ) : forward( pref->seeking4 ); break;
		case Preferences::ChangeSpeed : incSpeed10(); break;
		default : {} // do nothing
	}
}

void Core::wheelDown(Preferences::WheelFunction function) {
	qDebug("Core::wheelDown");

	if (function == Preferences::DoNothing)
		function = (Preferences::WheelFunction) pref->wheel_function;
	switch (function) {
		case Preferences::Volume : decVolume(); break;
		case Preferences::Zoom : decZoom(); break;
		case Preferences::Seeking : pref->wheel_function_seeking_reverse ? forward( pref->seeking4 ) : rewind( pref->seeking4 ); break;
		case Preferences::ChangeSpeed : decSpeed10(); break;
		default : {} // do nothing
	}
}

void Core::setAMarker() {
	setAMarker((int)mset.current_sec);
}

void Core::setAMarker(int sec) {
	qDebug("Core::setAMarker: %d", sec);

	mset.A_marker = sec;
	displayMessage( tr("\"A\" marker set to %1").arg(Helper::formatTime(sec)) );

	if (mset.B_marker > mset.A_marker) {
		if (proc->isRunning()) restartPlay();
	}

	emit ABMarkersChanged(mset.A_marker, mset.B_marker);
}

void Core::setBMarker() {
	setBMarker((int)mset.current_sec);
}

void Core::setBMarker(int sec) {
	qDebug("Core::setBMarker: %d", sec);

	mset.B_marker = sec;
	displayMessage( tr("\"B\" marker set to %1").arg(Helper::formatTime(sec)) );

	if ((mset.A_marker > -1) && (mset.A_marker < mset.B_marker)) {
		if (proc->isRunning()) restartPlay();
	}

	emit ABMarkersChanged(mset.A_marker, mset.B_marker);
}

void Core::clearABMarkers() {
	qDebug("Core::clearABMarkers");

	if ((mset.A_marker != -1) || (mset.B_marker != -1)) {
		mset.A_marker = -1;
		mset.B_marker = -1;
		displayMessage( tr("A-B markers cleared") );
		if (proc->isRunning()) restartPlay();
	}

	emit ABMarkersChanged(mset.A_marker, mset.B_marker);
}

void Core::toggleRepeat() {
	qDebug("Core::toggleRepeat");
	toggleRepeat( !mset.loop );
}

void Core::toggleRepeat(bool b) {
	qDebug("Core::toggleRepeat: %d", b);
	if ( mset.loop != b ) {
		mset.loop = b;
		if (MplayerVersion::isMplayerAtLeast(23747)) {
			// Use slave command
			int v = -1; // no loop
			if (mset.loop) v = 0; // infinite loop
			proc->setLoop(v);
		} else {
			// Restart mplayer
			if (proc->isRunning()) restartPlay();
		}
	}
}


// Audio filters
void Core::toggleKaraoke() {
	toggleKaraoke( !mset.karaoke_filter );
}

void Core::toggleKaraoke(bool b) {
	qDebug("Core::toggleKaraoke: %d", b);
	if (b != mset.karaoke_filter) {
		mset.karaoke_filter = b;
		if (MplayerVersion::isMplayerAtLeast(31030)) {
			// Change filter without restarting
			proc->enableKaraoke(b);
		} else {
			restartPlay();
		}
	}
}

void Core::toggleExtrastereo() {
	toggleExtrastereo( !mset.extrastereo_filter );
}

void Core::toggleExtrastereo(bool b) {
	qDebug("Core::toggleExtrastereo: %d", b);
	if (b != mset.extrastereo_filter) {
		mset.extrastereo_filter = b;
		if (MplayerVersion::isMplayerAtLeast(31030)) {
			// Change filter without restarting
			proc->enableExtrastereo(b);
		} else {
			restartPlay();
		}
	}
}

void Core::toggleVolnorm() {
	toggleVolnorm( !mset.volnorm_filter );
}

void Core::toggleVolnorm(bool b) {
	qDebug("Core::toggleVolnorm: %d", b);
	if (b != mset.volnorm_filter) {
		mset.volnorm_filter = b;
		if (MplayerVersion::isMplayerAtLeast(31030)) {
			// Change filter without restarting
			QString f = pref->filters->item("volnorm").filter();
			proc->enableVolnorm(b, pref->filters->item("volnorm").options());
		} else {
			restartPlay();
		}
	}
}

void Core::setAudioChannels(int channels) {
	qDebug("Core::setAudioChannels:%d", channels);
	if (channels != mset.audio_use_channels ) {
		mset.audio_use_channels = channels;
		restartPlay();
	}
}

void Core::setStereoMode(int mode) {
	qDebug("Core::setStereoMode:%d", mode);
	if (mode != mset.stereo_mode ) {
		mset.stereo_mode = mode;
		restartPlay();
	}
}


// Video filters

#define CHANGE_VF(Filter, Enable, Option) \
	if (proc->isMPV()) { \
		proc->changeVF(Filter, Enable, Option); \
	} else { \
		restartPlay(); \
	}

void Core::toggleFlip() {
	qDebug("Core::toggleFlip");
	toggleFlip( !mset.flip );
}

void Core::toggleFlip(bool b) {
	qDebug("Core::toggleFlip: %d", b);
	if (mset.flip != b) {
		mset.flip = b;
		CHANGE_VF("flip", b, QVariant());
	}
}

void Core::toggleMirror() {
	qDebug("Core::toggleMirror");
	toggleMirror( !mset.mirror );
}

void Core::toggleMirror(bool b) {
	qDebug("Core::toggleMirror: %d", b);
	if (mset.mirror != b) {
		mset.mirror = b;
		CHANGE_VF("mirror", b, QVariant());
	}
}

void Core::toggleAutophase() {
	toggleAutophase( !mset.phase_filter );
}

void Core::toggleAutophase( bool b ) {
	qDebug("Core::toggleAutophase: %d", b);
	if ( b != mset.phase_filter) {
		mset.phase_filter = b;
		CHANGE_VF("phase", b, "A");
	}
}

void Core::toggleDeblock() {
	toggleDeblock( !mset.deblock_filter );
}

void Core::toggleDeblock(bool b) {
	qDebug("Core::toggleDeblock: %d", b);
	if ( b != mset.deblock_filter ) {
		mset.deblock_filter = b;
		CHANGE_VF("deblock", b, pref->filters->item("deblock").options());
	}
}

void Core::toggleDering() {
	toggleDering( !mset.dering_filter );
}

void Core::toggleDering(bool b) {
	qDebug("Core::toggleDering: %d", b);
	if ( b != mset.dering_filter) {
		mset.dering_filter = b;
		CHANGE_VF("dering", b, QVariant());
	}
}

void Core::toggleGradfun() {
	toggleGradfun( !mset.gradfun_filter );
}

void Core::toggleGradfun(bool b) {
	qDebug("Core::toggleGradfun: %d", b);
	if ( b != mset.gradfun_filter) {
		mset.gradfun_filter = b;
		CHANGE_VF("gradfun", b, pref->filters->item("gradfun").options());
	}
}

void Core::toggleNoise() {
	toggleNoise( !mset.noise_filter );
}

void Core::toggleNoise(bool b) {
	qDebug("Core::toggleNoise: %d", b);
	if ( b != mset.noise_filter ) {
		mset.noise_filter = b;
		CHANGE_VF("noise", b, QVariant());
	}
}

void Core::togglePostprocessing() {
	togglePostprocessing( !mset.postprocessing_filter );
}

void Core::togglePostprocessing(bool b) {
	qDebug("Core::togglePostprocessing: %d", b);
	if ( b != mset.postprocessing_filter ) {
		mset.postprocessing_filter = b;
		CHANGE_VF("postprocessing", b, QVariant());
	}
}

void Core::changeDenoise(int id) {
	qDebug( "Core::changeDenoise: %d", id );
	if (id != mset.current_denoiser) {
		if (proc->isMPlayer()) {
			mset.current_denoiser = id;
			restartPlay();
		} else {
			// MPV
			QString dsoft = pref->filters->item("denoise_soft").options();
			QString dnormal = pref->filters->item("denoise_normal").options();
			// Remove previous filter
			switch (mset.current_denoiser) {
				case MediaSettings::DenoiseSoft: proc->changeVF("hqdn3d", false, dsoft); break;
				case MediaSettings::DenoiseNormal: proc->changeVF("hqdn3d", false, dnormal); break;
			}
			// New filter
			mset.current_denoiser = id;
			switch (mset.current_denoiser) {
				case MediaSettings::DenoiseSoft: proc->changeVF("hqdn3d", true, dsoft); break;
				case MediaSettings::DenoiseNormal: proc->changeVF("hqdn3d", true, dnormal); break;
			}
		}
	}
}

void Core::changeUnsharp(int id) {
	qDebug( "Core::changeUnsharp: %d", id );
	if (id != mset.current_unsharp) {
		if (proc->isMPlayer()) {
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

void Core::changeUpscale(bool b) {
	qDebug( "Core::changeUpscale: %d", b );
	if (mset.upscaling_filter != b) {
		mset.upscaling_filter = b;
		int width = DesktopInfo::desktop_size(mplayerwindow).width();
		CHANGE_VF("scale", b, QString::number(width) + ":-2");
	}
}

void Core::changeStereo3d(const QString & in, const QString & out) {
	qDebug("Core::changeStereo3d: in: %s out: %s", in.toUtf8().constData(), out.toUtf8().constData());

	if ((mset.stereo3d_in != in) || (mset.stereo3d_out != out)) {
		if (proc->isMPlayer()) {
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

void Core::setBrightness(int value) {
	qDebug("Core::setBrightness: %d", value);

	if (value > 100) value = 100;
	if (value < -100) value = -100;

	if (value != mset.brightness) {
		proc->setBrightness(value);
		mset.brightness = value;
		displayMessage( tr("Brightness: %1").arg(value) );
		emit videoEqualizerNeedsUpdate();
	}
}


void Core::setContrast(int value) {
	qDebug("Core::setContrast: %d", value);

	if (value > 100) value = 100;
	if (value < -100) value = -100;

	if (value != mset.contrast) {
		proc->setContrast(value);
		mset.contrast = value;
		displayMessage( tr("Contrast: %1").arg(value) );
		emit videoEqualizerNeedsUpdate();
	}
}

void Core::setGamma(int value) {
	qDebug("Core::setGamma: %d", value);

	if (value > 100) value = 100;
	if (value < -100) value = -100;

	if (value != mset.gamma) {
		proc->setGamma(value);
		mset.gamma= value;
		displayMessage( tr("Gamma: %1").arg(value) );
		emit videoEqualizerNeedsUpdate();
	}
}

void Core::setHue(int value) {
	qDebug("Core::setHue: %d", value);

	if (value > 100) value = 100;
	if (value < -100) value = -100;

	if (value != mset.hue) {
		proc->setHue(value);
		mset.hue = value;
		displayMessage( tr("Hue: %1").arg(value) );
		emit videoEqualizerNeedsUpdate();
	}
}

void Core::setSaturation(int value) {
	qDebug("Core::setSaturation: %d", value);

	if (value > 100) value = 100;
	if (value < -100) value = -100;

	if (value != mset.saturation) {
		proc->setSaturation(value);
		mset.saturation = value;
		displayMessage( tr("Saturation: %1").arg(value) );
		emit videoEqualizerNeedsUpdate();
	}
}

void Core::incBrightness() {
	setBrightness(mset.brightness + pref->min_step);
}

void Core::decBrightness() {
	setBrightness(mset.brightness - pref->min_step);
}

void Core::incContrast() {
	setContrast(mset.contrast + pref->min_step);
}

void Core::decContrast() {
	setContrast(mset.contrast - pref->min_step);
}

void Core::incGamma() {
	setGamma(mset.gamma + pref->min_step);
}

void Core::decGamma() {
	setGamma(mset.gamma - pref->min_step);
}

void Core::incHue() {
	setHue(mset.hue + pref->min_step);
}

void Core::decHue() {
	setHue(mset.hue - pref->min_step);
}

void Core::incSaturation() {
	setSaturation(mset.saturation + pref->min_step);
}

void Core::decSaturation() {
	setSaturation(mset.saturation - pref->min_step);
}

void Core::setSpeed( double value ) {
	qDebug("Core::setSpeed: %f", value);

	if (value < 0.10) value = 0.10;
	if (value > 100) value = 100;

	mset.speed = value;
	proc->setSpeed(value);

	displayMessage( tr("Speed: %1").arg(value) );
}

void Core::incSpeed10() {
	qDebug("Core::incSpeed10");
	setSpeed( (double) mset.speed + 0.1 );
}

void Core::decSpeed10() {
	qDebug("Core::decSpeed10");
	setSpeed( (double) mset.speed - 0.1 );
}

void Core::incSpeed4() {
	qDebug("Core::incSpeed4");
	setSpeed( (double) mset.speed + 0.04 );
}

void Core::decSpeed4() {
	qDebug("Core::decSpeed4");
	setSpeed( (double) mset.speed - 0.04 );
}

void Core::incSpeed1() {
	qDebug("Core::incSpeed1");
	setSpeed( (double) mset.speed + 0.01 );
}

void Core::decSpeed1() {
	qDebug("Core::decSpeed1");
	setSpeed( (double) mset.speed - 0.01 );
}

void Core::doubleSpeed() {
	qDebug("Core::doubleSpeed");
	setSpeed( (double) mset.speed * 2 );
}

void Core::halveSpeed() {
	qDebug("Core::halveSpeed");
	setSpeed( (double) mset.speed / 2 );
}

void Core::normalSpeed() {
	setSpeed(1);
}

int Core::adjustVolume(int v, int max_vol) {
	//qDebug() << "Core::adjustVolume: v:" << v << "max_vol:" << max_vol;
	if (max_vol < 100) max_vol = 100;
	int vol = v * max_vol / 100;
	return vol;
}

void Core::setVolume(int volume, bool force) {
	qDebug("Core::setVolume: %d", volume);

	int current_volume = (pref->global_volume ? pref->volume : mset.volume);

	if ((volume == current_volume) && (!force)) return;

	current_volume = volume;
	if (current_volume > 100) current_volume = 100;
	if (current_volume < 0) current_volume = 0;

	if (proc->isMPV()) {
		// MPV
		int vol = adjustVolume(current_volume, pref->use_soft_vol ? pref->softvol_max : 100);
		proc->setVolume(vol);
	} else {
		// MPlayer
		if (state() == Paused) {
			// Change volume later, after quiting pause
			change_volume_after_unpause = true;
		} else {
			proc->setVolume(current_volume);
		}
	}

	if (pref->global_volume) {
		pref->volume = current_volume;
		pref->mute = false;
	} else {
		mset.volume = current_volume;
		mset.mute = false;
	}

	updateWidgets();

	displayMessage( tr("Volume: %1").arg(current_volume) );
	emit volumeChanged( current_volume );
}

void Core::switchMute() {
	qDebug("Core::switchMute");

	mset.mute = !mset.mute;
	mute(mset.mute);
}

void Core::mute(bool b) {
	qDebug("Core::mute");

	proc->mute(b);

	if (pref->global_volume) {
		pref->mute = b;
	} else {
		mset.mute = b;
	}

	updateWidgets();
}

void Core::incVolume() {
	qDebug("Core::incVolume");
	int new_vol = (pref->global_volume ? pref->volume + pref->min_step : mset.volume + pref->min_step);
	setVolume(new_vol);
}

void Core::decVolume() {
	qDebug("Core::incVolume");
	int new_vol = (pref->global_volume ? pref->volume - pref->min_step : mset.volume - pref->min_step);
	setVolume(new_vol);
}

void Core::setSubDelay(int delay) {
	qDebug("Core::setSubDelay: %d", delay);
	mset.sub_delay = delay;
	proc->setSubDelay((double) mset.sub_delay/1000);
	displayMessage( tr("Subtitle delay: %1 ms").arg(delay) );
}

void Core::incSubDelay() {
	qDebug("Core::incSubDelay");
	setSubDelay(mset.sub_delay + 100);
}

void Core::decSubDelay() {
	qDebug("Core::decSubDelay");
	setSubDelay(mset.sub_delay - 100);
}

void Core::setAudioDelay(int delay) {
	qDebug("Core::setAudioDelay: %d", delay);
	mset.audio_delay = delay;
	proc->setAudioDelay((double) mset.audio_delay/1000);
	displayMessage( tr("Audio delay: %1 ms").arg(delay) );
}

void Core::incAudioDelay() {
	qDebug("Core::incAudioDelay");
	setAudioDelay(mset.audio_delay + 100);
}

void Core::decAudioDelay() {
	qDebug("Core::decAudioDelay");
	setAudioDelay(mset.audio_delay - 100);
}

void Core::incSubPos() {
	qDebug("Core::incSubPos");

	mset.sub_pos++;
	if (mset.sub_pos > 100) mset.sub_pos = 100;
	proc->setSubPos(mset.sub_pos);
}

void Core::decSubPos() {
	qDebug("Core::decSubPos");

	mset.sub_pos--;
	if (mset.sub_pos < 0) mset.sub_pos = 0;
	proc->setSubPos(mset.sub_pos);
}

void Core::changeSubScale(double value) {
	qDebug("Core::changeSubScale: %f", value);

	if (value < 0) value = 0;

	if (pref->use_ass_subtitles) {
		if (value != mset.sub_scale_ass) {
			mset.sub_scale_ass = value;
			proc->setSubScale(mset.sub_scale_ass);
		}
	} else if (proc->isMPV()) {
		if (value != mset.sub_scale_mpv) {
			mset.sub_scale_mpv = value;
			proc->setSubScale(value);
		}
	} else if (value != mset.sub_scale) {
		mset.sub_scale = value;
		proc->setSubScale(value);
	}

	displayMessage( tr("Font scale: %1").arg(value) );
}

void Core::incSubScale() {
	double step = 0.20;

	if (pref->use_ass_subtitles) {
		changeSubScale( mset.sub_scale_ass + step );
	} else if (proc->isMPV()) {
		changeSubScale( mset.sub_scale_mpv + step );
	} else {
		changeSubScale( mset.sub_scale + step );
	}
}

void Core::decSubScale() {
	double step = 0.20;

	if (pref->use_ass_subtitles) {
		changeSubScale( mset.sub_scale_ass - step );
	} else if (proc->isMPV()) {
		changeSubScale( mset.sub_scale_mpv - step );
	} else {
		changeSubScale( mset.sub_scale - step );
	}
}

void Core::setOSDPos(const QPoint &pos) {
	// qDebug("Core::setOSDPos");

	if (proc->isFullyStarted())
		proc->setOSDPos(pos);
}

void Core::changeOSDScale(double value) {
	qDebug("Core::changeOSDScale: %f", value);

	if (value < 0) value = 0;

	if (proc->isMPlayer()) {
		if (value != pref->subfont_osd_scale) {
			pref->subfont_osd_scale = value;
			restartPlay();
		}
	} else {
		if (value != pref->osd_scale) {
			pref->osd_scale = value;
			proc->setOSDScale(pref->osd_scale);
		}
	}
}

void Core::incOSDScale() {
	if (proc->isMPlayer()) {
		changeOSDScale(pref->subfont_osd_scale + 1);
	} else {
		changeOSDScale(pref->osd_scale + 0.10);
	}
}

void Core::decOSDScale() {
	if (proc->isMPlayer()) {
		changeOSDScale(pref->subfont_osd_scale - 1);
	} else {
		changeOSDScale(pref->osd_scale - 0.10);
	}
}

void Core::incSubStep() {
	qDebug("Core::incSubStep");
	proc->setSubStep(+1);
}

void Core::decSubStep() {
	qDebug("Core::decSubStep");
	proc->setSubStep(-1);
}

void Core::changeExternalSubFPS(int fps_id) {
	qDebug("Core::setExternalSubFPS: %d", fps_id);
	mset.external_subtitles_fps = fps_id;
	if (!mset.external_subtitles.isEmpty()) {
		restartPlay();
	}
}

// Audio equalizer functions
void Core::setAudioEqualizer(AudioEqualizerList values, bool restart) {
	if (pref->global_audio_equalizer) {
		pref->audio_equalizer = values;
	} else {
		mset.audio_equalizer = values;
	}

	if (!restart) {
		proc->setAudioEqualizer(Helper::equalizerListToString(values));
	} else {
		restartPlay();
	}

	// Infinite recursion
	//emit audioEqualizerNeedsUpdate();
}

void Core::updateAudioEqualizer() {
	setAudioEqualizer(pref->global_audio_equalizer ? pref->audio_equalizer : mset.audio_equalizer);
}

void Core::setAudioEq(int eq, int value) {
	if (pref->global_audio_equalizer) {
		pref->audio_equalizer[eq] = value;
	} else {
		mset.audio_equalizer[eq] = value;
	}
	updateAudioEqualizer();
}

void Core::setAudioEq0(int value) {
	setAudioEq(0, value);
}

void Core::setAudioEq1(int value) {
	setAudioEq(1, value);
}

void Core::setAudioEq2(int value) {
	setAudioEq(2, value);
}

void Core::setAudioEq3(int value) {
	setAudioEq(3, value);
}

void Core::setAudioEq4(int value) {
	setAudioEq(4, value);
}

void Core::setAudioEq5(int value) {
	setAudioEq(5, value);
}

void Core::setAudioEq6(int value) {
	setAudioEq(6, value);
}

void Core::setAudioEq7(int value) {
	setAudioEq(7, value);
}

void Core::setAudioEq8(int value) {
	setAudioEq(8, value);
}

void Core::setAudioEq9(int value) {
	setAudioEq(9, value);
}

void Core::changeCurrentSec(double sec) {

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
}

void Core::changePause() {
	qDebug("Core::changePause: player paused at %f", mset.current_sec);

	setState(Paused);
}

void Core::changeDeinterlace(int ID) {
	qDebug("Core::changeDeinterlace: %d", ID);

	if (ID != mset.current_deinterlacer) {
		if (proc->isMPlayer()) {
			mset.current_deinterlacer = ID;
			restartPlay();
		} else {
			// MPV
			// Remove previous filter
			switch (mset.current_deinterlacer) {
				case MediaSettings::L5:			proc->changeVF("l5", false); break;
				case MediaSettings::Yadif:		proc->changeVF("yadif", false); break;
				case MediaSettings::LB:			proc->changeVF("lb", false); break;
				case MediaSettings::Yadif_1:	proc->changeVF("yadif", false, "1"); break;
				case MediaSettings::Kerndeint:	proc->changeVF("kerndeint", false, "5"); break;
			}
			mset.current_deinterlacer = ID;
			// New filter
			switch (mset.current_deinterlacer) {
				case MediaSettings::L5:			proc->changeVF("l5", true); break;
				case MediaSettings::Yadif:		proc->changeVF("yadif", true); break;
				case MediaSettings::LB:			proc->changeVF("lb", true); break;
				case MediaSettings::Yadif_1:	proc->changeVF("yadif", true, "1"); break;
				case MediaSettings::Kerndeint:	proc->changeVF("kerndeint", true, "5"); break;
			}
		}
	}
}

void Core::changeSubtitle(int idx, bool updateWidgets) {
	qDebug("Core::changeSubtitle: idx %d", idx);

	if (idx >= 0 && idx < mdat.subs.numItems()) {
		SubData sub = mdat.subs.itemAt(idx);
		if (sub.ID() != mdat.subs.selectedID()
			|| sub.type() != mdat.subs.selectedType()) {
			mdat.subs.setSelected(sub.type(), sub.ID());
			proc->setSubtitle(sub.type(), sub.ID());
		}
	} else {
		if (mdat.subs.numItems() > 0) {
			proc->disableSubtitles();
		}
		mdat.subs.setSelectedID(-1);
		idx = MediaSettings::SubNone;
	}
	mset.current_sub_idx = idx;

	if (updateWidgets)
		this->updateWidgets();
}

void Core::nextSubtitle() {
	qDebug("Core::nextSubtitle");

	if (mdat.subs.numItems() > 0) {
		int idx = mset.current_sub_idx;
		if (idx < 0) {
			idx = 0;
		} else {
			idx++;
			if (idx >= mdat.subs.numItems()) {
				idx = MediaSettings::SubNone;
			}
		}
		changeSubtitle(idx);
	}
}

#ifdef MPV_SUPPORT
void Core::changeSecondarySubtitle(int idx) {
	// MPV only
	qDebug("Core::changeSecondarySubtitle: %d", idx);

	// Prevent update if requested idx is SubNone and
	if (mset.current_secondary_sub_id == MediaSettings::NoneSelected)
		mset.current_secondary_sub_id = MediaSettings::SubNone;

	// Keep in range
	if (idx < 0 || idx >= mdat.subs.numItems()) {
		idx = MediaSettings::SubNone;
	}

	if (idx != mset.current_secondary_sub_id) {
		mset.current_secondary_sub_id = idx;

		if (idx == MediaSettings::SubNone) {
			proc->disableSecondarySubtitles();
		} else {
			int real_id = mdat.subs.itemAt(idx).ID();
			proc->setSecondarySubtitle(real_id);
		}
	}
}
#endif

void Core::changeAudio(int id, bool allow_restart) {
	qDebug("Core::changeAudio: id: %d, allow_restart: %d", id, allow_restart);

	if (id != mset.current_audio_id) {
		mset.current_audio_id = id;
		if (id >= 0 && id != mdat.audios.selectedID()) {
			if (allow_restart
				&& pref->fast_audio_change == Preferences::Disabled) {
				restartPlay();
			} else {
				mdat.audios.setSelectedID(id);
				proc->setAudio(id);

				// Workaround for a mplayer problem in windows,
				// volume is too loud after changing audio.

				// Workaround too for a mplayer problem in linux,
				// the volume is reduced if using -softvol-max.

				if (proc->isMPlayer()) {
					if (pref->mplayer_additional_options.contains("-volume")) {
						qDebug("Core::changeAudio: don't set volume since -volume is used");
					} else if (pref->global_volume) {
						setVolume( pref->volume, true);
						if (pref->mute) mute(true);
					} else {
						setVolume( mset.volume, true );
						if (mset.mute) mute(true); // if muted, mute again
					}
				}
			}
		}
	}
}

void Core::nextAudio() {
	qDebug("Core::nextAudio");

	if (mdat.audios.numItems() > 0) {
		int idx = mdat.audios.find(mset.current_audio_id);
		if (idx < 0) idx = 0;
		idx++;
		if (idx >= mdat.audios.numItems()) idx = 0;
		int id = mdat.audios.itemAt(idx).ID();
		qDebug( "Core::nextAudio: idx: %d, id: %d", idx, id);
		changeAudio(id);
	}
}

void Core::changeVideo(int id) {
	qDebug("Core::changeVideo: idx: %d", id);

	if (id != mset.current_video_id) {
		mset.current_video_id = id;
		if (id >= 0 && id != mdat.videos.selectedID()) {
			restartPlay();
		}
	}
}

void Core::nextVideo() {
	qDebug("Core::nextVideo");

	if (mdat.videos.numItems() > 0) {
		int idx = mdat.videos.find( mset.current_video_id );
		if (idx < 0) idx = 0;
		idx++;
		if (idx >= mdat.videos.numItems()) idx = 0;
		int id = mdat.videos.itemAt(idx).ID();
		qDebug( "Core::nextVideo: idx: %d, id: %d", idx, id);
		changeVideo(id);
	}
}

#if PROGRAM_SWITCH
void Core::changeProgram(int ID) {
	qDebug("Core::changeProgram: %d", ID);

	if (ID != mset.current_program_id) {
		mset.current_program_id = ID;
		proc->setTSProgram(ID);

		/*
		mset.current_video_id = MediaSettings::NoneSelected;
		mset.current_audio_id = MediaSettings::NoneSelected;

		updateWidgets();
		*/
	}
}

void Core::nextProgram() {
	qDebug("Core::nextProgram");
	// Not implemented yet
}

#endif

void Core::changeTitle(int ID) {
	if (mdat.type == TYPE_VCD) {
		// VCD
		openVCD( ID );
	}
	else 
	if (mdat.type == TYPE_AUDIO_CD) {
		// AUDIO CD
		openAudioCD( ID );
	}
	else
	if (mdat.type == TYPE_DVD) {
		#if DVDNAV_SUPPORT
		if (mdat.filename.startsWith("dvdnav:")) {
			proc->setTitle(ID);
		} else {
		#endif
			DiscData disc_data = DiscName::split(mdat.filename);
			disc_data.title = ID;
			QString dvd_url = DiscName::join(disc_data);

			openDVD( DiscName::join(disc_data) );
		#if DVDNAV_SUPPORT
		}
		#endif
	}
#ifdef BLURAY_SUPPORT
	else
	if (mdat.type == TYPE_BLURAY) {
		//DiscName::test();

		DiscData disc_data = DiscName::split(mdat.filename);
		disc_data.title = ID;
		QString bluray_url = DiscName::join(disc_data);
		qDebug("Core::changeTitle: bluray_url: %s", bluray_url.toUtf8().constData());
		openBluRay(bluray_url);
	}
#endif
}

void Core::changeChapter(int ID) {
	qDebug("Core::changeChapter: ID: %d", ID);

	if (mdat.type != TYPE_DVD) {
		/*
		if (mdat.chapters.find(ID) > -1) {
			double start = mdat.chapters.item(ID).start();
			qDebug("Core::changeChapter: start: %f", start);
			goToSec(start);
			mset.current_chapter_id = ID;
		} else {
		*/
			proc->setChapter(ID);
			mset.current_chapter_id = ID;
			//updateWidgets();
		/*
		}
		*/
	} else {
#if SMART_DVD_CHAPTERS
		if (pref->cache_for_dvds == 0) {
#else
		if (pref->fast_chapter_change) {
#endif
			proc->setChapter(ID);
			mset.current_chapter_id = ID;
			updateWidgets();
		} else {
			stopMplayer();
			mset.current_chapter_id = ID;
			//goToPos(0);
			mset.current_sec = 0;
			restartPlay();
		}
	}
}

int Core::firstChapter() {
	if ( (MplayerVersion::isMplayerAtLeast(25391)) && 
         (!MplayerVersion::isMplayerAtLeast(29407)) ) 
		return 1;
	else
		return 0;
}

int Core::firstDVDTitle() {
	if (proc->isMPV()) {
		return 0;
	} else {
		return 1;
	}
}

int Core::firstBlurayTitle() {
	if (proc->isMPV()) {
		return 0;
	} else {
		return 1;
	}
}

void Core::prevChapter() {
	qDebug("Core::prevChapter");

	int last_chapter = 0;
	int first_chapter = firstChapter();

	int ID = mdat.chapters.itemBeforeTime(mset.current_sec).ID();

	if (ID == -1) {
		last_chapter = mdat.n_chapters + firstChapter() - 1;

		ID = mset.current_chapter_id - 1;
		if (ID < first_chapter) {
			ID = last_chapter;
		}
	}

	changeChapter(ID);
}

void Core::nextChapter() {
	qDebug("Core::nextChapter");

	int last_chapter = mdat.n_chapters + firstChapter() - 1;

	int ID = mdat.chapters.itemAfterTime(mset.current_sec).ID();

	if (ID == -1) {
		ID = mset.current_chapter_id + 1;
		if (ID > last_chapter) {
			ID = firstChapter();
		}
	}

	changeChapter(ID);
}

void Core::changeAngle(int ID) {
	qDebug("Core::changeAngle: ID: %d", ID);

	if (ID != mset.current_angle_id) {
		mset.current_angle_id = ID;
		restartPlay();
	}
}

void Core::changeAspectRatio( int ID ) {
	qDebug("Core::changeAspectRatio: %d", ID);

	mset.aspect_ratio_id = ID;

	double asp = mset.aspectToNum( (MediaSettings::Aspect) ID);

	if (!pref->use_mplayer_window) {
		// Set aspect video window. false: don't update video window
		mplayerwindow->setAspect(asp, false);
		// Resize with new aspect, normally updates video window
		emit needResize(mset.win_width, mset.win_height);
		// Adjust video window if resize canceled
		mplayerwindow->updateVideoWindow();
	} else {
		// Using mplayer own window
		if (!mdat.noVideo()) {
			if (ID == MediaSettings::AspectAuto) {
				asp = mdat.video_aspect;
			}
			proc->setAspect(asp);
		}
	}

	QString asp_name = MediaSettings::aspectToString( (MediaSettings::Aspect) mset.aspect_ratio_id);
	displayMessage( tr("Aspect ratio: %1").arg(asp_name) );
}

void Core::nextAspectRatio() {
	// Ordered list
	QList<int> s;
	s << MediaSettings::AspectNone 
      << MediaSettings::AspectAuto
      << MediaSettings::Aspect11	// 1
      << MediaSettings::Aspect54	// 1.25
      << MediaSettings::Aspect43	// 1.33
      << MediaSettings::Aspect118	// 1.37
      << MediaSettings::Aspect1410	// 1.4
      << MediaSettings::Aspect32	// 1.5
      << MediaSettings::Aspect149	// 1.55
      << MediaSettings::Aspect1610	// 1.6
      << MediaSettings::Aspect169	// 1.77
      << MediaSettings::Aspect235;	// 2.35

	int i = s.indexOf( mset.aspect_ratio_id ) + 1;
	if (i >= s.count()) i = 0;

	int new_aspect_id = s[i];

	changeAspectRatio( new_aspect_id );
	updateWidgets();
}

void Core::nextWheelFunction() {
	int a = pref->wheel_function;

	bool done = false;
	if(((int ) pref->wheel_function_cycle)==0)
		return;
	while(!done){
		// get next a

		a = a*2;
		if(a==32)
			a = 2;
		// See if we are done
		if (pref->wheel_function_cycle.testFlag((Preferences::WheelFunction)a))
			done = true;
	}
	pref->wheel_function = a;
	QString m = "";
	switch(a){
	case Preferences::Seeking:
		m = tr("Mouse wheel seeks now");
		break;
	case Preferences::Volume:
		m = tr("Mouse wheel changes volume now");
		break;
	case Preferences::Zoom:
		m = tr("Mouse wheel changes zoom level now");
		break;
	case Preferences::ChangeSpeed:
		m = tr("Mouse wheel changes speed now");
		break;
	}
	displayMessage(m);
}

void Core::changeLetterbox(bool b) {
	qDebug("Core::changeLetterbox: %d", b);

	if (mset.add_letterbox != b) {
		mset.add_letterbox = b;
		CHANGE_VF("letterbox", b, DesktopInfo::desktop_aspectRatio(mplayerwindow));
	}
}

void Core::changeLetterboxOnFullscreen(bool b) {
	qDebug("Core::changeLetterboxOnFullscreen: %d", b);
	CHANGE_VF("letterbox", b, DesktopInfo::desktop_aspectRatio(mplayerwindow));
}

void Core::changeOSDLevel(int level) {
	qDebug("Core::changeOSDLevel: %d", level);

	pref->osd_level = (Preferences::OSDLevel) level;
	proc->setOSDLevel(level);

	updateWidgets();
}

void Core::nextOSDLevel() {

	int level;
	if (pref->osd_level >= Preferences::SeekTimerTotal) {
		level = Preferences::None;
	} else {
		level = pref->osd_level + 1;
	}
	changeOSDLevel( level );
}

void Core::changeRotate(int r) {
	qDebug("Core::changeRotate: %d", r);

	if (mset.rotate != r) {
		if (proc->isMPlayer()) {
			mset.rotate = r;
			restartPlay();
		} else {
			// MPV
			// Remove previous filter
			switch (mset.rotate) {
				case MediaSettings::Clockwise_flip: proc->changeVF("rotate", false, MediaSettings::Clockwise_flip); break;
				case MediaSettings::Clockwise: proc->changeVF("rotate", false, MediaSettings::Clockwise); break;
				case MediaSettings::Counterclockwise: proc->changeVF("rotate", false, MediaSettings::Counterclockwise); break;
				case MediaSettings::Counterclockwise_flip: proc->changeVF("rotate", false, MediaSettings::Counterclockwise_flip); break;
			}
			mset.rotate = r;
			// New filter
			switch (mset.rotate) {
				case MediaSettings::Clockwise_flip: proc->changeVF("rotate", true, MediaSettings::Clockwise_flip); break;
				case MediaSettings::Clockwise: proc->changeVF("rotate", true, MediaSettings::Clockwise); break;
				case MediaSettings::Counterclockwise: proc->changeVF("rotate", true, MediaSettings::Counterclockwise); break;
				case MediaSettings::Counterclockwise_flip: proc->changeVF("rotate", true, MediaSettings::Counterclockwise_flip); break;
			}
		}
	}
}

#if USE_ADAPTER
void Core::changeAdapter(int n) {
	qDebug("Core::changeScreen: %d", n);

	if (pref->adapter != n) {
		pref->adapter = n;
		restartPlay();
	}
}
#endif

void Core::forceResize() {

	// Overide user setting
	int old_resize_method = pref->resize_method;
	pref->resize_method = Preferences::Always;
	emit needResize(mset.win_width, mset.win_height);
	pref->resize_method = old_resize_method;
}

void Core::changeSize(int percentage) {
	qDebug("Core::changeSize");

	if (!pref->use_mplayer_window && !pref->fullscreen) {
		pref->size_factor = (double) percentage / 100;
		forceResize();
		displayMessage( tr("Size %1%").arg(QString::number(percentage)) );
	}
}

void Core::getZoomFromMplayerWindow() {
	mset.zoom_factor = mplayerwindow->zoomNormalScreen();
	mset.zoom_factor_fullscreen = mplayerwindow->zoomFullScreen();
}

void Core::getPanFromMplayerWindow() {
	mset.pan_offset = mplayerwindow->panNormalScreen();
	mset.pan_offset_fullscreen = mplayerwindow->panFullScreen();
}

void Core::changeZoom(double factor) {
	qDebug("Core::changeZoom: %f", factor);

	// Kept between min and max by mplayerwindow->setZoom()
	// Hence reread of factors
	mplayerwindow->setZoom(factor);
	getZoomFromMplayerWindow();

	displayMessage( tr("Zoom: %1").arg(mplayerwindow->zoom()) );
}

void Core::resetZoomAndPan() {
	qDebug("Core::resetZoomAndPan");

	// Reset zoom and pan of video window
	mplayerwindow->resetZoomAndPan();
	// Reread modified settings
	getZoomFromMplayerWindow();
	getPanFromMplayerWindow();

	displayMessage( tr("Zoom and pan reset") );
}

void Core::pan(int dx, int dy) {
	qDebug("Core::pan");

	mplayerwindow->moveVideo(dx, dy);
	getPanFromMplayerWindow();

	QPoint current_pan = mplayerwindow->pan();
	displayMessage( tr("Pan (%1, %2)").arg(QString::number(current_pan.x())).arg(QString::number(current_pan.y())) );
}

void Core::panLeft() {

	pan( PAN_STEP, 0 );
}

void Core::panRight() {
	pan( -PAN_STEP, 0 );
}

void Core::panUp() {
	pan( 0, PAN_STEP );
}

void Core::panDown() {
	pan( 0, -PAN_STEP );
}

void Core::autoZoom() {
	double video_aspect = mset.aspectToNum( (MediaSettings::Aspect) mset.aspect_ratio_id);

	if (video_aspect <= 0) {
		QSize w = mplayerwindow->videoLayer()->size();
		video_aspect = (double) w.width() / w.height();
	}

	double screen_aspect = DesktopInfo::desktop_aspectRatio(mplayerwindow);
	double zoom_factor;

	if (video_aspect > screen_aspect)
		zoom_factor = video_aspect / screen_aspect;
	else
		zoom_factor = screen_aspect / video_aspect;

	qDebug("Core::autoZoom: video_aspect: %f", video_aspect);
	qDebug("Core::autoZoom: screen_aspect: %f", screen_aspect);
	qDebug("Core::autoZoom: zoom_factor: %f", zoom_factor);

	changeZoom(zoom_factor);
}

void Core::autoZoomFromLetterbox(double aspect) {
	qDebug("Core::autoZoomFromLetterbox: %f", aspect);

	// Probably there's a much easy way to do this, but I'm not good with maths...

	QSize desktop =  DesktopInfo::desktop_size(mplayerwindow);

	double video_aspect = mset.aspectToNum( (MediaSettings::Aspect) mset.aspect_ratio_id);

	if (video_aspect <= 0) {
		QSize w = mplayerwindow->videoLayer()->size();
		video_aspect = (double) w.width() / w.height();
	}

	// Calculate size of the video in fullscreen
	QSize video;
	video.setHeight( desktop.height() );;
	video.setWidth( (int) (video.height() * video_aspect) );
	if (video.width() > desktop.width()) {
		video.setWidth( desktop.width() );;
		video.setHeight( (int) (video.width() / video_aspect) );
	}

	qDebug("Core::autoZoomFromLetterbox: max. size of video: %d %d", video.width(), video.height());

	// Calculate the size of the actual video inside the letterbox
	QSize actual_video;
	actual_video.setWidth( video.width() );
	actual_video.setHeight( (int) (actual_video.width() / aspect) );

	qDebug("Core::autoZoomFromLetterbox: calculated size of actual video for aspect %f: %d %d", aspect, actual_video.width(), actual_video.height());

	double zoom_factor = (double) desktop.height() / actual_video.height();

	qDebug("Core::autoZoomFromLetterbox: calculated zoom factor: %f", zoom_factor);
	changeZoom(zoom_factor);	
}

void Core::autoZoomFor169() {
	autoZoomFromLetterbox((double) 16 / 9);
}

void Core::autoZoomFor235() {
	autoZoomFromLetterbox(2.35);
}

void Core::incZoom() {
	qDebug("Core::incZoom");
	changeZoom(mplayerwindow->zoom() + ZOOM_STEP );
}

void Core::decZoom() {
	qDebug("Core::decZoom");
	changeZoom(mplayerwindow->zoom() - ZOOM_STEP );
}

void Core::showFilenameOnOSD() {
	proc->showFilenameOnOSD();
}

void Core::showTimeOnOSD() {
	proc->showTimeOnOSD();
}

void Core::toggleDeinterlace() {
	qDebug("Core::toggleDeinterlace");
	proc->toggleDeinterlace();
}

void Core::changeUseCustomSubStyle(bool b) {
	qDebug("Core::changeUseCustomSubStyle: %d", b);

	if (pref->enable_ass_styles != b) {
		pref->enable_ass_styles = b;
		if (proc->isRunning()) restartPlay();
	}
}

void Core::toggleForcedSubsOnly(bool b) {
	qDebug("Core::toggleForcedSubsOnly: %d", b);

	if (pref->use_forced_subs_only != b) {
		pref->use_forced_subs_only = b;
		//if (proc->isRunning()) restartPlay();
		proc->setSubForcedOnly(b);
	}
}

void Core::changeClosedCaptionChannel(int c) {
	qDebug("Core::changeClosedCaptionChannel: %d", c);
	if (c != mset.closed_caption_channel) {
		mset.closed_caption_channel = c;
		if (proc->isRunning()) restartPlay();
	}
}

/*
void Core::nextClosedCaptionChannel() {
	int c = mset.closed_caption_channel;
	c++;
	if (c > 4) c = 0;
	changeClosedCaptionChannel(c);
}

void Core::prevClosedCaptionChannel() {
	int c = mset.closed_caption_channel;
	c--;
	if (c < 0) c = 4;
	changeClosedCaptionChannel(c);
}
*/

#if DVDNAV_SUPPORT
// dvdnav buttons
void Core::dvdnavUp() {
	qDebug("Core::dvdnavUp");
	proc->discButtonPressed("up");
}

void Core::dvdnavDown() {
	qDebug("Core::dvdnavDown");
	proc->discButtonPressed("down");
}

void Core::dvdnavLeft() {
	qDebug("Core::dvdnavLeft");
	proc->discButtonPressed("left");
}

void Core::dvdnavRight() {
	qDebug("Core::dvdnavRight");
	proc->discButtonPressed("right");
}

void Core::dvdnavMenu() {
	qDebug("Core::dvdnavMenu");
	proc->discButtonPressed("menu");
}

void Core::dvdnavSelect() {
	qDebug("Core::dvdnavSelect");
	proc->discButtonPressed("select");
}

void Core::dvdnavPrev() {
	qDebug("Core::dvdnavPrev");
	proc->discButtonPressed("prev");
}

void Core::dvdnavMouse() {
	qDebug("Core::dvdnavMouse");

	if ((state() == Playing) && (mdat.filename.startsWith("dvdnav:"))) {
		proc->discButtonPressed("mouse");
	}
}
#endif

void Core::displayMessage(QString text, int duration, int osd_level) {
	//qDebug("Core::displayMessage");

	emit showMessage(text);
	displayTextOnOSD(text, duration, osd_level);
}

void Core::displayScreenshotName(QString filename) {
	qDebug("Core::displayScreenshotName");

	//QString text = tr("Screenshot saved as %1").arg(filename);
	QString text = QString("Screenshot saved as %1").arg(filename);
	displayMessage(text);
}

void Core::displayUpdatingFontCache() {
	qDebug("Core::displayUpdatingFontCache");
	emit showMessage( tr("Updating the font cache. This may take some seconds...") );
}

void Core::displayBuffering() {
	emit showMessage(tr("Buffering..."));
}

void Core::gotVideoOutResolution(int w, int h) {
	qDebug("Core::gotVideoOutResolution: %d x %d", w, h);

	// w x h should be the output resolution selected by player with
	// aspect and filters applied.
	mset.win_width = w;
	mset.win_height = h;
	mplayerwindow->setResolution(w, h);

	if (pref->use_mplayer_window || w <= 0) {
		emit noVideo();
	} else {
		if (mset.aspect_ratio_id == MediaSettings::AspectAuto) {
			// Set aspect to w/h. false = do not update video window.
			mplayerwindow->setAspect(mset.win_aspect(), false);
		}

		emit needResize(w, h);

		// If resize is canceled adjust new video to old size
		mplayerwindow->updateVideoWindow();
	}
}

void Core::gotVO(QString vo) {
	qDebug("Core::gotVO: '%s'", vo.toUtf8().data() );

	if ( pref->vo.isEmpty()) {
		qDebug("Core::gotVO: saving vo");
		pref->vo = vo;
	}
}

void Core::gotAO(QString ao) {
	qDebug("Core::gotAO: '%s'", ao.toUtf8().data() );

	if ( pref->ao.isEmpty()) {
		qDebug("Core::gotAO: saving ao");
		pref->ao = ao;
	}
}

void Core::streamTitleChanged(QString title) {
	mdat.stream_title = title;
	emit mediaInfoChanged();
}

void Core::streamTitleAndUrlChanged(QString title, QString url) {
	mdat.stream_title = title;
	mdat.stream_url = url;
	emit mediaInfoChanged();
}

void Core::sendMediaInfo() {
	qDebug("Core::sendMediaInfo");
	emit mediaPlaying(mdat.filename, mdat.displayName(pref->show_tag_in_window_title));
}

//!  Called when the state changes
void Core::watchState(Core::State state) {
#ifdef SCREENSAVER_OFF
	#if 0
	qDebug("Core::watchState: %d", state);
	//qDebug("Core::watchState: has video: %d", !mdat.novideo);

	if ((state == Playing) /* && (!mdat.novideo) */) {
		disableScreensaver();
	} else {
		enableScreensaver();
	}
	#endif
#endif

	if ((proc->isMPlayer()) && (state == Playing) && (change_volume_after_unpause)) {
		// Delayed volume change
		qDebug("Core::watchState: delayed volume change");
		int volume = (pref->global_volume ? pref->volume : mset.volume);
		proc->setVolume(volume);
		change_volume_after_unpause = false;
	}
}

void Core::checkIfVideoIsHD() {
	qDebug("Core::checkIfVideoIsHD");

	// Check if the video is in HD and uses ffh264 codec.
	if ((mdat.video_codec=="ffh264") && (mset.win_height >= pref->HD_height)) {
		qDebug("Core::checkIfVideoIsHD: video == ffh264 and height >= %d", pref->HD_height);
		if (!mset.is264andHD) {
			mset.is264andHD = true;
			if (pref->h264_skip_loop_filter == Preferences::LoopDisabledOnHD) {
				qDebug("Core::checkIfVideoIsHD: we're about to restart the video");
				restartPlay();
			}
		}
	} else {
		mset.is264andHD = false;
		// FIXME: if the video was previously marked as HD, and now it's not
		// then the video should restart too.
	}
}

// Called when playerprocess finds new track info or changes selected track
// after player full loaded
void Core::updateVideoTracks() {
	qDebug("Core::updateTracks");

	initVideoTracks();

	// TODO: move to basegui?
	initializeMenus();
	updateWidgets();
}

// Called when playerprocess finds new track info or changes selected track
// after player full loaded, like when loading new external audio file.
void Core::updateAudioTracks() {
	qDebug("Core::updateTracks");

	initAudioTracks();

	// TODO: move to basegui?
	initializeMenus();
	updateWidgets();

	emit audioTracksChanged();
}

// Called after loading new external sub file
void Core::updateSubtitleTracks() {
	qDebug("Core::updateSubtitleTracks");

	initSubs();

	// TODO: move to basegui?
	initializeMenus();
	updateWidgets();
}

#if DVDNAV_SUPPORT
void Core::dvdTitleChanged(int title) {
	qDebug("Core::dvdTitleChanged: %d", title);
}

void Core::dvdnavUpdateMousePos(QPoint pos) {
	// bool under_mouse = mplayerwindow->videoLayer()->underMouse();
#if 0
	qDebug("Core::dvdnavUpdateMousePos: %d %d", pos.x(), pos.y());
	qDebug("Core::dvdnavUpdateMousePos: state: %d", state());
	qDebug("Core::dvdnavUpdateMousePos: filename: %s", mdat.filename.toUtf8().constData());
	qDebug("Core::dvdnavUpdateMousePos: dvdnav_title_is_menu: %d", dvdnav_title_is_menu);
	qDebug("Core::dvdnavUpdateMousePos: under mouse: %d", under_mouse);
#endif
	if ((state() == Playing) && (mdat.filename.startsWith("dvdnav:")) && (dvdnav_title_is_menu)) {
		//if (under_mouse) {
			proc->discSetMousePos(pos.x(), pos.y());
		//}
	}
}

void Core::dvdTitleIsMenu() {
	qDebug("Core::dvdTitleIsMenu");
	dvdnav_title_is_menu = true;
}

void Core::dvdTitleIsMovie() {
	qDebug("Core::dvdTitleIsMovie");
	dvdnav_title_is_menu = false;
}
#endif

#include "moc_core.cpp"
