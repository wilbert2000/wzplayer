/*  WZPlayer, GUI front-end for mplayer and MPV.
    Parts copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

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

#include "player/player.h"
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

#include "discname.h"
#include "mediadata.h"
#include "extensions.h"
#include "colorutils.h"
#include "helper.h"
#include "settings/paths.h"
#include "settings/aspectratio.h"
#include "settings/mediasettings.h"
#include "settings/preferences.h"
#include "settings/filesettings.h"
#include "settings/filesettingshash.h"
#include "settings/tvsettings.h"
#include "settings/filters.h"

#include "player/process/playerprocess.h"
#include "player/process/exitmsg.h"
#include "gui/playerwindow.h"

#include "gui/desktop.h"
#include "gui/msg.h"
#include "gui/action/tvlist.h"


using namespace Settings;

Player::TPlayer* player = 0;

namespace Player {

double TPlayer::restartTime = 0;

TPlayer::TPlayer(QWidget* parent, Gui::TPlayerWindow* pw) :
    QObject(parent),
    debug(logger()),
    mdat(),
    mset(&mdat),
    keepSize(false),
    playerwindow(pw),
    _state(STATE_LOADING) {

    //qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
    qRegisterMetaType<TState>("Player::TState");

    player = this;

    keepSizeTimer = new QTimer(this);
    keepSizeTimer->setInterval(1000);
    keepSizeTimer->setSingleShot(true);
    connect(keepSizeTimer, SIGNAL(timeout()), this, SLOT(clearKeepSize()));

    proc = Player::Process::TPlayerProcess::createPlayerProcess(this, &mdat);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    connect(proc, SIGNAL(errorOccurred(QProcess::ProcessError)),
            this, SLOT(onProcessError(QProcess::ProcessError)));
#else
    connect(proc, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(onProcessError(QProcess::ProcessError)));
#endif

    connect(proc, SIGNAL(processFinished(bool, int, bool)),
            this, SLOT(onProcessFinished(bool, int, bool)));

    connect(proc, SIGNAL(playerFullyLoaded()),
            this, SLOT(playingStarted()));

    connect(proc, SIGNAL(receivedPosition(double)),
            this, SLOT(onReceivedPosition(double)));

    connect(proc, SIGNAL(receivedPause()),
            this, SLOT(onReceivedPause()));

    connect(proc, SIGNAL(receivedBuffering()),
            this, SLOT(displayBuffering()));

    connect(proc, SIGNAL(receivedBufferingEnded()),
            this, SLOT(displayBufferingEnded()));

    connect(proc, SIGNAL(receivedMessage(const QString&)),
            this, SLOT(onReceivedMessage(const QString&)));

    connect(proc, SIGNAL(receivedScreenshot(const QString&)),
            this, SLOT(displayScreenshotName(const QString&)));

    connect(proc, SIGNAL(receivedUpdatingFontCache()),
            this, SLOT(displayUpdatingFontCache()));

    connect(proc, SIGNAL(receivedVideoOut()),
            this, SLOT(onReceivedVideoOut()));

    connect(proc, SIGNAL(receivedStreamTitle()),
            this, SIGNAL(mediaInfoChanged()));

    connect(proc, SIGNAL(receivedVideoTracks()),
            this, SIGNAL(videoTracksChanged()));
    connect(proc, SIGNAL(receivedVideoTrackChanged(int)),
            this, SIGNAL(videoTrackChanged(int)));

    connect(proc, SIGNAL(receivedAudioTracks()),
            this, SLOT(onAudioTracksChanged()));
    connect(proc, SIGNAL(receivedAudioTrackChanged(int)),
            this, SIGNAL(audioTrackChanged(int)));

    connect(proc, SIGNAL(receivedSubtitleTracks()),
            this, SLOT(onSubtitlesChanged()));
    connect(proc, SIGNAL(receivedSubtitleTrackChanged()),
            this, SLOT(onSubtitleChanged()));

    connect(proc, SIGNAL(receivedTitleTracks()),
            this, SIGNAL(titleTracksChanged()));
    connect(proc, SIGNAL(receivedTitleTrackChanged(int)),
            this, SIGNAL(titleTrackChanged(int)));

    connect(proc, SIGNAL(receivedChapters()),
            this, SIGNAL(chaptersChanged()));

    connect(proc, SIGNAL(receivedAngles()),
            this, SIGNAL(anglesChanged()));

    connect(proc, SIGNAL(durationChanged(double)),
            this, SIGNAL(durationChanged(double)));

    connect(proc, SIGNAL(videoBitRateChanged(int)),
            this, SIGNAL(videoBitRateChanged(int)));

    connect(proc, SIGNAL(audioBitRateChanged(int)),
            this, SIGNAL(audioBitRateChanged(int)));
}

TPlayer::~TPlayer() {

    player = 0;
}

void TPlayer::onProcessError(QProcess::ProcessError error) {
    logger()->error("onProcessError: %1", error);

    // Restore normal window background
    playerwindow->restoreNormalWindow(false);
    emit playerError(Player::Process::TExitMsg::processErrorToErrorID(error));
}

void TPlayer::onProcessFinished(bool normal_exit, int exit_code, bool eof) {
    logger()->debug("onProcessFinished: normal exit %1, exit code %2, eof %3",
           normal_exit, exit_code, eof);

    // Restore normal window background
    playerwindow->restoreNormalWindow(false);

    if (_state == STATE_STOPPING) {
        return;
    }

    logger()->debug("onProcessFinished: entering the stopped state");
    setState(STATE_STOPPED);

    if (eof || exit_code == Player::Process::TExitMsg::EXIT_OUT_POINT_REACHED) {
        logger()->debug("onProcessFinished: emit mediaEOF()");
        emit mediaEOF();
    } else if (!normal_exit) {
        logger()->debug("onProcessFinished: emit playerError()");
        emit playerError(exit_code);
    }
}

void TPlayer::onReceivedMessage(const QString& s) {

    if (!mdat.image) {
        Gui::msg2(s);
    }
}

void TPlayer::setState(TState s) {

    if (s != _state) {
        _state = s;
        logger()->debug("setState: state set to %1 at %2",
                        stateToString(), QString::number(mset.current_sec));
        emit stateChanged(_state);
    }
}

QString TPlayer::stateToString(TState state) {

    switch (state) {
        case STATE_STOPPED: return tr("Stopped");
        case STATE_PLAYING: return tr("Playing");
        case STATE_PAUSED: return tr("Paused");
        case STATE_STOPPING: return tr("Stopping");
        case STATE_RESTARTING: return tr("Restarting");
        case STATE_LOADING: return tr("Loading");
        default: return tr("Undefined");
    }
}

QString TPlayer::stateToString() const {
    return stateToString(_state);
}

void TPlayer::saveMediaSettings() {

    if (!pref->remember_media_settings) {
        logger()->debug("saveMediaSettings: save settings per file is disabled");
        return;
    }
    if (mdat.filename.isEmpty()) {
        logger()->debug("saveMediaSettings: nothing to save");
        return;
    }
    logger()->info("saveMediaSettings: saving settings for " + mdat.filename);
    Gui::msg(tr("Saving settings for %1").arg(mdat.filename), 0);

    if (mdat.selected_type == TMediaData::TYPE_FILE) {
        if (pref->file_settings_method.toLower() == "hash") {
            Settings::TFileSettingsHash settings(mdat.filename);
            settings.saveSettingsFor(mdat.filename, mset);
        } else {
            Settings::TFileSettings settings;
            settings.saveSettingsFor(mdat.filename, mset);
        }
    } else if (mdat.selected_type == TMediaData::TYPE_TV) {
        Settings::TTVSettings settings;
        settings.saveSettingsFor(mdat.filename, mset);
    }

    Gui::msg(tr("Saved settings for %1").arg(mdat.filename));
} // saveMediaSettings

void TPlayer::clearOSD() {
    logger()->debug("clearOSD");
    displayTextOnOSD("", 0, pref->osd_level);
}

void TPlayer::displayTextOnOSD(const QString& text, int duration, int level) {

    if (proc->isReady() && level <= pref->osd_level && mdat.hasVideo()) {
        proc->showOSDText(text, duration, level);
    }
}

void TPlayer::close(TState next_state) {
    logger()->debug("close");

    stopPlayer();
    // Save data previous file:
    saveMediaSettings();
    // Set state
    setState(next_state);
    // Clear media data
    mdat = TMediaData();
    logger()->debug("close: closed in state " + stateToString());
}

void TPlayer::openDisc(TDiscName disc, bool fast_open) {
    logger()->debug("openDisc: " + disc.toString()
                  + " fast open " + QString::number(fast_open));

    // Change title if already playing
    if (fast_open
        && !mset.playing_single_track
        && statePOP()
        && disc.title > 0
        && mdat.disc.valid
        && mdat.disc.device == disc.device) {
        // If changeTitle fails, it will call again with fast_open set to false
        logger()->debug("openDisc: trying changeTitle(%1)", disc.title);
        changeTitle(disc.title);
        return;
    }

    Gui::msg(tr("Opening %1").arg(disc.toString()), 0);

    // Add device from pref if none specified
    if (disc.device.isEmpty()) {
        if (disc.protocol == "vcd" || disc.protocol == "cdda") {
            disc.device = pref->cdrom_device;
        } else if (disc.protocol == "br") {
            disc.device = pref->bluray_device;
        } else {
            disc.device = pref->dvd_device;
        }
    }

    // Test access, correct missing /
    if (!QFileInfo(disc.device).exists()) {
        logger()->warn("openDisc: could not access '%1'", disc.device);
        // Forgot a /?
        if (QFileInfo("/" + disc.device).exists()) {
            logger()->warn("openDisc: adding missing /");
            disc.device = "/" + disc.device;
        } else {
            Gui::msg(tr("Device or file not found: '%1'").arg(disc.device), 0);
            return;
        }
    }

    // Finish current
    close(STATE_LOADING);

    // Set filename, selected type and disc
    mdat.filename = disc.toString();
    mdat.selected_type = (TMediaData::Type) disc.disc();
    mdat.disc = disc;

    // Clear settings
    mset.reset();

    // Single CD track needs separate treatment for chapters and fast open
    if (mdat.disc.title > 0 && TMediaData::isCD(mdat.selected_type)) {
        mset.playing_single_track = true;
    }

    initPlaying();
    return;
} // openDisc

// Generic open, autodetect type
void TPlayer::open(QString filename, bool loopImage) {
    logger()->debug("open: " + filename);

    if (filename.isEmpty()) {
        filename = mdat.filename;
        if (filename.isEmpty()) {
            Gui::msg(tr("No file to play"));
            return;
        }
    }

    QUrl url(filename);
    QString scheme = url.scheme().toLower();
    if (scheme == "file") {
        filename = url.toLocalFile();
        scheme = "";
    }

    QFileInfo fi(filename);

    // Check for Windows .lnk shortcuts
    if (fi.isSymLink() && fi.suffix().toLower() == ".lnk") {
        filename = fi.symLinkTarget();
    }

    TDiscName disc(filename);
    if (disc.valid) {
        openDisc(disc, true);
        return;
    }
    // Forget a previous disc
    mdat.disc.valid = false;

    Gui::msg(tr("Opening %1").arg(filename), 0);

    if (fi.exists()) {
        filename = fi.absoluteFilePath();

        // Subtitle file?
        QRegExp ext_sub(extensions.subtitles().forRegExp(), Qt::CaseInsensitive);
        if (ext_sub.indexIn(fi.suffix()) >= 0) {
            loadSub(filename);
            return;
        }

        if (fi.isDir()) {
            if (Helper::directoryContainsDVD(filename)) {
                logger()->debug("open: directory contains a dvd");
                disc = TDiscName(filename, pref->useDVDNAV());
                openDisc(disc);
                return;
            }
            logger()->error("open: file is a directory, use playlist to open it");
            return;
        }

        // Local file
        openFile(filename, loopImage);
        return;
    }

    // File does not exist
    if (scheme == "tv" || scheme == "dvb") {
        openTV(filename);
    } else {
        openStream(filename);
    }
}

void TPlayer::setExternalSubs(const QString &filename) {

    mset.current_sub_set_by_user = true;
    mset.current_sub_idx = TMediaSettings::NoneSelected;
    mset.sub.setID(TMediaSettings::NoneSelected);
    mset.sub.setType(SubData::File);
    // For mplayer assume vob if file extension idx
    if (pref->isMPlayer()) {
        QFileInfo fi(filename);
        if (fi.suffix().toLower() == "idx") {
            mset.sub.setType(SubData::Vob);
        }
    }
    mset.sub.setFilename(filename);
}

void TPlayer::loadSub(const QString & sub) {
    logger()->debug("loadSub");

    if (!sub.isEmpty() && QFile::exists(sub)) {
        setExternalSubs(sub);
        if (mset.external_subtitles_fps != TMediaSettings::SFPS_None) {
            restartPlay();
        } else {
            proc->setExternalSubtitleFile(sub);
        }
    } else {
        logger()->warn("loadSub: file not found '" + sub + "'");
    }
}

bool TPlayer::hasExternalSubs() const {
    return mdat.subs.hasFileSubs()
        || (mset.sub.type() == SubData::Vob && !mset.sub.filename().isEmpty());
}

void TPlayer::unloadSub() {

    mset.current_sub_set_by_user = false;
    mset.current_sub_idx = TMediaSettings::NoneSelected;
    mset.sub = SubData();

    restartPlay();
}

void TPlayer::loadAudioFile(const QString& audiofile) {

    if (!audiofile.isEmpty()) {
        mset.external_audio = audiofile;
        restartPlay();
    }
}

void TPlayer::unloadAudioFile() {

    if (!mset.external_audio.isEmpty()) {
        mset.external_audio = "";
        restartPlay();
    }
}

void TPlayer::openTV(QString channel_id) {
    logger()->debug("openTV: %1", channel_id);

    close(STATE_LOADING);

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
    if (pref->remember_media_settings) {
        Settings::TTVSettings settings;
        if (settings.existSettingsFor(channel_id)) {
            settings.loadSettingsFor(channel_id, mset);
        }
    }

    initPlaying();
}

void TPlayer::openStream(const QString& name) {
    logger()->debug("openStream: " + name);

    close(STATE_LOADING);
    mdat.filename = name;
    mdat.selected_type = TMediaData::TYPE_STREAM;
    mset.reset();

    initPlaying();
}

void TPlayer::openFile(const QString& filename, bool loopImage) {
    logger()->debug("openFile: '" + filename + "'");

    close(STATE_LOADING);
    mdat.filename = QDir::toNativeSeparators(filename);
    mdat.selected_type = TMediaData::TYPE_FILE;
    mdat.image = extensions.isImage(mdat.filename);
    mset.reset();

    // Check if we have info about this file
    if (pref->remember_media_settings) {
        if (pref->file_settings_method.toLower() == "hash") {
            Settings::TFileSettingsHash settings(mdat.filename);
            if (settings.existSettingsFor(mdat.filename)) {
                settings.loadSettingsFor(mdat.filename, mset);
            }
        } else {
            Settings::TFileSettings settings;
            if (settings.existSettingsFor(mdat.filename)) {
                settings.loadSettingsFor(mdat.filename, mset);
            }
        }

        if (!pref->remember_time_pos) {
            mset.current_sec = 0;
            logger()->debug("openFile: Time pos reset to 0");
        }
    }

    initPlaying(loopImage);
}

void TPlayer::restartPlay() {
    logger()->debug("restartPlay");

    // Save state proc, currently only used by TMPlayerProcess for DVDNAV
    proc->save();
    stopPlayer();

    logger()->debug("restartPlay: entering the restarting state");
    setState(STATE_RESTARTING);
    initPlaying();
}

// Public restart
void TPlayer::restart() {
    logger()->debug("restart");

    if (proc->isReady()) {
        restartPlay();
    } else {
        logger()->debug("restart: player not ready");
    }
}

void TPlayer::reload() {
    logger()->debug("reload");

    stopPlayer();
    logger()->debug("reload: entering the loading state");
    setState(STATE_LOADING);
    initPlaying();
}

void TPlayer::initVolume() {

    // Keep currrent volume if no media settings are loaded.
    // restore_volume is set to true by mset.reset and set
    // to false by mset.load
    if (mset.restore_volume) {
        logger()->debug("initVolume: keeping current volume");
        mset.volume = mset.old_volume;
        mset.mute = mset.old_mute;
    } else if (!pref->global_volume) {
        if (mset.old_volume != mset.volume) {
            logger()->debug("initVolume: emit volumeChanged()");
            emit volumeChanged(mset.volume);
        }
        if (mset.old_mute != mset.mute) {
            logger()->debug("initVolume: emit muteChanged()");
            emit muteChanged(mset.mute);
        }
    }
}

void TPlayer::initMediaSettings() {
    logger()->debug("initMediaSettings");

    // Restore old volume or emit new volume
    initVolume();

    // Apply settings to playerwindow, false = do not update video window
    playerwindow->setZoom(mset.zoom_factor, mset.zoom_factor_fullscreen, false);
    playerwindow->setPan(mset.pan_offset, mset.pan_offset_fullscreen);

    emit mediaSettingsChanged();
}

void TPlayer::initPlaying(bool loopImages) {
    logger()->debug("initPlaying: starting time");

    time.start();

    if (_state != STATE_RESTARTING) {
        // Clear background
        playerwindow->repaint();
        initMediaSettings();
    }

    startPlayer(mdat.filename, loopImages);
}

void TPlayer::playingStartedNewMedia() {
    logger()->debug("playingStartedNewMedia");

    mdat.list();

    // Copy the demuxer
    mset.current_demuxer = mdat.demuxer;
    if (pref->remember_media_settings) {
        mset.list();
    }

    logger()->debug("playingStartedNewMedia: emit newMediaStartedPlaying()");
    emit newMediaStartedPlaying();
}

// Slot called when signal playerFullyLoaded arrives.
void TPlayer::playingStarted() {
    logger()->debug("playingStarted");

    if (forced_titles.contains(mdat.filename)) {
        mdat.title = forced_titles[mdat.filename];
    }

    if (_state != STATE_RESTARTING) {
        playingStartedNewMedia();
    }

    setState(STATE_PLAYING);

    logger()->debug("playingStarted: emit mediaInfoChanged()");
    emit mediaInfoChanged();

    logger()->debug("playingStarted: done in %1 ms", time.elapsed());
}

void TPlayer::stopPlayer() {

    if (proc->state() == QProcess::NotRunning) {
        logger()->debug("stopPlayer: player not running");
        return;
    }

    logger()->debug("stopPlayer: entering the stopping state");
    setState(STATE_STOPPING);

    // If set high enough the OS will detect the "not responding state" and
    // popup a dialog
    int timeout = pref->time_to_kill_player;

#ifdef Q_OS_OS2
    QEventLoop eventLoop;

    connect(proc, SIGNAL(processFinished(bool, int, bool)),
            &eventLoop, SLOT(quit()));

    proc->quit(0);

    QTimer::singleShot(timeout, &eventLoop, SLOT(quit()));
    eventLoop.exec(QEventLoop::ExcludeUserInputEvents);

    if (proc->isRunning()) {
        logger()->warn("stopPlayer: player didn't finish. Killing it...");
        proc->kill();
    }
#else
    proc->quit(0);

    logger()->debug("stopPlayer: Waiting %1 ms for player to quit...", timeout);
    if (!proc->waitForFinished(timeout)) {
        logger()->warn("stopPlayer: player process did not finish in %1 ms."
                       " Killing it...", timeout);
        proc->kill();
    }
#endif

    logger()->debug("stopPlayer: done");
}

void TPlayer::stop() {
    logger()->debug("stop: current state: %1", stateToString());

    stopPlayer();
    logger()->debug("stop: entering the stopped state");
    setState(STATE_STOPPED);
    emit mediaStopped();
}

void TPlayer::play() {
    logger()->debug("play: current state: %1", stateToString());

    switch (proc->state()) {
        case QProcess::Running:
            if (_state == STATE_PAUSED) {
                proc->setPause(false);
                setState(STATE_PLAYING);
            }
            break;
        case QProcess::NotRunning:
            if (mdat.filename.isEmpty()) {
                emit noFileToPlay();
            } else {
                restartPlay();
            }
            break;
        case QProcess::Starting:
            break;
        default:
            break;
    }
}

void TPlayer::pause() {
    logger()->debug("pause: current state: %1", stateToString());

    if (proc->isRunning() && _state != STATE_PAUSED) {
        proc->setPause(true);
        setState(STATE_PAUSED);
    }
}

void TPlayer::playOrPause() {
    logger()->debug("playOrPause: current state: %1", stateToString());

    if (_state == STATE_PLAYING) {
        pause();
    } else {
        play();
    }
}

void TPlayer::frameStep() {
    logger()->debug("frameStep at %1", mset.current_sec);

    if (proc->isRunning()) {
        if (_state == STATE_PAUSED) {
            proc->frameStep();
        } else {
            pause();
        }
    }
}

void TPlayer::frameBackStep() {
    logger()->debug("frameBackStep at %1", mset.current_sec);

    if (proc->isRunning()) {
        if (_state == STATE_PAUSED) {
            proc->frameBackStep();
        } else {
            pause();
        }
    }
}

void TPlayer::screenshot() {
    logger()->debug("screenshot");

    if (pref->use_screenshot && !pref->screenshot_directory.isEmpty()) {
        proc->takeScreenshot(Player::Process::TPlayerProcess::Single,
                             pref->subtitles_on_screenshots);
        logger()->debug("screenshot: took screenshot");
    } else {
        logger()->warn("screenshot: directory for screenshots not valid or enabled");
        Gui::msg(tr("Screenshot NOT taken, folder not configured or enabled"));
    }
}

void TPlayer::screenshots() {
    logger()->debug("screenshots");

    if (pref->use_screenshot && !pref->screenshot_directory.isEmpty()) {
        proc->takeScreenshot(Player::Process::TPlayerProcess::Multiple,
                             pref->subtitles_on_screenshots);
    } else {
        logger()->warn("screenshots: directory for screenshots not valid or enabled");
        Gui::msg(tr("Screenshots NOT taken, folder not configured or enabled"));
    }
}

void TPlayer::switchCapturing() {
    logger()->debug("switchCapturing");
    proc->switchCapturing();
}

bool TPlayer::haveVideoFilters() const {

    return mset.phase_filter
        || mset.current_deinterlacer != TMediaSettings::NoDeinterlace
        || (mset.stereo3d_in != "none" && !mset.stereo3d_out.isEmpty())
        || mset.current_denoiser != TMediaSettings::NoDenoise
        || mset.current_unsharp
        || mset.deblock_filter
        || mset.dering_filter
        || mset.gradfun_filter
        || mset.upscaling_filter
        || mset.noise_filter
        || mset.postprocessing_filter
        || mset.add_letterbox
        || pref->use_soft_video_eq
        || !mset.player_additional_video_filters.isEmpty()
        || !pref->player_additional_video_filters.isEmpty()
        || mset.rotate
        || mset.flip
        || mset.mirror;
}

#ifdef Q_OS_WIN
bool TPlayer::videoFiltersEnabled(bool) {
    return true;
}
#else
bool TPlayer::videoFiltersEnabled(bool displayMessage) {

    bool enabled = true;

    if (pref->isMPlayer()) {
        QString msg;
        if (pref->vo.startsWith("vdpau")) {
            enabled = !pref->vdpau.disable_video_filters;
            if (enabled) {
                msg = tr("The video driver settings for vdpau allow filters, this might not work...");
            } else {
                msg = tr("Using vdpau, the video filters will be ignored");
            }
        }

        if (displayMessage && !msg.isEmpty() && haveVideoFilters()) {
            logger()->debug("videoFiltersEnabled: " + msg);
            Gui::msg(msg, 0);
        }
    }

    return enabled;
}
#endif

void TPlayer::startPlayer(QString file, bool loopImage) {
    logger()->debug("startPlayer: '%1'", file);

    if (file.isEmpty()) {
        logger()->warn("startPlayer: file is empty");
        return;
    }

    if (proc->isRunning()) {
        logger()->warn("startPlayer: Player still running");
        return;
    }

    Gui::msg(tr("Starting player..."), 0);

    // Check URL playlist
    if (file.endsWith("|playlist")) {
        file = file.remove("|playlist");
    }

    // Load m4a audio file with the same name as file
    if (pref->autoload_m4a && mset.external_audio.isEmpty()) {
        QFileInfo fi(file);
        if (fi.exists() && !fi.isDir() && fi.suffix().toLower() == "mp4") {
            QString file2 = fi.path() + "/" + fi.completeBaseName() + ".m4a";
            if (!QFile::exists(file2)) {
                // Check for upper case
                file2 = fi.path() + "/" + fi.completeBaseName() + ".M4A";
            }
            if (QFile::exists(file2)) {
                logger()->debug("startPlayer: using external audio file '%1'",
                                file2);
                mset.external_audio = file2;
            }
        }
    }

    proc->clearArguments();
    proc->setExecutable(pref->player_bin);
    proc->setFixedOptions();
    proc->disableInput();

#if defined(Q_OS_OS2)
#define WINIDFROMHWND(hwnd) ((hwnd) - 0x80000000UL)
    proc->setOption("wid", QString::number(
        WINIDFROMHWND((int) playerwindow->videoWindow()->winId())));
#else
    proc->setOption("wid",
        QString::number((qint64) playerwindow->videoWindow()->winId()));
#endif

    if (pref->log_verbose) {
        proc->setOption("verbose");
    }

    // Seek to in point, mset.current_sec or restartTime
    seeking = false;
    if (mdat.selected_type == TMediaData::TYPE_FILE) {
        if (mdat.image) {
            proc->setImageDuration(pref->imageDuration);
            if (loopImage) {
                proc->setOption("loop", 0 /* loop forever */);
            }
        }

        double ss = 0;
        if (mset.in_point > 0) {
            ss = mset.in_point;
        }
        // current_sec may override in point, but cannot be beyond out point
        if (mset.current_sec > 0
            && (mset.out_point <= 0 || mset.current_sec < mset.out_point)) {
            ss = mset.current_sec;
        }
        if (restartTime != 0) {
            ss = restartTime;
        }
        if (ss != 0) {
            proc->setOption("ss", ss);
        }
    }
    restartTime = 0;

    // Setup hardware decoding for MPV.
    // First set mdat.video_hwdec, handle setting hwdec option later
    QString hwdec = pref->hwdec;
    if (pref->isMPV()) {
        // Disable hardware decoding when there are filters in use
        if (hwdec != "no" && haveVideoFilters()) {
            hwdec = "no";
            QString msg = tr("Disabled hardware decoding for video filters");
            logger()->debug("startPlayer: " + msg);
            Gui::msg(msg, 0);
        }
        mdat.video_hwdec = hwdec != "no";
    } else {
        mdat.video_hwdec = false;
    }

    // Forced demuxer
    if (!mset.forced_demuxer.isEmpty()) {
        proc->setOption("demuxer", mset.forced_demuxer);
    }
    // Forced audio codec
    if (!mset.forced_audio_codec.isEmpty()) {
        proc->setOption("ac", mset.forced_audio_codec);
    }
    // Forced video codec
    if (!mset.forced_video_codec.isEmpty()) {
        proc->setOption("vc", mset.forced_video_codec);
    } else {

#ifndef Q_OS_WIN
        // VDPAU codecs
        if (pref->vo.startsWith("vdpau")) {
            if (pref->isMPlayer()) {
                QString c;
                if (pref->vdpau.ffh264vdpau) c = "ffh264vdpau,";
                if (pref->vdpau.ffmpeg12vdpau) c += "ffmpeg12vdpau,";
                if (pref->vdpau.ffwmv3vdpau) c += "ffwmv3vdpau,";
                if (pref->vdpau.ffvc1vdpau) c += "ffvc1vdpau,";
                if (pref->vdpau.ffodivxvdpau) c += "ffodivxvdpau,";
                if (!c.isEmpty()) {
                    proc->setOption("vc", c);
                }
            } else if (mdat.video_hwdec) {
                QString c;
                if (pref->vdpau.ffh264vdpau) c = ",h264";
                if (pref->vdpau.ffmpeg12vdpau) c += ",mpeg1video,mpeg2video";
                if (pref->vdpau.ffwmv3vdpau) c += ",wmv3";
                if (pref->vdpau.ffvc1vdpau) c += ",vc1";
                if (pref->vdpau.ffodivxvdpau) c += ",mpeg4";
                if (!c.isEmpty()) {
                    proc->setOption("hwdec-codecs", c.mid(1));
                }
            }
        }
#endif

    }

    // MPV only
    if (!hwdec.isEmpty() && pref->isMPV()) {
        proc->setOption("hwdec", hwdec);
    }

    if (!pref->vo.isEmpty()) {
        proc->setOption("vo", pref->vo);
    }

    // Enable software scaling for vo x11
#if !defined(Q_OS_WIN) && !defined(Q_OS_OS2)
    if (pref->vo.startsWith("x11")) {
        proc->setOption("zoom");
    }
#endif

    if (!pref->ao.isEmpty()) {
        proc->setOption("ao", pref->ao);
    }

#if PROGRAM_SWITCH
    if (mset.current_program_id != TMediaSettings::NoneSelected) {
        proc->setOption("tsprog", QString::number(mset.current_program_id));
    }
    // Don't set video and audio track if using -tsprog
    else {
#endif

    if (mset.current_video_id >= 0) {
        proc->setOption("vid", QString::number(mset.current_video_id));
    }

    if (mset.external_audio.isEmpty() && mset.current_audio_id >= 0) {
        proc->setOption("aid", QString::number(mset.current_audio_id));
    }

#if PROGRAM_SWITCH
    }
#endif

    if (!mset.external_audio.isEmpty()) {
        proc->setOption("audiofile", mset.external_audio);
    }

    // Aspect ratio video
    QString aspect_option = mset.aspect_ratio.toOption();
    if (!aspect_option.isEmpty()) {
        // Clear original aspect ratio
        if (_state != STATE_RESTARTING) {
            mdat.video_aspect_original = -1;
        }
        proc->setOption("aspect", aspect_option);
    }
    // Aspect ratio monitor
    double aspect = pref->monitorAspectDouble();
    if (aspect > 0) {
        proc->setOption("monitorpixelaspect", aspect);
    }

    // Colorkey, only used by XP directx and OS2 kva drivers
    // to set color key for overlay
    if (pref->useColorKey()) {
        proc->setOption("colorkey", TColorUtils::colorToRGB(pref->color_key));
    }

    // Synchronization
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

    // OSD
    proc->setOption("osdlevel", pref->osd_level);
    if (pref->isMPlayer()) {
        proc->setOption("osd-scale", pref->subfont_osd_scale);
    } else {
        proc->setOption("osd-scale", pref->osd_scale);
        proc->setOption("osd-scale-by-window", "no");
    }

    // Subtitle search fuzziness
    proc->setOption("sub-fuzziness", pref->subtitle_fuzziness);

    // Subtitles fonts
    if (pref->use_ass_subtitles && pref->freetype_support) {
        // Use ASS options
        proc->setOption("ass");
        proc->setOption("embeddedfonts");
        proc->setOption("ass-font-scale", QString::number(mset.sub_scale_ass));
        proc->setOption("ass-line-spacing",
                        QString::number(pref->ass_line_spacing));

        // Custom ASS style
        if (pref->use_custom_ass_style) {
            QString ass_force_style;
            if (pref->user_forced_ass_style.isEmpty()) {
                ass_force_style = pref->ass_styles.toString();
            } else {
                ass_force_style = pref->user_forced_ass_style;
            }

            if (pref->isMPV()) {
                proc->setSubStyles(pref->ass_styles);
                if (pref->force_ass_styles) {
                    proc->setOption("ass-force-style", ass_force_style);
                }
            } else {
                if (pref->force_ass_styles) {
                    proc->setOption("ass-force-style", ass_force_style);
                } else {
                    proc->setSubStyles(pref->ass_styles,
                                       TPaths::subtitleStyleFile());
                }
            }
        }
    } else {
        // NO ASS
        if (pref->freetype_support)
            proc->setOption("noass");
        if (pref->isMPV()) {
            proc->setOption("sub-scale", QString::number(mset.sub_scale_mpv));
        } else {
            proc->setOption("subfont-text-scale",
                            QString::number(mset.sub_scale));
        }
    }

    // Subtitle encoding
    if (pref->subtitle_enca_language.isEmpty()) {
        // No encoding language, set fallback code page
        if (!pref->subtitle_encoding_fallback.isEmpty()) {
            if (pref->isMPlayer()) {
                proc->setOption("subcp", pref->subtitle_encoding_fallback);
            } else if (pref->subtitle_encoding_fallback != "UTF-8") {
                // Use pref->subtitle_encoding_fallback if encoding is not utf8
                proc->setOption("subcp",
                                "utf8:" + pref->subtitle_encoding_fallback);
            }
        }
    } else {
        // Add subtitle encoding enca language
        QString encoding = "enca:"+ pref->subtitle_enca_language;
        // Add subtitle encoding fallback
        if (!pref->subtitle_encoding_fallback.isEmpty()) {
            encoding += ":"+ pref->subtitle_encoding_fallback;
        }
        proc->setOption("subcp", encoding);
    }

    if (mset.closed_caption_channel > 0) {
        proc->setOption("subcc", QString::number(mset.closed_caption_channel));
    }

    if (pref->use_forced_subs_only) {
        proc->setOption("forcedsubsonly");
    }

    if (!initial_subtitle.isEmpty()) {
        // Setup external sub from command line or other instance
        setExternalSubs(initial_subtitle);
        initial_subtitle = "";
    } else if (mset.current_sub_set_by_user) {
        // Sub selected by user
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

    if (mset.sub_pos != 100)
        proc->setOption("subpos", QString::number(mset.sub_pos));

    if (mset.sub_delay != 0) {
        proc->setOption("subdelay",
                        QString::number((double) mset.sub_delay/1000));
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

    if (mset.current_angle > 0) {
        proc->setOption("dvdangle", QString::number(mset.current_angle));
    }

    // TODO: TMPVProcess title and track switch code does not run nicely when
    // caching is set. Seeks inside cache don't notify track or title changes.
    cache_size = -1;
    if (mdat.selected_type == TMediaData::TYPE_DVDNAV) {
        // Always set no cache for DVDNAV
        cache_size = 0;
    } else if (pref->cache_enabled) {
        // Enabled set cache
        switch (mdat.selected_type) {
            case TMediaData::TYPE_FILE  : cache_size = pref->cache_for_files; break;
            case TMediaData::TYPE_DVD   : cache_size = pref->cache_for_dvds; break;
            case TMediaData::TYPE_STREAM: cache_size = pref->cache_for_streams; break;
            case TMediaData::TYPE_VCD   : cache_size = pref->cache_for_vcds; break;
            case TMediaData::TYPE_CDDA  : cache_size = pref->cache_for_audiocds; break;
            case TMediaData::TYPE_TV    : cache_size = pref->cache_for_tv; break;
            case TMediaData::TYPE_BLURAY: cache_size = pref->cache_for_dvds; break; // FIXME: use cache for bluray?
            default: cache_size = 0;
        } // switch
    }
    if (cache_size >= 0) {
        proc->setOption("cache", QString::number(cache_size));
    }

    if (mset.speed != 1.0) {
        proc->setOption("speed", QString::number(mset.speed));
    }

    if (pref->use_idx) {
        proc->setOption("idx");
    }

    if (mdat.image || pref->use_correct_pts != TPreferences::Detect) {
        proc->setOption("correct-pts", !mdat.image
                        && pref->use_correct_pts == TPreferences::Enabled);
    }

    // Setup screenshot directory.
    // Needs to be setup before the video filters below use it.
    if (pref->screenshot_directory.isEmpty()) {
        pref->use_screenshot = false;
    } else {
        QFileInfo fi(pref->screenshot_directory);
        if (!fi.isDir() || !fi.isWritable()) {
            logger()->warn("startPlayer: disabled screenshots and capturing,"
                           " directory '" + pref->screenshot_directory
                           + "' not writable");
            pref->use_screenshot = false;
            // Need to clear to disable capture
            pref->screenshot_directory = "";
        }
    }
    if (pref->use_screenshot) {
        proc->setScreenshotDirectory(pref->screenshot_directory);
    }

    if (!videoFiltersEnabled(true)) {
        goto end_video_filters;
    }

    // Video filters:
    // Phase
    if (mset.phase_filter) {
        proc->addVF("phase", "A");
    }

    // Deinterlace
    if (mset.current_deinterlacer != TMediaSettings::NoDeinterlace) {
        switch (mset.current_deinterlacer) {
            case TMediaSettings::L5:         proc->addVF("l5"); break;
            case TMediaSettings::Yadif:     proc->addVF("yadif"); break;
            case TMediaSettings::LB:        proc->addVF("lb"); break;
            case TMediaSettings::Yadif_1:    proc->addVF("yadif", "1"); break;
            case TMediaSettings::Kerndeint:    proc->addVF("kerndeint", "5"); break;
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
        proc->setOption("autoq", QString::number(pref->postprocessing_quality));
    }

    // Letterbox (expand)
    if (mset.add_letterbox) {
        proc->addVF("expand", QString("aspect=%1").arg(TDesktop::aspectRatio(playerwindow)));
    }

    // Software equalizer
    if (pref->use_soft_video_eq) {
        proc->addVF("eq2");
        proc->addVF("hue");
        if (pref->vo == "gl" || pref->vo == "gl2" || pref->vo == "gl_tiled"

#ifdef Q_OS_WIN
            || pref->vo == "directx:noaccel"
#endif

            ) {
            proc->addVF("scale");
        }
    }

    // Additional video filters, supplied by user
    // File
    if (!mset.player_additional_video_filters.isEmpty()) {
        proc->setOption("vf-add", mset.player_additional_video_filters);
    }
    // Global
    if (!pref->player_additional_video_filters.isEmpty()) {
        proc->setOption("vf-add", pref->player_additional_video_filters);
    }

    // Filters for subtitles on screenshots
    if (pref->use_screenshot && pref->subtitles_on_screenshots) {
        if (pref->use_ass_subtitles) {
            proc->addVF("subs_on_screenshots", "ass");
        } else {
            proc->addVF("subs_on_screenshots");
        }
    }

    // Rotate
    if (mset.rotate) {
        proc->addVF("rotate", mset.rotate);
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
    if (pref->use_screenshot) {
        proc->addVF("screenshot");
    }

end_video_filters:

    // Template for screenshots (only works with mpv)
    if (pref->isMPV() && pref->use_screenshot) {
        if (!pref->screenshot_template.isEmpty()) {
            proc->setOption("screenshot_template", pref->screenshot_template);
        }
        if (!pref->screenshot_format.isEmpty()) {
            proc->setOption("screenshot_format", pref->screenshot_format);
        }
    }

    // Volume
    if (pref->player_additional_options.contains("-volume")) {
        logger()->debug("startPlayer: don't set volume since -volume is used");
    } else {
        proc->setOption("volume", QString::number(getVolume()));
    }

    if (getMute()) {
        proc->setOption("mute");
    }

    // Audio channels
    if (mset.audio_use_channels != 0) {
        proc->setOption("channels", QString::number(mset.audio_use_channels));
    }

    if (mset.audio_delay != 0) {
        proc->setOption("delay", QString::number((double) mset.audio_delay/1000));
    }

    if (pref->use_hwac3) {
        proc->setOption("afm", "hwac3");
        logger()->debug("startPlayer: audio filters are disabled when using the S/PDIF output");
    } else {

        // Audio filters
        if (mset.karaoke_filter) {
            proc->addAF("karaoke");
        }

        // Stereo mode
        if (mset.stereo_mode != 0) {
            switch (mset.stereo_mode) {
                case TMediaSettings::Left: proc->addAF("channels", "2:2:0:1:0:0"); break;
                case TMediaSettings::Right: proc->addAF("channels", "2:2:1:0:1:1"); break;
                case TMediaSettings::Mono: proc->addAF("pan", "1:0.5:0.5"); break;
                case TMediaSettings::Reverse: proc->addAF("channels", "2:2:0:1:1:0"); break;
            }
        }

        if (mset.extrastereo_filter) {
            proc->addAF("extrastereo");
        }

        if (mset.volnorm_filter) {
            proc->addAF("volnorm", pref->filters.item("volnorm").options());
        }

        if (pref->use_scaletempo == TPreferences::Enabled) {
            proc->addAF("scaletempo");
        }

        // Audio equalizer
        if (pref->use_audio_equalizer) {
            proc->addAF("equalizer", equalizerListToString(getAudioEqualizer()));
        }

        // Additional audio filters
        // Global from pref
        if (!pref->player_additional_audio_filters.isEmpty()) {
            proc->setOption("af-add", pref->player_additional_audio_filters);
        }
        // This file from mset
        if (!mset.player_additional_audio_filters.isEmpty()) {
            proc->setOption("af-add", mset.player_additional_audio_filters);
        }
    }

#ifndef Q_OS_WIN
    if (pref->isMPV() && file.startsWith("dvb:")) {
        QString channels_file = Gui::Action::TTVList::findChannelsFile();
        logger()->debug("startPlayer: channels_file: " + channels_file);
        if (!channels_file.isEmpty())
            proc->setChannelsFile(channels_file);
    }
#endif

    // Set the capture directory
    proc->setCaptureDirectory(pref->screenshot_directory);

    // Set preferred ip version
    if (pref->ipPrefer == Settings::TPreferences::IP_PREFER_4) {
        proc->setOption("prefer-ipv4");
    } else if (pref->ipPrefer == Settings::TPreferences::IP_PREFER_6) {
        proc->setOption("prefer-ipv6");
    }

    // Load edl file
    if (pref->use_edl_files) {
        QString edl_f;
        QFileInfo f(file);
        QString basename = f.path() + "/" + f.completeBaseName();

        if (QFile::exists(basename + ".edl"))
            edl_f = basename + ".edl";
        else if (QFile::exists(basename + ".EDL"))
            edl_f = basename + ".EDL";

        logger()->debug("startPlayer: edl file: '" + edl_f + "'");
        if (!edl_f.isEmpty()) {
            proc->setOption("edl", edl_f);
        }
    }

    // Per file additional options
    if (!mset.player_additional_options.isEmpty()) {
        QStringList args = Player::Process::TProcess::splitArguments(
                               mset.player_additional_options);
        for (int n = 0; n < args.count(); n++) {
            QString arg = args[n].trimmed();
            if (!arg.isEmpty()) {
                logger()->debug("startPlayer: adding file specific argument"
                                " '%1'", arg);
                proc->addUserOption(arg);
            }
        }
    }

    // Global additional options
    if (!pref->player_additional_options.isEmpty()) {
        QStringList args = Player::Process::TProcess::splitArguments(
                               pref->player_additional_options);
        for (int n = 0; n < args.count(); n++) {
            QString arg = args[n].trimmed();
            if (!arg.isEmpty()) {
                logger()->debug("startPlayer: adding argument '%1'", arg);
                proc->addUserOption(arg);
            }
        }
    }

    // Set file and playing msg
    proc->setMedia(file);

    // Setup environment
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (pref->use_proxy
        && pref->proxy_type == QNetworkProxy::HttpProxy
        && !pref->proxy_host.isEmpty()) {
        QString proxy = QString("http://%1:%2@%3:%4")
                        .arg(pref->proxy_username)
                        .arg(pref->proxy_password)
                        .arg(pref->proxy_host)
                        .arg(pref->proxy_port);
        env.insert("http_proxy", proxy);
    }

#ifdef Q_OS_WIN
    if (!pref->use_windowsfontdir) {
        env.insert("FONTCONFIG_FILE", TPaths::fontConfigFilename());
    }
#endif

    proc->setProcessEnvironment(env);

    if (!proc->startPlayer()) {
        // Error reported by onProcessError()
        logger()->error("startPlayer: player process didn't start");
    }
} //startPlayer()

void TPlayer::seekCmd(double secs, int mode) {
    logger()->debug("seekCmd: %1 secs, mode %2, at %3",
                    QString::number(secs), mode,
                    QString::number(mset.current_sec));

    // seek <value> [type]
    // Seek to some place in the movie.
    // mode 0 is a relative seek of +/- <value> seconds (default).
    // mode 1 is a seek to <value> % in the movie.
    // mode 2 is a seek to an absolute position of <value> seconds.

    if (mode != 0) {
        if (secs < 0)
            secs = 0;
        if (mode == 1) {
            if (secs > 100) {
                secs = 100;
            }
        } else if (mode == 2 && mdat.duration > 0 && secs >= mdat.duration) {
            logger()->warn("seekCmd: seek " + QString::number(secs)
                           + " beyond end of video "
                           + QString::number(mdat.duration));
            // TODO: limit only when mdat.duration is proven reliable...
            // if (mdat.video_fps > 0)
            //    secs = mdat.duration - (1.0 / mdat.video_fps);
            // else secs = mdat.duration - 0.1;
        }
    }

    if (proc->isReady()) {
        proc->seek(secs, mode, pref->precise_seeking, _state == STATE_PAUSED);
    } else {
        logger()->warn("seekCmd: ignored seek, player not ready");
    }
}

void TPlayer::seekRelative(double secs) {
    logger()->debug("seekRelative: %1 seconds", secs);
    seekCmd(secs, 0);
}

void TPlayer::seekPercentage(double perc) {
    logger()->debug("seekPercentage: %1 procent", perc);
    seekCmd(perc, 1);
}

void TPlayer::seekTime(double sec) {
    logger()->debug("seekTime: %1 seconds", sec);
    seekCmd(sec, 2);
}

void TPlayer::sforward() {
    logger()->debug("sforward");
    seekRelative(pref->seeking1); // +10s
}

void TPlayer::srewind() {
    logger()->debug("srewind");
    seekRelative(-pref->seeking1); // -10s
}


void TPlayer::forward() {
    logger()->debug("forward");
    seekRelative(pref->seeking2); // +1m
}


void TPlayer::rewind() {
    logger()->debug("rewind");
    seekRelative(-pref->seeking2); // -1m
}


void TPlayer::fastforward() {
    logger()->debug("fastforward");
    seekRelative(pref->seeking3); // +10m
}


void TPlayer::fastrewind() {
    logger()->debug("fastrewind");
    seekRelative(-pref->seeking3); // -10m
}

void TPlayer::forward(int secs) {
    logger()->debug("forward: %1", secs);
    seekRelative(secs);
}

void TPlayer::rewind(int secs) {
    logger()->debug("rewind: %1", secs);
    seekRelative(-secs);
}

void TPlayer::seekToNextSub() {
    logger()->debug("seekToNextSub");
    proc->seekSub(1);
}

void TPlayer::seekToPrevSub() {
    logger()->debug("seekToPrevSub");
    proc->seekSub(-1);
}

void TPlayer::wheelUp(TPreferences::TWheelFunction function) {
    logger()->debug("wheelUp");

    if (function == TPreferences::DoNothing) {
        function = (TPreferences::TWheelFunction) pref->wheel_function;
    }
    switch (function) {
        case TPreferences::Volume : incVolume(); break;
        case TPreferences::Zoom : incZoom(); break;
        case TPreferences::Seeking : pref->wheel_function_seeking_reverse ? rewind(pref->seeking4) : forward(pref->seeking4); break;
        case TPreferences::ChangeSpeed : incSpeed10(); break;
        default : {} // do nothing
    }
}

void TPlayer::wheelDown(TPreferences::TWheelFunction function) {
    logger()->debug("wheelDown");

    if (function == TPreferences::DoNothing) {
        function = (TPreferences::TWheelFunction) pref->wheel_function;
    }
    switch (function) {
        case TPreferences::Volume : decVolume(); break;
        case TPreferences::Zoom : decZoom(); break;
        case TPreferences::Seeking : pref->wheel_function_seeking_reverse ? forward(pref->seeking4) : rewind(pref->seeking4); break;
        case TPreferences::ChangeSpeed : decSpeed10(); break;
        default : {} // do nothing
    }
}

void TPlayer::setInPoint() {
    setInPoint(mset.current_sec);
}

void TPlayer::setInPoint(double sec) {
    logger()->debug("setInPoint: %1", sec);

    mset.in_point = sec;
    if (mset.in_point < 0) {
        mset.in_point = 0;
    }
    QString msg = tr("In point set to %1")
                  .arg(Helper::formatTime(qRound(mset.in_point)));

    if (mset.out_point >= 0 && mset.in_point >= mset.out_point) {
        mset.out_point = -1;
        mset.loop = false;
        updateLoop();
        msg += tr(", cleared out point and repeat");
    }

    emit InOutPointsChanged();
    Gui::msgOSD(msg);
}

void TPlayer::seekInPoint() {

    seekTime(mset.in_point);
    Gui::msgOSD(tr("Seeking to %1")
                .arg(Helper::formatTime(qRound(mset.in_point))));
}

void TPlayer::clearInPoint() {
    logger()->debug("clearInPoint");

    mset.in_point = 0;
    emit InOutPointsChanged();
    Gui::msgOSD(tr("Cleared in point"));
}

void TPlayer::setOutPoint() {
    setOutPoint(mset.current_sec);
}

void TPlayer::setOutPoint(double sec) {
    logger()->debug("setOutPoint: %1", sec);

    if (sec <= 0) {
        clearOutPoint();
        return;
    }

    mset.out_point = sec;
    mset.loop = true;

    QString msg;
    msg = tr("Out point set to %1, repeat set")
          .arg(Helper::formatTime(qRound(mset.out_point)));
    if (mset.in_point >= mset.out_point) {
        mset.in_point = 0;
        msg += tr(" and cleared in point");
    }

    updateLoop();

    emit InOutPointsChanged();
    Gui::msgOSD(msg);
}

void TPlayer::seekOutPoint() {

    double seek;
    if (mset.loop && _state != STATE_PAUSED) {
        seek = mset.in_point;
    } else if (mset.out_point > 0){
        seek = mset.out_point;
    } else {
        Gui::msgOSD(tr("Out point not set"));
        return;
    }
    seekTime(seek);
    Gui::msgOSD(tr("Seeking to %1").arg(Helper::formatTime(qRound(seek))));
}

void TPlayer::clearOutPoint() {
    logger()->debug("clearOutPoint");

    mset.out_point = -1;
    mset.loop = false;
    updateLoop();

    emit InOutPointsChanged();
    Gui::msgOSD(tr("Cleared out point and repeat"));
}

void TPlayer::clearInOutPoints() {
    logger()->debug("clearInOutPoints");

    mset.in_point = 0;
    mset.out_point = -1;
    mset.loop = false;
    updateLoop();

    emit InOutPointsChanged();
    Gui::msgOSD(tr("In-out points and repeat cleared"));
}

void TPlayer::updateLoop() {

    if (mset.loop && mset.out_point <= 0) {
        proc->setLoop(true);
    } else {
        proc->setLoop(false);
    }
}

void TPlayer::toggleRepeat(bool b) {
    logger()->debug("toggleRepeat: %1", b);

    mset.loop = b;
    updateLoop();

    emit InOutPointsChanged();
    if (mset.loop) {
        Gui::msgOSD(tr("Repeat in-out set"));
    } else {
        Gui::msgOSD(tr("Repeat in-out cleared"));
    }
}

// Audio filters
void TPlayer::toggleKaraoke() {
    toggleKaraoke(!mset.karaoke_filter);
}

void TPlayer::toggleKaraoke(bool b) {
    logger()->debug("toggleKaraoke: %1", b);
    if (b != mset.karaoke_filter) {
        mset.karaoke_filter = b;
        proc->enableKaraoke(b);
    }
}

void TPlayer::toggleExtrastereo() {
    toggleExtrastereo(!mset.extrastereo_filter);
}

void TPlayer::toggleExtrastereo(bool b) {
    logger()->debug("toggleExtrastereo: %1", b);
    if (b != mset.extrastereo_filter) {
        mset.extrastereo_filter = b;
        proc->enableExtrastereo(b);
    }
}

void TPlayer::toggleVolnorm() {
    toggleVolnorm(!mset.volnorm_filter);
}

void TPlayer::toggleVolnorm(bool b) {
    logger()->debug("toggleVolnorm: %1", b);

    if (b != mset.volnorm_filter) {
        mset.volnorm_filter = b;
        QString f = pref->filters.item("volnorm").filter();
        proc->enableVolnorm(b, pref->filters.item("volnorm").options());
    }
}

void TPlayer::setAudioChannels(int channels) {
    logger()->debug("setAudioChannels:%1", channels);

    if (channels != mset.audio_use_channels) {
        mset.audio_use_channels = channels;
        if (proc->isRunning())
            restartPlay();
    }
}

void TPlayer::setStereoMode(int mode) {
    logger()->debug("setStereoMode:%1", mode);
    if (mode != mset.stereo_mode) {
        mset.stereo_mode = mode;
        if (proc->isRunning())
            restartPlay();
    }
}


// Video filters

void TPlayer::changeVF(const QString& filter, bool enable,
                       const QVariant& option) {

    if (pref->isMPV() && !mdat.video_hwdec) { \
        proc->changeVF(filter, enable, option); \
    } else { \
        restartPlay(); \
    }
}

void TPlayer::toggleFlip() {
    logger()->debug("toggleFlip");
    toggleFlip(!mset.flip);
}

void TPlayer::toggleFlip(bool b) {
    logger()->debug("toggleFlip: %1", b);

    if (mset.flip != b) {
        mset.flip = b;
        changeVF("flip", b, QVariant());
    }
}

void TPlayer::toggleMirror() {
    logger()->debug("toggleMirror");
    toggleMirror(!mset.mirror);
}

void TPlayer::toggleMirror(bool b) {
    logger()->debug("toggleMirror: %1", b);

    if (mset.mirror != b) {
        mset.mirror = b;
        changeVF("mirror", b, QVariant());
    }
}

void TPlayer::toggleAutophase() {
    toggleAutophase(!mset.phase_filter);
}

void TPlayer::toggleAutophase(bool b) {
    logger()->debug("toggleAutophase: %1", b);

    if (b != mset.phase_filter) {
        mset.phase_filter = b;
        changeVF("phase", b, "A");
    }
}

void TPlayer::toggleDeblock() {
    toggleDeblock(!mset.deblock_filter);
}

void TPlayer::toggleDeblock(bool b) {
    logger()->debug("toggleDeblock: %1", b);

    if (b != mset.deblock_filter) {
        mset.deblock_filter = b;
        changeVF("deblock", b, pref->filters.item("deblock").options());
    }
}

void TPlayer::toggleDering() {
    toggleDering(!mset.dering_filter);
}

void TPlayer::toggleDering(bool b) {
    logger()->debug("toggleDering: %1", b);

    if (b != mset.dering_filter) {
        mset.dering_filter = b;
        changeVF("dering", b, QVariant());
    }
}

void TPlayer::toggleGradfun() {
    toggleGradfun(!mset.gradfun_filter);
}

void TPlayer::toggleGradfun(bool b) {
    logger()->debug("toggleGradfun: %1", b);

    if (b != mset.gradfun_filter) {
        mset.gradfun_filter = b;
        changeVF("gradfun", b, pref->filters.item("gradfun").options());
    }
}

void TPlayer::toggleNoise() {
    toggleNoise(!mset.noise_filter);
}

void TPlayer::toggleNoise(bool b) {
    logger()->debug("toggleNoise: %1", b);

    if (b != mset.noise_filter) {
        mset.noise_filter = b;
        changeVF("noise", b, QVariant());
    }
}

void TPlayer::togglePostprocessing() {
    togglePostprocessing(!mset.postprocessing_filter);
}

void TPlayer::togglePostprocessing(bool b) {
    logger()->debug("togglePostprocessing: %1", b);

    if (b != mset.postprocessing_filter) {
        mset.postprocessing_filter = b;
        changeVF("postprocessing", b, QVariant());
    }
}

void TPlayer::changeDenoise(int id) {
    logger()->debug("changeDenoise: %1", id);

    if (id != mset.current_denoiser) {
        if (pref->isMPlayer() || mdat.video_hwdec) {
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

void TPlayer::changeUnsharp(int id) {
    logger()->debug("changeUnsharp: %1", id);

    if (id != mset.current_unsharp) {
        if (pref->isMPlayer() || mdat.video_hwdec) {
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

void TPlayer::changeUpscale(bool b) {
    logger()->debug("changeUpscale: %1", b);

    if (mset.upscaling_filter != b) {
        mset.upscaling_filter = b;
        int width = TDesktop::size(playerwindow).width();
        changeVF("scale", b, QString::number(width) + ":-2");
    }
}

void TPlayer::changeStereo3d(const QString & in, const QString & out) {
    logger()->debug("changeStereo3d: in: " + in + " out: " + out);

    if ((mset.stereo3d_in != in) || (mset.stereo3d_out != out)) {
        if (pref->isMPlayer() || mdat.video_hwdec) {
            mset.stereo3d_in = in;
            mset.stereo3d_out = out;
            restartPlay();
        } else {
            // Remove previous filter
            if (mset.stereo3d_in != "none" && !mset.stereo3d_out.isEmpty()) {
                proc->changeStereo3DFilter(false, mset.stereo3d_in,
                                           mset.stereo3d_out);
            }

            // New filter
            mset.stereo3d_in = in;
            mset.stereo3d_out = out;
            if (mset.stereo3d_in != "none" && !mset.stereo3d_out.isEmpty()) {
                proc->changeStereo3DFilter(true, mset.stereo3d_in,
                                           mset.stereo3d_out);
            }
        }
    }
}

void TPlayer::setBrightness(int value) {
    logger()->debug("setBrightness: %1", value);

    if (value > 100) value = 100;
    if (value < -100) value = -100;

    if (value != mset.brightness) {
        proc->setBrightness(value);
        mset.brightness = value;
        Gui::msgOSD(tr("Brightness: %1").arg(value));
        emit videoEqualizerNeedsUpdate();
    }
}

void TPlayer::setContrast(int value) {
    logger()->debug("setContrast: %1", value);

    if (value > 100) value = 100;
    if (value < -100) value = -100;

    if (value != mset.contrast) {
        proc->setContrast(value);
        mset.contrast = value;
        Gui::msgOSD(tr("Contrast: %1").arg(value));
        emit videoEqualizerNeedsUpdate();
    }
}

void TPlayer::setGamma(int value) {
    logger()->debug("setGamma: %1", value);

    if (value > 100) value = 100;
    if (value < -100) value = -100;

    if (value != mset.gamma) {
        proc->setGamma(value);
        mset.gamma= value;
        Gui::msgOSD(tr("Gamma: %1").arg(value));
        emit videoEqualizerNeedsUpdate();
    }
}

void TPlayer::setHue(int value) {
    logger()->debug("setHue: %1", value);

    if (value > 100) value = 100;
    if (value < -100) value = -100;

    if (value != mset.hue) {
        proc->setHue(value);
        mset.hue = value;
        Gui::msgOSD(tr("Hue: %1").arg(value));
        emit videoEqualizerNeedsUpdate();
    }
}

void TPlayer::setSaturation(int value) {
    logger()->debug("setSaturation: %1", value);

    if (value > 100) value = 100;
    if (value < -100) value = -100;

    if (value != mset.saturation) {
        proc->setSaturation(value);
        mset.saturation = value;
        Gui::msgOSD(tr("Saturation: %1").arg(value));
        emit videoEqualizerNeedsUpdate();
    }
}

void TPlayer::incBrightness() {
    setBrightness(mset.brightness + pref->min_step);
}

void TPlayer::decBrightness() {
    setBrightness(mset.brightness - pref->min_step);
}

void TPlayer::incContrast() {
    setContrast(mset.contrast + pref->min_step);
}

void TPlayer::decContrast() {
    setContrast(mset.contrast - pref->min_step);
}

void TPlayer::incGamma() {
    setGamma(mset.gamma + pref->min_step);
}

void TPlayer::decGamma() {
    setGamma(mset.gamma - pref->min_step);
}

void TPlayer::incHue() {
    setHue(mset.hue + pref->min_step);
}

void TPlayer::decHue() {
    setHue(mset.hue - pref->min_step);
}

void TPlayer::incSaturation() {
    setSaturation(mset.saturation + pref->min_step);
}

void TPlayer::decSaturation() {
    setSaturation(mset.saturation - pref->min_step);
}

void TPlayer::setSpeed(double value) {
    logger()->debug("setSpeed: %1", value);

    // Min and max used by players
    if (value < 0.10) value = 0.10;
    if (value > 100) value = 100;

    mset.speed = value;
    proc->setSpeed(value);

    Gui::msgOSD(tr("Speed: %1").arg(value));
}

void TPlayer::incSpeed10() {
    logger()->debug("incSpeed10");
    setSpeed((double) mset.speed + 0.1);
}

void TPlayer::decSpeed10() {
    logger()->debug("decSpeed10");
    setSpeed((double) mset.speed - 0.1);
}

void TPlayer::incSpeed4() {
    logger()->debug("incSpeed4");
    setSpeed((double) mset.speed + 0.04);
}

void TPlayer::decSpeed4() {
    logger()->debug("decSpeed4");
    setSpeed((double) mset.speed - 0.04);
}

void TPlayer::incSpeed1() {
    logger()->debug("incSpeed1");
    setSpeed((double) mset.speed + 0.01);
}

void TPlayer::decSpeed1() {
    logger()->debug("decSpeed1");
    setSpeed((double) mset.speed - 0.01);
}

void TPlayer::doubleSpeed() {
    logger()->debug("doubleSpeed");
    setSpeed((double) mset.speed * 2);
}

void TPlayer::halveSpeed() {
    logger()->debug("halveSpeed");
    setSpeed((double) mset.speed / 2);
}

void TPlayer::normalSpeed() {
    setSpeed(1);
}

int TPlayer::getVolume() const {
    return pref->global_volume ? pref->volume : mset.volume;
}

/*
 Notes on volume:

 Mplayer uses 0 - 100 for volume, where 100 is the maximum volume.
 If soft volume is enabled MPlayer will amplify this value by softvol-max.
 A value of 200 for softvol-max will double the volume.

 MPV uses 0 - 100, where 100 is no amplification.
 MPV softvol-max serves as max and scale for amplification
 and according to the docs doubles volume with softvol-max 130.
 Since MPV version 0.18.1 softvol is deprecated,
*/

void TPlayer::setVolume(int volume, bool unmute) {

    if (volume < 0) {
        volume = 0;
    } else if (volume > 100) {
        volume = 100;
    }

    bool muted;
    if (pref->global_volume) {
        pref->volume = volume;
        muted = pref->mute;
    } else {
        mset.volume = volume;
        muted = mset.mute;
    }

    if (proc->isRunning()) {
        proc->setVolume(getVolume());
    }

    // Unmute audio if it was muted
    if (muted && unmute) {
        mute(false);
    }

    Gui::msgOSD(tr("Volume: %1").arg(volume));
    emit volumeChanged(volume);
}

bool TPlayer::getMute() const {
    return pref->global_volume ? pref->mute : mset.mute;
}

void TPlayer::mute(bool b) {
    logger()->debug("mute: %1", b);

    if (pref->global_volume) {
        pref->mute = b;
    } else {
        mset.mute = b;
    }
    if (proc->isRunning()) {
        proc->mute(b);
    }

    Gui::msgOSD(tr("Mute: %1").arg(b ? tr("yes") : tr("no")));
    emit muteChanged(b);
}

void TPlayer::incVolume() {
    logger()->debug("incVolume");
    setVolume(getVolume() + pref->min_step);
}

void TPlayer::decVolume() {
    logger()->debug("incVolume");
    setVolume(getVolume() - pref->min_step);
}

TAudioEqualizerList TPlayer::getAudioEqualizer() const {
    return pref->global_audio_equalizer ? pref->audio_equalizer
                                        : mset.audio_equalizer;
}

void TPlayer::setSubDelay(int delay) {
    logger()->debug("setSubDelay: %1", delay);

    mset.sub_delay = delay;
    proc->setSubDelay((double) mset.sub_delay/1000);
    Gui::msgOSD(tr("Subtitle delay: %1 ms").arg(delay));
}

void TPlayer::incSubDelay() {
    logger()->debug("incSubDelay");
    setSubDelay(mset.sub_delay + 100);
}

void TPlayer::decSubDelay() {
    logger()->debug("decSubDelay");
    setSubDelay(mset.sub_delay - 100);
}

void TPlayer::setAudioDelay(int delay) {
    logger()->debug("setAudioDelay: %1", delay);

    mset.audio_delay = delay;
    proc->setAudioDelay((double) mset.audio_delay/1000);
    Gui::msgOSD(tr("Audio delay: %1 ms").arg(delay));
}

void TPlayer::incAudioDelay() {
    logger()->debug("incAudioDelay");
    setAudioDelay(mset.audio_delay + 100);
}

void TPlayer::decAudioDelay() {
    logger()->debug("decAudioDelay");
    setAudioDelay(mset.audio_delay - 100);
}

void TPlayer::incSubPos() {
    logger()->debug("incSubPos");

    mset.sub_pos++;
    if (mset.sub_pos > 100) mset.sub_pos = 100;
    proc->setSubPos(mset.sub_pos);
}

void TPlayer::decSubPos() {
    logger()->debug("decSubPos");

    mset.sub_pos--;
    if (mset.sub_pos < 0) mset.sub_pos = 0;
    proc->setSubPos(mset.sub_pos);
}

void TPlayer::changeSubScale(double value) {
    logger()->debug("changeSubScale: %1", value);

    if (value < 0) value = 0;

    if (pref->use_ass_subtitles) {
        if (value != mset.sub_scale_ass) {
            mset.sub_scale_ass = value;
            proc->setSubScale(mset.sub_scale_ass);
        }
    } else if (pref->isMPV()) {
        if (value != mset.sub_scale_mpv) {
            mset.sub_scale_mpv = value;
            proc->setSubScale(value);
        }
    } else if (value != mset.sub_scale) {
        mset.sub_scale = value;
        proc->setSubScale(value);
    }

    Gui::msgOSD(tr("Font scale: %1").arg(value));
}

void TPlayer::incSubScale() {

    double step = 0.20;

    if (pref->use_ass_subtitles) {
        changeSubScale(mset.sub_scale_ass + step);
    } else if (pref->isMPV()) {
        changeSubScale(mset.sub_scale_mpv + step);
    } else {
        changeSubScale(mset.sub_scale + step);
    }
}

void TPlayer::decSubScale() {

    double step = 0.20;

    if (pref->use_ass_subtitles) {
        changeSubScale(mset.sub_scale_ass - step);
    } else if (pref->isMPV()) {
        changeSubScale(mset.sub_scale_mpv - step);
    } else {
        changeSubScale(mset.sub_scale - step);
    }
}

void TPlayer::changeOSDScale(double value) {
    logger()->debug("changeOSDScale: %1", value);

    if (value < 0) value = 0;

    if (pref->isMPlayer()) {
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

void TPlayer::incOSDScale() {

    if (pref->isMPlayer()) {
        changeOSDScale(pref->subfont_osd_scale + 1);
    } else {
        changeOSDScale(pref->osd_scale + 0.10);
    }
}

void TPlayer::decOSDScale() {

    if (pref->isMPlayer()) {
        changeOSDScale(pref->subfont_osd_scale - 1);
    } else {
        changeOSDScale(pref->osd_scale - 0.10);
    }
}

void TPlayer::incSubStep() {
    logger()->debug("incSubStep");
    proc->setSubStep(+1);
}

void TPlayer::decSubStep() {
    logger()->debug("decSubStep");
    proc->setSubStep(-1);
}

void TPlayer::changeExternalSubFPS(int fps_id) {
    logger()->debug("setExternalSubFPS: %1", fps_id);

    mset.external_subtitles_fps = fps_id;
    if (hasExternalSubs()) {
        restartPlay();
    }
}

// Audio equalizer functions
QString TPlayer::equalizerListToString(const Settings::TAudioEqualizerList& values) {

    double v0 = (double) values[0].toInt() / 10;
    double v1 = (double) values[1].toInt() / 10;
    double v2 = (double) values[2].toInt() / 10;
    double v3 = (double) values[3].toInt() / 10;
    double v4 = (double) values[4].toInt() / 10;
    double v5 = (double) values[5].toInt() / 10;
    double v6 = (double) values[6].toInt() / 10;
    double v7 = (double) values[7].toInt() / 10;
    double v8 = (double) values[8].toInt() / 10;
    double v9 = (double) values[9].toInt() / 10;
    QString s = QString::number(v0) + ":" + QString::number(v1) + ":" +
                QString::number(v2) + ":" + QString::number(v3) + ":" +
                QString::number(v4) + ":" + QString::number(v5) + ":" +
                QString::number(v6) + ":" + QString::number(v7) + ":" +
                QString::number(v8) + ":" + QString::number(v9);

    return s;
}

void TPlayer::setAudioEqualizer(const TAudioEqualizerList& values,
                                bool restart) {

    if (pref->global_audio_equalizer) {
        pref->audio_equalizer = values;
    } else {
        mset.audio_equalizer = values;
    }

    if (restart) {
        restartPlay();
    } else {
        proc->setAudioEqualizer(equalizerListToString(values));
    }
}

void TPlayer::setAudioEq(int eq, int value) {
    logger()->debug("setAudioEq: eq %1 value %1", eq, value);

    if (pref->global_audio_equalizer) {
        pref->audio_equalizer[eq] = value;
        setAudioEqualizer(pref->audio_equalizer);
    } else {
        mset.audio_equalizer[eq] = value;
        setAudioEqualizer(mset.audio_equalizer);
    }
}

void TPlayer::setAudioEq0(int value) {
    setAudioEq(0, value);
}

void TPlayer::setAudioEq1(int value) {
    setAudioEq(1, value);
}

void TPlayer::setAudioEq2(int value) {
    setAudioEq(2, value);
}

void TPlayer::setAudioEq3(int value) {
    setAudioEq(3, value);
}

void TPlayer::setAudioEq4(int value) {
    setAudioEq(4, value);
}

void TPlayer::setAudioEq5(int value) {
    setAudioEq(5, value);
}

void TPlayer::setAudioEq6(int value) {
    setAudioEq(6, value);
}

void TPlayer::setAudioEq7(int value) {
    setAudioEq(7, value);
}

void TPlayer::setAudioEq8(int value) {
    setAudioEq(8, value);
}

void TPlayer::setAudioEq9(int value) {
    setAudioEq(9, value);
}

void TPlayer::handleOutPoint() {

    if (_state != STATE_PLAYING) {
        seeking = false;
        return;
    }

    // Handle out point
    if (mset.out_point > 0 && mset.current_sec > mset.out_point) {
        if (mset.loop) {
            if (!seeking && mset.in_point < mset.out_point) {
                debug << "handleOutPoint: position" << mset.current_sec
                      << "reached out point" << mset.out_point
                      << ", start seeking in point " << mset.in_point
                      << debug;
                seeking = true;
                seekTime(mset.in_point);
            }
        } else {
            debug << "handleOutPoint: position" << mset.current_sec
                  << "reached out point" << mset.out_point
                  << ", sending quit" << debug;

            proc->quit(Player::Process::TExitMsg::EXIT_OUT_POINT_REACHED);
        }
    } else if (seeking) {
        debug << "handleOutPoint: done handling out point, position"
              << mset.current_sec << "no longer after out point"
              << mset.out_point << debug;
        seeking = false;
    }
}

void TPlayer::onReceivedPosition(double sec) {

    mset.current_sec = sec;

    handleOutPoint();

    emit positionChanged(sec);

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
        logger()->debug("onReceivedPosition: emit chapterChanged(%1)",
                        new_chapter_id);
        emit chapterChanged(new_chapter_id);
    }
}

// MPlayer only
void TPlayer::onReceivedPause() {
    logger()->debug("onReceivedPause: at %1",
                    QString::number(mset.current_sec));

    setState(STATE_PAUSED);
}

void TPlayer::changeDeinterlace(int ID) {
    logger()->debug("changeDeinterlace: %1", ID);

    if (ID != mset.current_deinterlacer) {
        if (pref->isMPlayer()) {
            mset.current_deinterlacer = ID;
            restartPlay();
        } else {
            // MPV: remove previous filter
            switch (mset.current_deinterlacer) {
                case TMediaSettings::L5: proc->changeVF("l5", false); break;
                case TMediaSettings::Yadif: proc->changeVF("yadif", false); break;
                case TMediaSettings::LB: proc->changeVF("lb", false); break;
                case TMediaSettings::Yadif_1: proc->changeVF("yadif", false, "1"); break;
                case TMediaSettings::Kerndeint: proc->changeVF("kerndeint", false, "5"); break;
            }
            mset.current_deinterlacer = ID;
            // Add new filter
            switch (mset.current_deinterlacer) {
                case TMediaSettings::L5: proc->changeVF("l5", true); break;
                case TMediaSettings::Yadif: proc->changeVF("yadif", true); break;
                case TMediaSettings::LB: proc->changeVF("lb", true); break;
                case TMediaSettings::Yadif_1: proc->changeVF("yadif", true, "1"); break;
                case TMediaSettings::Kerndeint: proc->changeVF("kerndeint", true, "5"); break;
            }
        }
    }
}

void TPlayer::changeVideoTrack(int id) {
    logger()->debug("changeVideoTrack: id: %1", id);

    // TODO: fix video tracks having different dimensions. The video out
    // dimension will be the dimension of the last selected video track.
    // When fixed use proc->setVideo() instead of restartPlay().
    if (id != mdat.videos.getSelectedID()) {
        mset.current_video_id = id;
        restartPlay();
    }
}

void TPlayer::nextVideoTrack() {
    logger()->debug("nextVideoTrack");

    changeVideoTrack(mdat.videos.nextID(mdat.videos.getSelectedID()));
}

void TPlayer::changeAudioTrack(int id) {
    logger()->debug("changeAudio: id: %1", id);

    mset.current_audio_id = id;
    if (id >= 0) {
        proc->setAudio(id);
        mdat.audios.setSelectedID(id);
        emit audioTrackChanged(id);

        // Workaround for a mplayer problem in windows,
        // volume is too loud after changing audio.
        // Workaround too for a mplayer problem in linux,
        // the volume is reduced if using -softvol-max.
        if (pref->isMPlayer()
            && !pref->player_additional_options.contains("-volume")) {
            setVolume(getVolume(), false);
        }
    }
}

void TPlayer::nextAudioTrack() {
    logger()->debug("nextAudioTrack");

    changeAudioTrack(mdat.audios.nextID(mdat.audios.getSelectedID()));
}

// Note: changeSubtitle is by index, not ID
void TPlayer::changeSubtitle(int idx, bool selected_by_user) {
    logger()->debug("changeSubtitle: idx %1", idx);

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

void TPlayer::nextSubtitle() {
    logger()->debug("nextSubtitle");

    changeSubtitle(mdat.subs.nextID());
}

void TPlayer::changeSecondarySubtitle(int idx) {
    logger()->debug("changeSecondarySubtitle: idx %1", idx);

    bool clr = true;

    if (idx >= 0 && idx < mdat.subs.count()) {
        SubData sub = mdat.subs.itemAt(idx);
        // Secondary may not be the same as the first
        if (sub.type() != mdat.subs.selectedType()
            || sub.ID() != mdat.subs.selectedID()) {
            mset.current_secondary_sub_idx = idx;
            clr = false;
            if (sub.type() != mdat.subs.selectedSecondaryType()
                || sub.ID() != mdat.subs.selectedSecondaryID()) {
                proc->setSecondarySubtitle(sub.type(), sub.ID());
            }
        }
    }

    if (clr) {
        mset.current_secondary_sub_idx = TMediaSettings::SubNone;
        if (mdat.subs.selectedSecondaryID() >= 0) {
            proc->disableSecondarySubtitles();
        }
    }

    emit secondarySubtitleTrackChanged(mset.current_secondary_sub_idx);
}

void TPlayer::changeTitle(int title) {
    logger()->debug("changeTitle: %1", title);

    if (proc->isRunning()) {
        // Handle CDs with the chapter commands
        if (TMediaData::isCD(mdat.detected_type)) {
            changeChapter(title - mdat.titles.firstID()
                          + mdat.chapters.firstID());
            return;
        }
        // Handle DVDNAV with title command
        if (mdat.detected_type == TMediaData::TYPE_DVDNAV) {
            proc->setTitle(title);
            return;
        }
    }

    // Start/restart
    mdat.disc.title = title;
    openDisc(mdat.disc, false);
}

void TPlayer::changeChapter(int id) {
    logger()->debug("changeChapter: ID: %1", id);

    if (id >= mdat.chapters.firstID()) {
        proc->setChapter(id);
    }
}

void TPlayer::prevChapter() {
    logger()->debug("prevChapter");
    proc->nextChapter(-1);
}

void TPlayer::nextChapter() {
    logger()->debug("nextChapter");
    proc->nextChapter(1);
}

void TPlayer::setAngle(int angle) {
    logger()->debug("setAngle: angle: %1", angle);

    mset.current_angle = angle;
    proc->setAngle(angle);
}

void TPlayer::nextAngle() {
    logger()->debug("nextAngle");

    /*
    if (mdat.angle == mdat.angles) {
        setAngle(1);
    } else {
        setAngle(mdat.angle + 1);
    }
    */

    // For now use proc->nextAngle as alternative to setAngle()
    if (mdat.angle == mdat.angles) {
        // Clear angle. 0 instead of 1 to not save the angle
        mset.current_angle = 0;
    } else {
        mset.current_angle = mdat.angle + 1;
    }
    proc->nextAngle();
}

#if PROGRAM_SWITCH
void TPlayer::changeProgram(int ID) {
    logger()->debug("changeProgram: %1", ID);

    if (ID != mset.current_program_id) {
        mset.current_program_id = ID;
        proc->setTSProgram(ID);

        /*
        mset.current_video_id = TMediaSettings::NoneSelected;
        mset.current_audio_id = TMediaSettings::NoneSelected;
        */
    }
}

void TPlayer::nextProgram() {
    logger()->debug("nextProgram");
    // Not implemented yet
}
#endif

void TPlayer::clearKeepSize() {
    logger()->debug("clearKeepSize: clearing keepSize");

    keepSizeTimer->stop();
    keepSize = false;
}

void TPlayer::setAspectRatio(int id) {
    logger()->debug("setAspectRatio: %1", id);

    // Keep id in range
    TAspectRatio::TMenuID aspect_id;
    if (id < 0 || id > TAspectRatio::MAX_MENU_ID)
        aspect_id = TAspectRatio::AspectAuto;
    else
        aspect_id = (TAspectRatio::TMenuID) id;

    if (proc->isReady() && mdat.hasVideo()) {
        mset.aspect_ratio.setID(aspect_id);

        // proc->setAspect can generate a change of VO size.
        // Inform TMainWindow::getNewSizeFactor() to keep the current size
        keepSize = true;
        proc->setAspect(mset.aspect_ratio.toDouble());

        emit aspectRatioChanged(aspect_id);
        Gui::msg(tr("Aspect ratio: %1").arg(mset.aspect_ratio.toString()));

        // Clear keepSize in case main window resize not arriving
        if (keepSize) {
            keepSizeTimer->start();
        }
    } else {
        logger()->error("setAspectRatio: " + mset.aspect_ratio.toString()
                        + ". No video player.");
    }
}

void TPlayer::nextAspectRatio() {
    setAspectRatio(mset.aspect_ratio.nextMenuID());
}

void TPlayer::nextWheelFunction() {

    int a = pref->wheel_function;

    bool done = false;
    if(((int) pref->wheel_function_cycle) == 0)
        return;
    while(!done){
        // get next a
        a = a * 2;
        if(a == 32)
            a = 2;
        // See if we are done
        if (pref->wheel_function_cycle.testFlag((TPreferences::TWheelFunction)a))
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
    Gui::msgOSD(m);
}

void TPlayer::changeLetterbox(bool b) {
    logger()->debug("changeLetterbox: %1", b);

    if (mset.add_letterbox != b) {
        mset.add_letterbox = b;
        changeVF("letterbox", b, TDesktop::aspectRatio(playerwindow));
    }
}

void TPlayer::changeLetterboxOnFullscreen(bool b) {
    logger()->debug("changeLetterboxOnFullscreen: %1", b);
    changeVF("letterbox", b, TDesktop::aspectRatio(playerwindow));
}

void TPlayer::changeOSDLevel(int level) {
    logger()->debug("changeOSDLevel: %1", level);

    pref->osd_level = (TPreferences::TOSDLevel) level;
    if (proc->isRunning())
        proc->setOSDLevel(level);
    emit osdLevelChanged(level);
}

void TPlayer::nextOSDLevel() {

    int level;
    if (pref->osd_level >= TPreferences::SeekTimerTotal) {
        level = TPreferences::None;
    } else {
        level = pref->osd_level + 1;
    }
    changeOSDLevel(level);
}

void TPlayer::changeRotate(int r) {
    logger()->debug("changeRotate: %1", r);

    if (mset.rotate != r) {
        if (pref->isMPlayer()) {
            mset.rotate = r;
            restartPlay();
        } else {
            // MPV
            // Remove previous filter
            if (mset.rotate)
                proc->changeVF("rotate", false, mset.rotate);
            mset.rotate = r;
            // Set new filter
            if (mset.rotate)
                proc->changeVF("rotate", true, mset.rotate);
        }
    }
}

void TPlayer::getZoomFromPlayerWindow() {
    mset.zoom_factor = playerwindow->zoomNormalScreen();
    mset.zoom_factor_fullscreen = playerwindow->zoomFullScreen();
}

void TPlayer::getPanFromPlayerWindow() {
    mset.pan_offset = playerwindow->panNormalScreen();
    mset.pan_offset_fullscreen = playerwindow->panFullScreen();
}

void TPlayer::setZoom(double zoom) {
    logger()->debug("setZoom: %1", zoom);

    if (mdat.hasVideo()) {
        if (zoom < TConfig::ZOOM_MIN)
            zoom = TConfig::ZOOM_MIN;
        else if (zoom > TConfig::ZOOM_MAX)
            zoom = TConfig::ZOOM_MAX;
        playerwindow->setZoom(zoom);
        getZoomFromPlayerWindow();
        Gui::msg2(tr("Zoom: %1").arg(playerwindow->zoom()));
    }
}

void TPlayer::resetZoomAndPan() {
    logger()->debug("resetZoomAndPan");

    // Reset zoom and pan of video window
    playerwindow->resetZoomAndPan();
    // Reread modified settings
    getZoomFromPlayerWindow();
    getPanFromPlayerWindow();
    Gui::msgOSD(tr("Zoom and pan reset"));
}

void TPlayer::pan(int dx, int dy) {
    logger()->debug("pan: dx %1, dy %2", dx, dy);

    if (mdat.hasVideo()) {
        playerwindow->moveVideo(dx, dy);
        getPanFromPlayerWindow();
    }
}

void TPlayer::panLeft() {
    pan(TConfig::PAN_STEP, 0);
}

void TPlayer::panRight() {
    pan(-TConfig::PAN_STEP, 0);
}

void TPlayer::panUp() {
    pan(0, TConfig::PAN_STEP);
}

void TPlayer::panDown() {
    pan(0, -TConfig::PAN_STEP);
}

void TPlayer::incZoom() {
    setZoom(playerwindow->zoom() + TConfig::ZOOM_STEP);
}

void TPlayer::decZoom() {
    setZoom(playerwindow->zoom() - TConfig::ZOOM_STEP);
}

void TPlayer::showFilenameOnOSD() {
    proc->showFilenameOnOSD();
}

void TPlayer::showTimeOnOSD() {
    proc->showTimeOnOSD();
}

void TPlayer::toggleDeinterlace() {
    proc->toggleDeinterlace();
}

void TPlayer::changeUseCustomSubStyle(bool b) {
    logger()->debug("changeUseCustomSubStyle: %1", b);

    if (pref->use_custom_ass_style != b) {
        pref->use_custom_ass_style = b;
        if (proc->isRunning())
            restartPlay();
    }
}

void TPlayer::toggleForcedSubsOnly(bool b) {
    logger()->debug("toggleForcedSubsOnly: %1", b);

    pref->use_forced_subs_only = b;
    if (proc->isRunning())
        proc->setSubForcedOnly(b);
}

void TPlayer::changeClosedCaptionChannel(int c) {
    logger()->debug("changeClosedCaptionChannel: %1", c);

    if (c != mset.closed_caption_channel) {
        mset.closed_caption_channel = c;
        if (proc->isRunning())
            restartPlay();
    }
}

// dvdnav buttons
void TPlayer::dvdnavUp() {
    logger()->debug("dvdnavUp");
    if (proc->isReady())
        proc->discButtonPressed("up");
}

void TPlayer::dvdnavDown() {
    logger()->debug("dvdnavDown");
    if (proc->isReady())
        proc->discButtonPressed("down");
}

void TPlayer::dvdnavLeft() {
    logger()->debug("dvdnavLeft");
    if (proc->isReady())
        proc->discButtonPressed("left");
}

void TPlayer::dvdnavRight() {
    logger()->debug("dvdnavRight");
    if (proc->isReady())
        proc->discButtonPressed("right");
}

void TPlayer::dvdnavMenu() {
    logger()->debug("dvdnavMenu");
    if (proc->isReady())
        proc->discButtonPressed("menu");
}

void TPlayer::dvdnavPrev() {
    logger()->debug("dvdnavPrev");
    if (proc->isReady())
        proc->discButtonPressed("prev");
}

// Slot called by action dvdnav_select
void TPlayer::dvdnavSelect() {
    logger()->debug("dvdnavSelect");

    if (proc->isReady() && mdat.detected_type == TMediaData::TYPE_DVDNAV) {
        if (_state == STATE_PAUSED) {
            play();
        }
        if (_state == STATE_PLAYING) {
            proc->discButtonPressed("select");
        }
    }
}

// Slot called by action dvdnav_mouse and Gui::TPlayerWindow when left mouse
// clicked
void TPlayer::dvdnavMouse() {
    logger()->debug("dvdnavMouse");

    if (proc->isReady()
        && mdat.detected_type == TMediaData::TYPE_DVDNAV) {
        if (_state == STATE_PAUSED) {
            play();
        }
        if (_state == STATE_PLAYING) {
            // Give a last update on the mouse position
            QPoint pos = playerwindow->videoWindow()->mapFromGlobal(
                             QCursor::pos());
            dvdnavUpdateMousePos(pos);
            // Click
            proc->discButtonPressed("mouse");
        }
    }
}

// Slot called by playerwindow to pass mouse move local to video
void TPlayer::dvdnavUpdateMousePos(const QPoint& pos) {

    if (proc->isReady()
        && mdat.detected_type == TMediaData::TYPE_DVDNAV) {
        // MPlayer won't act if paused. Play if menu not animated.
        if (_state == STATE_PAUSED && mdat.duration == 0) {
            play();
        }
        if (_state == STATE_PLAYING) {
            proc->discSetMousePos(pos.x(), pos.y());
        }
    }
}

void TPlayer::displayScreenshotName(const QString& filename) {
    logger()->debug("displayScreenshotName: " + filename);

    QFileInfo fi(filename);
    QString text = tr("Screenshot saved as %1").arg(fi.fileName());
    Gui::msgOSD(text);
}

void TPlayer::displayUpdatingFontCache() {
    logger()->debug("displayUpdatingFontCache");
    Gui::msg(tr("Updating the font cache. This may take some seconds..."));
}

void TPlayer::displayBuffering() {
    Gui::msg(tr("Buffering..."));
}

void TPlayer::displayBufferingEnded() {
    Gui::msg(tr("Playing from %1")
        .arg(Helper::formatTime(qRound(mset.current_sec))));
}

void TPlayer::onReceivedVideoOut() {
    logger()->debug("onReceivedVideoOut");

    // w x h is output resolution selected by player with aspect and filters
    // applied
    playerwindow->setResolution(mdat.video_out_width, mdat.video_out_height);

    // Normally, let the main window know the new video dimension, unless
    // restarting, then need to prevent the main window from resizing itself.
    if (_state != STATE_RESTARTING) {
        emit videoOutResolutionChanged(mdat.video_out_width,
                                       mdat.video_out_height);
    }

    // If resize of main window is canceled adjust new video to the old size
    playerwindow->updateVideoWindow();

    if (_state == STATE_RESTARTING) {
        // Adjust the size factor to the current window size,
        // in case the restart changed the video resolution.
        playerwindow->updateSizeFactor();
    }

    // Update original aspect
    if (mset.aspect_ratio.ID() == TAspectRatio::AspectAuto) {
        mdat.video_aspect_original = playerwindow->aspectRatio();
    }
}

bool TPlayer::setPreferredAudio() {

    if (!pref->audio_lang.isEmpty()) {
        int selected_id = mdat.audios.getSelectedID();
        if (selected_id >= 0) {
            int wanted_id = mdat.audios.findLangID(pref->audio_lang);
            if (wanted_id >= 0 && wanted_id != selected_id) {
                logger()->debug("setPreferredAudio: selecting preferred audio"
                                " with id %1", wanted_id);
                changeAudioTrack(wanted_id);
                return true;
            }
        }
    }

    logger()->debug("setPreferredAudio: no player overrides");
    return false;
}

void TPlayer::onAudioTracksChanged() {
    logger()->debug("onAudioTracksChanged");

    // First update the track actions, so a possible track change by
    // setpreferredAudio() will arrive on defined actions
    emit audioTracksChanged();

    // Check if one of the audio tracks matches the preferred language
    setPreferredAudio();
}

void TPlayer::selectPreferredSubtitles() {

    // Select no subtitles
    int wanted_idx = -1;
    if (mdat.subs.count() > 0) {
        // Select subtitles with preferred language
        if (!pref->language.isEmpty()) {
            wanted_idx = mdat.subs.findLangIdx(pref->language);
        }
        // Keep subtitles selected by the player
        if (wanted_idx < 0 && mset.current_sub_idx >= 0) {
            wanted_idx = mset.current_sub_idx;
        }
        // Select first subtitles
        if (wanted_idx < 0 && pref->select_first_subtitle) {
            wanted_idx = 0;
        }
    }

    if (wanted_idx < 0) {
        logger()->debug("selectPreferredSubtitles: no player overrides");
    } else if (wanted_idx == mset.current_sub_idx) {
        logger()->debug("selectPreferredSubtitles: keeping selected subtitles");
    } else {
        logger()->debug("selectPreferredSubtitles: selecting preferred"
                        " subtitles");
        changeSubtitle(wanted_idx, false);
    }
}

void TPlayer::onSubtitlesChanged() {
    logger()->debug("onSubtitlesChanged");

    // Need to set current_sub_idx, the subtitle action group checks on it.
    mset.current_sub_idx = mdat.subs.findSelectedIdx();
    mset.current_secondary_sub_idx = mdat.subs.findSelectedSecondaryIdx();
    emit subtitlesChanged();

    if (pref->isMPlayer()) {
        // MPlayer selected sub will not yet be updated, que the subtitle
        // selection
        logger()->debug("onSubtitlesChanged: posting selectPreferredSubtitles()");
        QTimer::singleShot(1500, this, SLOT(selectPreferredSubtitles()));
    } else {
        selectPreferredSubtitles();
    }
}

// Called when player changed subtitle track
void TPlayer::onSubtitleChanged() {
    logger()->debug("onSubtitleChanged");

    // Need to set current_sub_idx, the subtitle group checks on it.
    int selected_idx = mdat.subs.findSelectedIdx();
    mset.current_sub_idx = selected_idx;

    mset.current_secondary_sub_idx = mdat.subs.findSelectedSecondaryIdx();

    emit subtitleTrackChanged(selected_idx);
}

} // namespace Player

#include "moc_player.cpp"
