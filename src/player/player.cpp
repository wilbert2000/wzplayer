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
#include "player/process/playerprocess.h"
#include "player/process/exitmsg.h"

#include "gui/playerwindow.h"
#include "gui/msg.h"
#include "desktop.h"

#include "settings/preferences.h"
#include "settings/mediasettings.h"
#include "settings/aspectratio.h"
#include "settings/filesettingshash.h"
#include "settings/filesettings.h"
#include "settings/tvsettings.h"
#include "settings/filters.h"
#include "settings/paths.h"

#include "wztime.h"
#include "discname.h"
#include "mediadata.h"
#include "extensions.h"
#include "colorutils.h"
#include "wzfiles.h"

#include <QDir>
#include <QFileInfo>
#include <QRegExp>
#include <QTextStream>
#include <QUrl>
#include <QNetworkProxy>
#include <QTimer>
#include <QApplication>


Player::TPlayer* player = 0;

namespace Player {

double TPlayer::restartTime = 0;
bool TPlayer::startPausedOnce = false;

TPlayer::TPlayer(QWidget* parent, Gui::TPlayerWindow* pw) :
    QObject(parent),
    debug(logger()),
    mdat(),
    mset(&mdat),
    keepSize(false),
    playerwindow(pw),
    _state(STATE_LOADING) {

    qRegisterMetaType<TState>("Player::TState");

    player = this;

    keepSizeTimer = new QTimer(this);
    keepSizeTimer->setInterval(1000);
    keepSizeTimer->setSingleShot(true);
    connect(keepSizeTimer, &QTimer::timeout, this, &TPlayer::clearKeepSize);

    proc = Player::Process::TPlayerProcess::createPlayerProcess(this, &mdat);

    connect(proc, &Process::TPlayerProcess::errorOccurred,
            this, &TPlayer::onProcessError);

    connect(proc, &Process::TPlayerProcess::processFinished,
            this, &TPlayer::onProcessFinished);

    connect(proc, &Process::TPlayerProcess::playerFullyLoaded,
            this, &TPlayer::onPlayingStarted);

    connect(proc, &Process::TPlayerProcess::receivedPosition,
            this, &TPlayer::onReceivedPosition);

    connect(proc, &Process::TPlayerProcess::receivedPause,
            this, &TPlayer::onReceivedPause);

    connect(proc, &Process::TPlayerProcess::receivedBuffering,
            this, &TPlayer::displayBuffering);

    connect(proc, &Process::TPlayerProcess::receivedBufferingEnded,
            this, &TPlayer::displayBufferingEnded);

    connect(proc, &Process::TPlayerProcess::receivedMessage,
            this, &TPlayer::onReceivedMessage);

    connect(proc, &Process::TPlayerProcess::receivedScreenshot,
            this, &TPlayer::displayScreenshotName);

    connect(proc, &Process::TPlayerProcess::receivedUpdatingFontCache,
            this, &TPlayer::displayUpdatingFontCache);

    connect(proc, &Process::TPlayerProcess::receivedVideoOut,
            this, &TPlayer::onReceivedVideoOut);

    connect(proc, &Process::TPlayerProcess::receivedStreamTitle,
            this, &TPlayer::mediaInfoChanged);

    connect(proc, &Process::TPlayerProcess::receivedVideoTracks,
            this, &TPlayer::videoTracksChanged);
    connect(proc, &Process::TPlayerProcess::receivedVideoTrackChanged,
            this, &TPlayer::videoTrackChanged);

    connect(proc, &Process::TPlayerProcess::receivedAudioTracks,
            this, &TPlayer::onAudioTracksChanged);
    connect(proc, &Process::TPlayerProcess::receivedAudioTrackChanged,
            this, &TPlayer::audioTrackChanged);

    connect(proc, &Process::TPlayerProcess::receivedSubtitleTracks,
            this, &TPlayer::onSubtitlesChanged);
    connect(proc, &Process::TPlayerProcess::receivedSubtitleTrackChanged,
            this, &TPlayer::onSubtitleChanged);

    connect(proc, &Process::TPlayerProcess::receivedTitleTracks,
            this, &TPlayer::titleTracksChanged);
    connect(proc, &Process::TPlayerProcess::receivedTitleTrackChanged,
            this, &TPlayer::titleTrackChanged);

    connect(proc, &Process::TPlayerProcess::receivedChapters,
            this, &TPlayer::chaptersChanged);

    connect(proc, &Process::TPlayerProcess::receivedAngles,
            this, &TPlayer::anglesChanged);

    connect(proc, &Process::TPlayerProcess::durationChanged,
            this, &TPlayer::durationChanged);

    connect(proc, &Process::TPlayerProcess::videoBitRateChanged,
            this, &TPlayer::videoBitRateChanged);

    connect(proc, &Process::TPlayerProcess::audioBitRateChanged,
            this, &TPlayer::audioBitRateChanged);
}

TPlayer::~TPlayer() {

    player = 0;
}

void TPlayer::setState(TState s) {

    if (s != _state) {
        WZDEBUG(QString("State changed from %1 to %2 at %3")
                .arg(stateToString())
                .arg(stateToString(s))
                .arg(mset.current_sec));
        _state = s;
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

void TPlayer::onProcessError(QProcess::ProcessError error) {
    WZERROR(QString::number(error));

    // Restore normal window background
    playerwindow->restoreNormalWindow(false);
    emit playerError(Player::Process::TExitMsg::processErrorToErrorID(error));
}

void TPlayer::onProcessFinished(bool normal_exit, int exit_code, bool eof) {
    WZDEBUG(QString("Normal exit %1, exit code %2, eof %3, state %4")
            .arg(normal_exit).arg(exit_code).arg(eof).arg(stateToString()));

    // Restore normal window background
    playerwindow->restoreNormalWindow(false);

    if (_state == STATE_STOPPING || _state == STATE_STOPPED) {
        return;
    }

    // Clear eof and catch unreported error when still loading
    if (_state == STATE_LOADING) {
        eof = false;
        if (normal_exit) {
            normal_exit = false;
            exit_code = Player::Process::TExitMsg::ERR_QUIT_WITHOUT_PLAY;
        }
    }

    WZDEBUG("Entering the stopped state");
    setState(STATE_STOPPED);

    if (eof || exit_code == Player::Process::TExitMsg::EXIT_OUT_POINT_REACHED) {
        WZDEBUG("Emit mediaEOF()");
        emit mediaEOF();
    } else if (!normal_exit) {
        WZDEBUG("Emit playerError()");
        emit playerError(exit_code);
    }
}

void TPlayer::onReceivedMessage(const QString& s) {

    if (!mdat.image) {
        Gui::msg2(s);
    }
}

void TPlayer::saveMediaSettings() {

    if (mdat.filename.isEmpty()) {
        return;
    }
    if (!Settings::pref->remember_media_settings) {
        WZTRACE("Save settings per file is disabled");
        return;
    }
    WZINFO("Saving settings for '" + mdat.filename + "'");
    Gui::msg(tr("Saving settings for %1").arg(mdat.filename), 0);

    if (mdat.selected_type == TMediaData::TYPE_FILE) {
        if (Settings::pref->file_settings_method.toLower() == "hash") {
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

void TPlayer::close(TState nextState) {
    WZDEBUG("");

    stopPlayer();
    // Save data previous file:
    saveMediaSettings();
    // Set state
    if (_state == nextState) {
        WZDEBUG("Closed");
    } else {
        WZDEBUG("Closed. Entering state " + stateToString(nextState));
        setState(nextState);
    }
    // Clear media data
    mdat = TMediaData();
}

void TPlayer::openDisc(TDiscName disc, bool fast_open) {
    WZDEBUG(disc.toString() + " fast open " + QString::number(fast_open));

    // Change title if already playing
    if (fast_open
        && !mset.playing_single_track
        && statePOP()
        && disc.title > 0
        && mdat.disc.valid
        && mdat.disc.device == disc.device) {
        // If setTitle fails, it will call again with fast_open set to false
        WZDEBUG("trying setTitle(" + QString::number(disc.title) + ")");
        setTitle(disc.title);
        return;
    }

    Gui::msg(tr("Opening %1").arg(disc.toString()), 0);

    // Add device from pref if none specified
    if (disc.device.isEmpty()) {
        if (disc.protocol == "vcd" || disc.protocol == "cdda") {
            disc.device = Settings::pref->cdrom_device;
        } else if (disc.protocol == "br") {
            disc.device = Settings::pref->bluray_device;
        } else {
            disc.device = Settings::pref->dvd_device;
        }
    }

    // Test access, correct missing /
    if (!QFileInfo(disc.device).exists()) {
        WZWARN("could not access '" + disc.device + "'");
        // Forgot a /?
        if (QFileInfo("/" + disc.device).exists()) {
            WZWARN("adding missing /");
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

    startPlayer();
    return;
} // openDisc

// Generic open, autodetect type
void TPlayer::open(QString filename, bool loopImage) {
    WZDEBUG("'" + filename + "'");

    if (filename.isEmpty()) {
        filename = mdat.filename;
        if (filename.isEmpty()) {
            Gui::msg(tr("No file to play"));
            return;
        }
    }

    QUrl url(filename);
    QString scheme = url.scheme();
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
        QRegExp ext_sub(extensions.subtitles().forRegExp(),
                        Qt::CaseInsensitive);
        if (ext_sub.indexIn(fi.suffix()) >= 0) {
            loadSub(filename);
            return;
        }

        if (fi.isDir()) {
            if (TWZFiles::directoryContainsDVD(filename)) {
                WZDEBUG("Directory contains a dvd");
                disc = TDiscName(filename, Settings::pref->useDVDNAV());
                openDisc(disc);
                return;
            }
            WZERROR("File is a directory, use playlist to open it");
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

void TPlayer::openFile(const QString& filename, bool loopImage) {
    WZDEBUG("'" + filename + "'");

    close(STATE_LOADING);
    mdat.filename = QDir::toNativeSeparators(filename);
    mdat.selected_type = TMediaData::TYPE_FILE;
    mdat.image = extensions.isImage(mdat.filename);
    mset.reset();

    // Check if we have info about this file
    if (Settings::pref->remember_media_settings) {
        if (Settings::pref->file_settings_method.toLower() == "hash") {
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

        if (!Settings::pref->remember_time_pos) {
            mset.current_sec = 0;
            WZDEBUG("play position reset to 0");
        }
    }

    startPlayer(loopImage);
}

void TPlayer::openTV(QString channel_id) {
    WZDEBUG(channel_id);

    close(STATE_LOADING);

    // Use last channel if the name is just "dvb://" or "tv://"
    if (channel_id == "dvb://" && !Settings::pref->last_dvb_channel.isEmpty()) {
        channel_id = Settings::pref->last_dvb_channel;
    } else if (channel_id == "tv://"
               && !Settings::pref->last_tv_channel.isEmpty()) {
        channel_id = Settings::pref->last_tv_channel;
    }

    // Save last channel
    if (channel_id.startsWith("dvb://")) {
        Settings::pref->last_dvb_channel = channel_id;
    } else if (channel_id.startsWith("tv://")) {
        Settings::pref->last_tv_channel = channel_id;
    }

    mdat.filename = channel_id;
    mdat.selected_type = TMediaData::TYPE_TV;

    mset.reset();
    // Set the default deinterlacer for TV
    mset.current_deinterlacer = Settings::pref->initial_tv_deinterlace;
    // Load settings
    if (Settings::pref->remember_media_settings) {
        Settings::TTVSettings settings;
        if (settings.existSettingsFor(channel_id)) {
            settings.loadSettingsFor(channel_id, mset);
        }
    }

    startPlayer();
}

void TPlayer::openStream(const QString& name) {
    WZDEBUG("'" + name + "'");

    close(STATE_LOADING);
    mdat.filename = name;
    mdat.selected_type = TMediaData::TYPE_STREAM;
    mset.reset();

    startPlayer();
}

bool TPlayer::hasExternalSubs() const {
    return mdat.subs.hasFileSubs()
        || (mset.sub.type() == SubData::Vob && !mset.sub.filename().isEmpty());
}

void TPlayer::setExternalSubs(const QString &filename) {

    mset.current_sub_set_by_user = true;
    mset.current_sub_idx = Settings::TMediaSettings::NoneSelected;
    mset.sub.setID(Settings::TMediaSettings::NoneSelected);
    mset.sub.setType(SubData::File);
    // For mplayer assume vob if file extension idx
    if (Settings::pref->isMPlayer()) {
        QFileInfo fi(filename);
        if (fi.suffix().toLower() == "idx") {
            mset.sub.setType(SubData::Vob);
        }
    }
    mset.sub.setFilename(filename);
}

void TPlayer::loadSub(const QString & sub) {
    WZDEBUG("");

    if (!sub.isEmpty() && QFile::exists(sub)) {
        setExternalSubs(sub);
        if (mset.external_subtitles_fps
            != Settings::TMediaSettings::SFPS_None) {
            restartPlayer();
        } else {
            proc->setExternalSubtitleFile(sub);
        }
    } else {
        WZWARN("file not found '" + sub + "'");
    }
}

void TPlayer::unloadSub() {

    mset.current_sub_set_by_user = false;
    mset.current_sub_idx = Settings::TMediaSettings::NoneSelected;
    mset.sub = SubData();

    restartPlayer();
}

void TPlayer::loadAudioFile(const QString& audiofile) {

    if (!audiofile.isEmpty()) {
        mset.external_audio = audiofile;
        restartPlayer();
    }
}

void TPlayer::unloadAudioFile() {

    if (!mset.external_audio.isEmpty()) {
        mset.external_audio = "";
        restartPlayer();
    }
}

void TPlayer::stopPlayer() {

    if (proc->state() == QProcess::NotRunning) {
        return;
    }

    int timeout = Settings::pref->time_to_kill_player;
    WZDEBUG(QString("Entering the stopping state with timeout of %1 ms")
            .arg(timeout));
    setState(STATE_STOPPING);
    QTime time;
    time.start();
    proc->quit(0);

    int lastSec = timeout;
    while (proc->state() == QProcess::Running) {
        int elapsed = time.elapsed();
        if (elapsed >= timeout) {
            break;
        }
        int sec = qRound(float(timeout - elapsed) / 1000);
        if (sec < lastSec) {
            lastSec = sec;
            Gui::msg(tr("Waiting %1 seconds for player to quit...").arg(sec));
        }
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 100);
    };

    if (proc->state() == QProcess::Running) {
        WZWARN(QString("Player did not quit in %1 ms, killing it...")
               .arg(timeout));
        proc->kill();
    }

    WZDEBUG(QString("Player stopped in %1 ms").arg(time.elapsed()));
}

void TPlayer::stop() {
    WZDEBUG("Current state " + stateToString());

    stopPlayer();
    WZDEBUG("Entering the stopped state");
    setState(STATE_STOPPED);
}

void TPlayer::play() {
    WZDEBUG("Current state " + stateToString());

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
                restartPlayer();
            }
            break;
        case QProcess::Starting:
            break;
        default:
            break;
    }
}

void TPlayer::pause() {
    WZDEBUG("current state " + stateToString());

    if (proc->isRunning() && _state != STATE_PAUSED) {
        proc->setPause(true);
        setState(STATE_PAUSED);
    }
}

void TPlayer::playOrPause() {
    WZDEBUG("current state " + stateToString());

    if (_state == STATE_PLAYING) {
        pause();
    } else {
        play();
    }
}

// Save current state and restart player
void TPlayer::restartPlayer() {
    WZDEBUG("");

    saveRestartState();
    stopPlayer();

    WZDEBUG("Entering the restarting state");
    setState(STATE_RESTARTING);
    startPlayer();
}

// Public restart
void TPlayer::restart() {
    WZDEBUG("");

    if (proc->isReady()) {
        restartPlayer();
    } else {
        WZWARN("Player not ready");
    }
}

void TPlayer::reload() {
    WZDEBUG("");

    stopPlayer();
    WZDEBUG("Entering the loading state");
    setState(STATE_LOADING);
    startPlayer();
}

void TPlayer::initVolume() {

    // Keep currrent volume if no media settings are loaded.
    // restore_volume is set to true by mset.reset and set
    // to false by mset.load
    if (mset.restore_volume) {
        WZDEBUG("keeping current volume");
        mset.volume = mset.old_volume;
        mset.mute = mset.old_mute;
    } else if (!Settings::pref->global_volume) {
        if (mset.old_volume != mset.volume) {
            WZDEBUG("emit volumeChanged()");
            emit volumeChanged(mset.volume);
        }
        if (mset.old_mute != mset.mute) {
            WZDEBUG("emit muteChanged()");
            emit muteChanged(mset.mute);
        }
    }
}

void TPlayer::initMediaSettings() {
    WZDEBUG("");

    // Restore old volume or emit new volume
    initVolume();

    // Apply settings to playerwindow, false = do not update video window
    playerwindow->setZoom(mset.zoom_factor, mset.zoom_factor_fullscreen, false);
    playerwindow->setPan(mset.pan_offset, mset.pan_offset_fullscreen);

    emit mediaSettingsChanged();
}

void TPlayer::onPlayingStartedNewMedia() {
    WZDEBUG("");

    mdat.list();

    // Copy the demuxer
    mset.current_demuxer = mdat.demuxer;
    if (Settings::pref->remember_media_settings) {
        mset.list();
    }

    emit newMediaStartedPlaying();
}

// Slot called when signal playerFullyLoaded arrives.
void TPlayer::onPlayingStarted() {
    WZDEBUG("");

    if (forced_titles.contains(mdat.filename)) {
        mdat.title = forced_titles[mdat.filename];
    }

    if (_state != STATE_RESTARTING) {
        onPlayingStartedNewMedia();
    }

    setState(STATE_PLAYING);

    WZDEBUG("emit mediaInfoChanged()");
    emit mediaInfoChanged();

    WZDEBUG("done in " + QString::number(time.elapsed()) + " ms");
}

void TPlayer::clearOSD() {
    WZDEBUG("");
    displayTextOnOSD("", 0, Settings::pref->osd_level);
}

void TPlayer::displayTextOnOSD(const QString& text, int duration, int level) {

    if (proc->isReady() && level <= Settings::pref->osd_level
        && mdat.hasVideo()) {
        proc->showOSDText(text, duration, level);
    }
}

void TPlayer::frameStep() {
    WZDEBUG("at " + QString::number(mset.current_sec));

    if (proc->isRunning()) {
        if (_state == STATE_PAUSED) {
            proc->frameStep();
        } else {
            pause();
        }
    }
}

void TPlayer::frameBackStep() {
    WZDEBUG("at " + QString::number(mset.current_sec));

    if (proc->isRunning()) {
        if (_state == STATE_PAUSED) {
            proc->frameBackStep();
        } else {
            pause();
        }
    }
}

void TPlayer::screenshot() {
    WZDEBUG("");

    if (Settings::pref->use_screenshot
        && !Settings::pref->screenshot_directory.isEmpty()) {
        proc->takeScreenshot(Player::Process::TPlayerProcess::Single,
                             Settings::pref->subtitles_on_screenshots);
        WZDEBUG("took screenshot");
    } else {
        WZWARN("directory for screenshots not valid or not enabled");
        Gui::msg(tr("Screenshot NOT taken, folder not configured or enabled"));
    }
}

void TPlayer::screenshots() {
    WZDEBUG("");

    if (!Settings::pref->use_screenshot) {
        WZWARN("Screenshots are disabled in capture section of settings");
        Gui::msg(tr("Screenshots are disabled in capture section of settings"));
        return;
    }
    if (Settings::pref->screenshot_directory.isEmpty()) {
        WZWARN("Directory not set in capture section of settings");
        Gui::msg(tr("Directory not set in capture section of settings"));
        return;
    }

    proc->takeScreenshot(Player::Process::TPlayerProcess::Multiple,
                         Settings::pref->subtitles_on_screenshots);
}

void TPlayer::switchCapturing() {
    WZDEBUG("");
    proc->switchCapturing();
}

bool TPlayer::haveVideoFilters() const {

    return mset.phase_filter
        || mset.current_deinterlacer != Settings::TMediaSettings::NoDeinterlace
        || (mset.stereo3d_in != "none" && !mset.stereo3d_out.isEmpty())
        || mset.current_denoiser != Settings::TMediaSettings::NoDenoise
        || mset.current_unsharp
        || mset.deblock_filter
        || mset.dering_filter
        || mset.gradfun_filter
        || mset.upscaling_filter
        || mset.noise_filter
        || mset.postprocessing_filter
        || mset.add_letterbox
        || Settings::pref->use_soft_video_eq
        || !mset.player_additional_video_filters.isEmpty()
        || !Settings::pref->player_additional_video_filters.isEmpty()
        || mset.rotate
        || mset.flip
        || mset.mirror
        || mset.color_space
           != Settings::TMediaSettings::TColorSpace::COLORSPACE_AUTO;
}

#ifdef Q_OS_WIN
bool TPlayer::videoFiltersEnabled(bool) {
    return true;
}
#else
bool TPlayer::videoFiltersEnabled(bool displayMessage) {

    bool enabled = true;

    if (Settings::pref->isMPlayer()) {
        QString msg;
        if (Settings::pref->vo.startsWith("vdpau")) {
            enabled = !Settings::pref->vdpau.disable_video_filters;
            if (enabled) {
                msg = tr("The video driver settings for vdpau allow video "
                         " filters, this might not work...");
            } else {
                msg = tr("Using vdpau, ignoring video filters");
            }
        }

        if (!msg.isEmpty() && displayMessage && haveVideoFilters()) {
            WZDEBUG(msg);
            Gui::msg(msg, 0);
        }
    }

    return enabled;
}
#endif

void TPlayer::saveRestartState() {
    WZDEBUG("");

    // Note: when starting mset.current_sec is already used as start time.
    // This static start time is used to survive restarting the app and to
    // override start times saved in mset

    // Save state proc, currently only used by TMPlayerProcess for DVDNAV.
    // DVDNAV restores its own start time and paused state
    proc->save();
    if (mdat.detected_type != TMediaData::TYPE_DVDNAV) {
        restartTime = mset.current_sec;
        startPausedOnce = _state == STATE_PAUSED;
    }
}

void TPlayer::startPlayer(bool loopImage) {

    WZDEBUG("starting time");
    time.start();

    if (_state != STATE_RESTARTING) {
        // Clear background
        playerwindow->repaint();
        // Reset media settings
        initMediaSettings();
    }

    QString fileName = mdat.filename;
    if (fileName.isEmpty()) {
        WZWARN("file name is empty");
        return;
    }

    if (proc->isRunning()) {
        WZWARN("player still running");
        return;
    }

    WZDEBUG("'" + fileName + "'");
    Gui::msg(tr("Starting %1...").arg(fileName), 0);

    // Check URL playlist
    if (fileName.endsWith("|playlist")) {
        fileName = fileName.remove("|playlist");
    }

    // Load m4a audio file with the same name as file
    if (Settings::pref->autoload_m4a && mset.external_audio.isEmpty()) {
        QFileInfo fi(fileName);
        if (fi.exists() && !fi.isDir() && fi.suffix().toLower() == "mp4") {
            QString file2 = fi.path() + "/" + fi.completeBaseName() + ".m4a";
            if (!QFile::exists(file2)) {
                // Check for upper case
                file2 = fi.path() + "/" + fi.completeBaseName() + ".M4A";
            }
            if (QFile::exists(file2)) {
                WZDEBUG("using external audio file '" + file2 + "'"),
                mset.external_audio = file2;
            }
        }
    }

    proc->clearArguments();
    proc->setExecutable(Settings::pref->player_bin);
    proc->setFixedOptions();
    proc->disableInput();
    proc->setOption("wid",
        QString::number((qint64) playerwindow->videoWindow()->winId()));
    if (Settings::pref->log_verbose) {
        proc->setOption("verbose");
    }

    // Seek to in point, mset.current_sec or restartTime
    seeking = false;
    {
        // Clear start time
        double ss = 0;
        if (mdat.selected_type == TMediaData::TYPE_FILE) {
            if (mdat.image) {
                proc->setImageDuration(Settings::pref->imageDuration);
                // loopImage is set by TPlaylist::playItem() when playlist
                // has only a single item
                if (loopImage) {
                    proc->setOption("loop", 0 /* loop forever */);
                }
            }

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

        // Set mset.current_sec
        mset.current_sec = ss;
        emit positionChanged(ss);
    }

    restartTime = 0;
    if (startPausedOnce) {
        startPausedOnce = false;
        proc->setOption("pause");
    }

    // Setup hardware decoding for MPV.
    // First set mdat.video_hwdec, handle setting hwdec option later
    QString hwdec = Settings::pref->hwdec;
    if (Settings::pref->isMPV()) {
        // Disable hardware decoding when there are filters in use
        if (hwdec != "no" && haveVideoFilters()) {
            hwdec = "no";
            QString msg = tr("Disabled hardware decoding for video filters");
            WZDEBUG(msg);
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
        if (Settings::pref->vo.startsWith("vdpau")) {
            if (Settings::pref->isMPlayer()) {
                QString c;
                if (Settings::pref->vdpau.ffh264vdpau) c = "ffh264vdpau,";
                if (Settings::pref->vdpau.ffmpeg12vdpau) c += "ffmpeg12vdpau,";
                if (Settings::pref->vdpau.ffwmv3vdpau) c += "ffwmv3vdpau,";
                if (Settings::pref->vdpau.ffvc1vdpau) c += "ffvc1vdpau,";
                if (Settings::pref->vdpau.ffodivxvdpau) c += "ffodivxvdpau,";
                if (!c.isEmpty()) {
                    proc->setOption("vc", c);
                }
            } else if (mdat.video_hwdec) {
                QString c;
                if (Settings::pref->vdpau.ffh264vdpau) c = ",h264";
                if (Settings::pref->vdpau.ffmpeg12vdpau) c += ",mpeg1video,mpeg2video";
                if (Settings::pref->vdpau.ffwmv3vdpau) c += ",wmv3";
                if (Settings::pref->vdpau.ffvc1vdpau) c += ",vc1";
                if (Settings::pref->vdpau.ffodivxvdpau) c += ",mpeg4";
                if (!c.isEmpty()) {
                    proc->setOption("hwdec-codecs", c.mid(1));
                }
            }
        }
#endif

    }

    // MPV only
    if (!hwdec.isEmpty() && Settings::pref->isMPV()) {
        proc->setOption("hwdec", hwdec);
    }

    if (!Settings::pref->vo.isEmpty()) {
        proc->setOption("vo", Settings::pref->vo);
    }

    // Enable software scaling for vo x11
#if !defined(Q_OS_WIN)
    if (Settings::pref->vo.startsWith("x11")) {
        proc->setOption("zoom");
    }
#endif

    if (!Settings::pref->ao.isEmpty()) {
        proc->setOption("ao", Settings::pref->ao);
    }

    if (mset.current_video_id >= 0) {
        proc->setOption("vid", QString::number(mset.current_video_id));
    }

    if (mset.external_audio.isEmpty() && mset.current_audio_id >= 0) {
        proc->setOption("aid", QString::number(mset.current_audio_id));
    }

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
    double aspect = Settings::pref->monitorAspectDouble();
    if (aspect > 0) {
        proc->setOption("monitorpixelaspect", aspect);
    }

    // Colorkey, only used by XP directx to set color key for overlay
    if (Settings::pref->useColorKey()) {
        proc->setOption("colorkey",
                        TColorUtils::colorToRGB(Settings::pref->color_key));
    }

    // Synchronization
    if (Settings::pref->frame_drop && Settings::pref->hard_frame_drop) {
        proc->setOption("framedrop", "decoder+vo");
    } else if (Settings::pref->frame_drop) {
        proc->setOption("framedrop", "vo");
    } else if (Settings::pref->hard_frame_drop) {
        proc->setOption("framedrop", "decoder");
    }
    if (Settings::pref->autosync) {
        proc->setOption("autosync",
                        QString::number(Settings::pref->autosync_factor));
    }
    if (Settings::pref->use_mc) {
        proc->setOption("mc", QString::number(Settings::pref->mc_value));
    }

    // OSD
    proc->setOption("osdlevel", Settings::pref->osd_level);
    if (Settings::pref->isMPlayer()) {
        proc->setOption("osd-scale", Settings::pref->subfont_osd_scale);
    } else {
        proc->setOption("osd-scale", Settings::pref->osd_scale);
        proc->setOption("osd-scale-by-window", "no");
    }

    // Subtitle search fuzziness
    proc->setOption("sub-fuzziness", Settings::pref->subtitle_fuzziness);

    // Subtitles fonts
    if (Settings::pref->use_ass_subtitles && Settings::pref->freetype_support) {
        // Use ASS options
        proc->setOption("ass");
        proc->setOption("embeddedfonts");
        proc->setOption("ass-font-scale", QString::number(mset.sub_scale_ass));
        proc->setOption("ass-line-spacing",
                        QString::number(Settings::pref->ass_line_spacing));

        // Custom ASS style
        if (Settings::pref->use_custom_ass_style) {
            QString ass_force_style;
            if (Settings::pref->user_forced_ass_style.isEmpty()) {
                ass_force_style = Settings::pref->ass_styles.toString();
            } else {
                ass_force_style = Settings::pref->user_forced_ass_style;
            }

            if (Settings::pref->isMPV()) {
                proc->setSubStyles(Settings::pref->ass_styles);
                if (Settings::pref->force_ass_styles) {
                    proc->setOption("ass-force-style", ass_force_style);
                }
            } else {
                if (Settings::pref->force_ass_styles) {
                    proc->setOption("ass-force-style", ass_force_style);
                } else {
                    proc->setSubStyles(Settings::pref->ass_styles,
                                       Settings::TPaths::subtitleStyleFileName());
                }
            }
        }
    } else {
        // NO ASS
        if (Settings::pref->freetype_support)
            proc->setOption("noass");
        if (Settings::pref->isMPV()) {
            proc->setOption("sub-scale", QString::number(mset.sub_scale_mpv));
        } else {
            proc->setOption("subfont-text-scale",
                            QString::number(mset.sub_scale));
        }
    }

    // Subtitle encoding
    if (Settings::pref->subtitle_enca_language.isEmpty()) {
        // No encoding language, set fallback code page
        if (!Settings::pref->subtitle_encoding_fallback.isEmpty()) {
            if (Settings::pref->isMPlayer()) {
                proc->setOption("subcp",
                                Settings::pref->subtitle_encoding_fallback);
            } else if (Settings::pref->subtitle_encoding_fallback != "UTF-8") {
                // Use pref->subtitle_encoding_fallback if encoding is not utf8
                proc->setOption("subcp",
                                "utf8:"
                                + Settings::pref->subtitle_encoding_fallback);
            }
        }
    } else {
        // Add subtitle encoding enca language
        QString encoding = "enca:"+ Settings::pref->subtitle_enca_language;
        // Add subtitle encoding fallback
        if (!Settings::pref->subtitle_encoding_fallback.isEmpty()) {
            encoding += ":"+ Settings::pref->subtitle_encoding_fallback;
        }
        proc->setOption("subcp", encoding);
    }

    if (mset.closed_caption_channel > 0) {
        proc->setOption("subcc", QString::number(mset.closed_caption_channel));
    }

    if (Settings::pref->use_forced_subs_only) {
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
        && mset.external_subtitles_fps != Settings::TMediaSettings::SFPS_None) {

        QString fps;
        switch (mset.external_subtitles_fps) {
            case Settings::TMediaSettings::SFPS_23: fps = "23"; break;
            case Settings::TMediaSettings::SFPS_24: fps = "24"; break;
            case Settings::TMediaSettings::SFPS_25: fps = "25"; break;
            case Settings::TMediaSettings::SFPS_30: fps = "30"; break;
            case Settings::TMediaSettings::SFPS_23976:
                fps = "24000/1001";
                break;
            case Settings::TMediaSettings::SFPS_29970:
                fps = "30000/1001";
                break;
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
    if (Settings::pref->change_video_equalizer_on_startup) {
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

    // Color space video filter
    if (Settings::pref->isMPV() && mset.color_space
        != Settings::TMediaSettings::TColorSpace::COLORSPACE_AUTO) {
        proc->setOption("vf-add", "format=colormatrix="
                        + mset.getColorSpaceOptionString());
    }

    if (mset.current_angle > 0) {
        proc->setOption("dvdangle", QString::number(mset.current_angle));
    }

    // TODO: TMPVProcess title and track switch code does not run nicely when
    // caching is set. Seeks inside the cache don't notify track or title
    // changes...
    cache_size = -1;
    if (mdat.selected_type == TMediaData::TYPE_DVDNAV) {
        // Always set no cache for DVDNAV
        cache_size = 0;
    } else if (Settings::pref->cache_enabled) {
        // Enabled cache
        switch (mdat.selected_type) {
            case TMediaData::TYPE_FILE:
                cache_size = Settings::pref->cache_for_files;
                break;
            case TMediaData::TYPE_DVD:
                cache_size = Settings::pref->cache_for_dvds;
                break;
            case TMediaData::TYPE_VCD:
                cache_size = Settings::pref->cache_for_vcds;
                break;
            case TMediaData::TYPE_CDDA:
                cache_size = Settings::pref->cache_for_audiocds;
                break;
            case TMediaData::TYPE_BLURAY:
                cache_size = Settings::pref->cache_for_brs;
                break;
            case TMediaData::TYPE_STREAM:
                cache_size = Settings::pref->cache_for_streams;
                break;
            case TMediaData::TYPE_TV:
                cache_size = Settings::pref->cache_for_tv;
                break;
            default: cache_size = 0;
        } // switch
    }
    if (cache_size >= 0) {
        proc->setOption("cache", QString::number(cache_size));
    }

    if (mset.speed != 1.0) {
        proc->setOption("speed", QString::number(mset.speed));
    }

    if (Settings::pref->use_idx) {
        proc->setOption("idx");
    }

    if (mdat.image
        || Settings::pref->use_correct_pts != Settings::TPreferences::Detect) {
        proc->setOption("correct-pts",
                        !mdat.image
                        && Settings::pref->use_correct_pts
                           == Settings::TPreferences::Enabled);
    }

    // Setup screenshot directory.
    // Needs to be setup before the video filters below use it.
    if (Settings::pref->screenshot_directory.isEmpty()) {
        Settings::pref->use_screenshot = false;
    } else {
        QFileInfo fi(Settings::pref->screenshot_directory);
        if (!fi.isDir() || !fi.isWritable()) {
            WZWARN("disabled screenshots and capture, directory '"
                   + Settings::pref->screenshot_directory
                   + "' is not writable");
            Settings::pref->use_screenshot = false;
            // Need to clear to disable capture
            Settings::pref->screenshot_directory = "";
        }
    }
    if (Settings::pref->use_screenshot) {
        proc->setScreenshotDirectory(Settings::pref->screenshot_directory);
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
    if (mset.current_deinterlacer != Settings::TMediaSettings::NoDeinterlace) {
        switch (mset.current_deinterlacer) {
            case Settings::TMediaSettings::L5: proc->addVF("l5"); break;
            case Settings::TMediaSettings::Yadif: proc->addVF("yadif"); break;
            case Settings::TMediaSettings::LB: proc->addVF("lb"); break;
            case Settings::TMediaSettings::Yadif_1:
                proc->addVF("yadif", "1");
                break;
            case Settings::TMediaSettings::Kerndeint:
                proc->addVF("kerndeint", "5");
                break;
        }
    }

    // 3D stereo
    if (mset.stereo3d_in != "none" && !mset.stereo3d_out.isEmpty()) {
        proc->addStereo3DFilter(mset.stereo3d_in, mset.stereo3d_out);
    }

    // Denoise
    if (mset.current_denoiser != Settings::TMediaSettings::NoDenoise) {
        if (mset.current_denoiser == Settings::TMediaSettings::DenoiseSoft) {
            proc->addVF("hqdn3d",
                        Settings::pref->filters.item("denoise_soft").options());
        } else {
            proc->addVF("hqdn3d",
                        Settings::pref->filters.item("denoise_normal")
                        .options());
        }
    }

    // Unsharp
    if (mset.current_unsharp != 0) {
        if (mset.current_unsharp == 1) {
            proc->addVF("blur", Settings::pref->filters.item("blur").options());
        } else {
            proc->addVF("sharpen",
                        Settings::pref->filters.item("sharpen").options());
        }
    }

    // Deblock
    if (mset.deblock_filter) {
        proc->addVF("deblock",
                    Settings::pref->filters.item("deblock").options());
    }

    // Dering
    if (mset.dering_filter) {
        proc->addVF("dering");
    }

    // Gradfun
    if (mset.gradfun_filter) {
        proc->addVF("gradfun",
                    Settings::pref->filters.item("gradfun").options());
    }

    // Upscale
    if (mset.upscaling_filter) {
        int width = TDesktop::size(playerwindow).width();
        proc->setOption("sws", "9");
        proc->addVF("scale", QString::number(width) + ":-2");
    }

    // Addnoise
    if (mset.noise_filter) {
        proc->addVF("noise", Settings::pref->filters.item("noise").options());
    }

    // Postprocessing
    if (mset.postprocessing_filter) {
        proc->addVF("postprocessing");
        proc->setOption("autoq",
            QString::number(Settings::pref->postprocessing_quality));
    }

    // Letterbox (expand)
    if (mset.add_letterbox) {
        proc->addVF("expand", QString("aspect=%1")
                    .arg(TDesktop::aspectRatio(playerwindow)));
    }

    // Software equalizer
    if (Settings::pref->use_soft_video_eq) {
        proc->addVF("eq2");
        proc->addVF("hue");
        if (Settings::pref->vo == "gl"
            || Settings::pref->vo == "gl2"
            || Settings::pref->vo == "gl_tiled"

#ifdef Q_OS_WIN
            || Settings::pref->vo == "directx:noaccel"
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
    if (!Settings::pref->player_additional_video_filters.isEmpty()) {
        proc->setOption("vf-add",
                        Settings::pref->player_additional_video_filters);
    }

    // Filters for subtitles on screenshots
    if (Settings::pref->use_screenshot
        && Settings::pref->subtitles_on_screenshots) {
        if (Settings::pref->use_ass_subtitles) {
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
    if (Settings::pref->use_screenshot) {
        proc->addVF("screenshot");
    }

end_video_filters:

    // Template for screenshots (only works with mpv)
    if (Settings::pref->isMPV() && Settings::pref->use_screenshot) {
        if (!Settings::pref->screenshot_template.isEmpty()) {
            proc->setOption("screenshot_template",
                            Settings::pref->screenshot_template);
        }
        if (!Settings::pref->screenshot_format.isEmpty()) {
            proc->setOption("screenshot_format",
                            Settings::pref->screenshot_format);
        }
    }

    // Volume
    if (Settings::pref->player_additional_options.contains("-volume")) {
        WZDEBUG("don't set volume since -volume is used");
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

    if (Settings::pref->use_hwac3) {
        proc->setOption("afm", "hwac3");
        WZDEBUG("audio filters are disabled when using the S/PDIF output");
    } else {
        // Audio filters
        if (mset.karaoke_filter) {
            proc->addAF("karaoke");
        }

        // Stereo mode
        if (mset.stereo_mode != 0) {
            switch (mset.stereo_mode) {
                case Settings::TMediaSettings::Left:
                    proc->addAF("channels", "2:2:0:1:0:0");
                    break;
                case Settings::TMediaSettings::Right:
                    proc->addAF("channels", "2:2:1:0:1:1");
                    break;
                case Settings::TMediaSettings::Mono:
                    proc->addAF("pan", "1:0.5:0.5");
                    break;
                case Settings::TMediaSettings::Reverse:
                    proc->addAF("channels", "2:2:0:1:1:0");
                    break;
            }
        }

        if (mset.extrastereo_filter) {
            proc->addAF("extrastereo");
        }

        if (mset.volnorm_filter) {
            proc->addAF("volnorm",
                        Settings::pref->filters.item("volnorm").options());
        }

        if (Settings::pref->use_scaletempo == Settings::TPreferences::Enabled) {
            proc->addAF("scaletempo");
        }

        // Audio equalizer
        if (Settings::pref->use_audio_equalizer) {
            proc->addAF("equalizer", equalizerListToString(getAudioEqualizer()));
        }

        // Additional audio filters
        // Global from pref
        if (!Settings::pref->player_additional_audio_filters.isEmpty()) {
            proc->setOption("af-add",
                            Settings::pref->player_additional_audio_filters);
        }
        // This file from mset
        if (!mset.player_additional_audio_filters.isEmpty()) {
            proc->setOption("af-add", mset.player_additional_audio_filters);
        }
    }


    // Set the capture directory
    proc->setCaptureDirectory(Settings::pref->screenshot_directory);

    // Set preferred ip version
    if (Settings::pref->ipPrefer == Settings::TPreferences::IP_PREFER_4) {
        proc->setOption("prefer-ipv4");
    } else if (Settings::pref->ipPrefer
               == Settings::TPreferences::IP_PREFER_6) {
        proc->setOption("prefer-ipv6");
    }

    // Load edl file
    if (Settings::pref->use_edl_files) {
        QString edl_f;
        QFileInfo f(fileName);
        QString basename = f.path() + "/" + f.completeBaseName();

        if (QFile::exists(basename + ".edl"))
            edl_f = basename + ".edl";
        else if (QFile::exists(basename + ".EDL"))
            edl_f = basename + ".EDL";

        WZDEBUG("edl file: '" + edl_f + "'");
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
                WZDEBUG("adding file specific argument '" + arg + "'");
                proc->addUserOption(arg);
            }
        }
    }

    // Global additional options
    if (!Settings::pref->player_additional_options.isEmpty()) {
        QStringList args = Player::Process::TProcess::splitArguments(
                                     Settings::pref->player_additional_options);
        for (int n = 0; n < args.count(); n++) {
            QString arg = args[n].trimmed();
            if (!arg.isEmpty()) {
                WZDEBUG("adding argument '" + arg + "'");
                proc->addUserOption(arg);
            }
        }
    }

    // Set file and playing msg
    proc->setMedia(fileName);

    // Setup environment
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (Settings::pref->use_proxy
        && Settings::pref->proxy_type == QNetworkProxy::HttpProxy
        && !Settings::pref->proxy_host.isEmpty()) {
        QString proxy = QString("http://%1:%2@%3:%4")
                        .arg(Settings::pref->proxy_username)
                        .arg(Settings::pref->proxy_password)
                        .arg(Settings::pref->proxy_host)
                        .arg(Settings::pref->proxy_port);
        env.insert("http_proxy", proxy);
    }

    proc->setProcessEnvironment(env);

    if (!proc->startPlayer()) {
        // Error reported by onProcessError()
        WZERROR("player process didn't start");
    }
} //startPlayer()

void TPlayer::seekCmd(double secs, int mode) {
    WZDEBUG(QString::number(secs) + " seconds, mode " + QString::number(mode)
            + ", at " + QString::number(mset.current_sec));

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
            WZWARN("seek " + QString::number(secs)
                   + " beyond end of video " + QString::number(mdat.duration));
            // TODO: limit only when mdat.duration is proven reliable...
            // if (mdat.video_fps > 0)
            //    secs = mdat.duration - (1.0 / mdat.video_fps);
            // else secs = mdat.duration - 0.1;
        }
    }

    if (proc->isReady()) {
        proc->seek(secs, mode, Settings::pref->precise_seeking,
                   _state == STATE_PAUSED);
    } else {
        WZWARN("ignored seek, player not ready");
    }
}

void TPlayer::seekRelative(double secs) {
    WZDEBUG(QString::number(secs) + " seconds");
    seekCmd(secs, 0);
}

void TPlayer::seekPercentage(double perc) {
    WZDEBUG(QString::number(perc) + "%");
    seekCmd(perc, 1);
}

void TPlayer::seekTime(double sec) {
    WZDEBUG(QString::number(sec) + " seconds");
    seekCmd(sec, 2);
}

void TPlayer::forward1() {
    WZDEBUG("");
    seekRelative(Settings::pref->seeking1); // +10s
}

void TPlayer::rewind1() {
    WZDEBUG("");
    seekRelative(-Settings::pref->seeking1); // -10s
}


void TPlayer::forward2() {
    WZDEBUG("");
    seekRelative(Settings::pref->seeking2); // +1m
}


void TPlayer::rewind2() {
    WZDEBUG("");
    seekRelative(-Settings::pref->seeking2); // -1m
}


void TPlayer::forward3() {
    WZDEBUG("");
    seekRelative(Settings::pref->seeking3); // +10m
}


void TPlayer::rewind3() {
    WZDEBUG("");
    seekRelative(-Settings::pref->seeking3); // -10m
}

void TPlayer::forward(int secs) {
    WZDEBUG(QString::number(secs));
    seekRelative(secs);
}

void TPlayer::rewind(int secs) {
    WZDEBUG(QString::number(secs));
    seekRelative(-secs);
}

void TPlayer::seekToNextSub() {
    WZDEBUG("");
    proc->seekSub(1);
}

void TPlayer::seekToPrevSub() {
    WZDEBUG("");
    proc->seekSub(-1);
}

void TPlayer::wheelUpFunc(Settings::TPreferences::TWheelFunction function) {
    WZDEBUG("");

    if (function == Settings::TPreferences::DoNothing) {
        function = (Settings::TPreferences::TWheelFunction)
                   Settings::pref->wheel_function;
    }
    switch (function) {
        case Settings::TPreferences::Volume: incVolume(); break;
        case Settings::TPreferences::Zoom: incZoom(); break;
        case Settings::TPreferences::Seeking:
            Settings::pref->wheel_function_seeking_reverse
                    ? rewind(Settings::pref->seeking4)
                    : forward(Settings::pref->seeking4);
            break;
        case Settings::TPreferences::ChangeSpeed : incSpeed10(); break;
        default : {} // do nothing
    }
}

void TPlayer::wheelUpSeeking() {
    wheelUpFunc(Settings::TPreferences::Seeking);
}

void TPlayer::wheelUp() {
    wheelUpFunc(Settings::TPreferences::DoNothing);
}

void TPlayer::wheelDownFunc(Settings::TPreferences::TWheelFunction function) {
    WZDEBUG("");

    if (function == Settings::TPreferences::DoNothing) {
        function = (Settings::TPreferences::TWheelFunction)
                   Settings::pref->wheel_function;
    }
    switch (function) {
        case Settings::TPreferences::Volume: decVolume(); break;
        case Settings::TPreferences::Zoom: decZoom(); break;
        case Settings::TPreferences::Seeking:
            Settings::pref->wheel_function_seeking_reverse
                    ? forward(Settings::pref->seeking4)
                    : rewind(Settings::pref->seeking4);
            break;
        case Settings::TPreferences::ChangeSpeed: decSpeed10(); break;
        default : {} // do nothing
    }
}

void TPlayer::wheelDownSeeking() {
    wheelDownFunc(Settings::TPreferences::Seeking);
}

void TPlayer::wheelDown() {
    wheelDownFunc(Settings::TPreferences::DoNothing);
}

void TPlayer::setInPointSec(double sec) {
    WZDEBUG(QString::number(sec));

    mset.in_point = sec;
    if (mset.in_point < 0) {
        mset.in_point = 0;
    }
    QString msg = tr("In point set to %1")
                  .arg(TWZTime::formatTime(qRound(mset.in_point)));

    if (mset.out_point >= 0 && mset.in_point >= mset.out_point) {
        mset.out_point = -1;
        mset.loop = false;
        updateLoop();
        msg += tr(", cleared out point and repeat");
    }

    emit InOutPointsChanged();
    Gui::msgOSD(msg);
}

void TPlayer::setInPoint() {
    setInPointSec(mset.current_sec);
}

void TPlayer::seekInPoint() {

    seekTime(mset.in_point);
    Gui::msgOSD(tr("Seeking to %1")
                .arg(TWZTime::formatTime(qRound(mset.in_point))));
}

void TPlayer::clearInPoint() {
    WZDEBUG("");

    mset.in_point = 0;
    emit InOutPointsChanged();
    Gui::msgOSD(tr("Cleared in point"));
}

void TPlayer::setOutPointSec(double sec) {
    WZDEBUG(QString::number(sec));

    if (sec <= 0) {
        clearOutPoint();
        return;
    }

    mset.out_point = sec;
    mset.loop = true;

    QString msg;
    msg = tr("Out point set to %1, repeat set")
          .arg(TWZTime::formatTime(qRound(mset.out_point)));
    if (mset.in_point >= mset.out_point) {
        mset.in_point = 0;
        msg += tr(" and cleared in point");
    }

    updateLoop();

    emit InOutPointsChanged();
    Gui::msgOSD(msg);
}

void TPlayer::setOutPoint() {
    setOutPointSec(mset.current_sec);
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
    Gui::msgOSD(tr("Seeking to %1").arg(TWZTime::formatTime(qRound(seek))));
}

void TPlayer::clearOutPoint() {
    WZDEBUG("");

    mset.out_point = -1;
    mset.loop = false;
    updateLoop();

    emit InOutPointsChanged();
    Gui::msgOSD(tr("Cleared out point and repeat"));
}

void TPlayer::clearInOutPoints() {
    WZDEBUG("");

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

void TPlayer::setRepeat(bool b) {
    WZDEBUG(QString::number(b));

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
void TPlayer::setVolnorm(bool b) {
    WZDEBUG(QString::number(b));

    if (b != mset.volnorm_filter) {
        mset.volnorm_filter = b;
        QString f = Settings::pref->filters.item("volnorm").filter();
        proc->enableVolnorm(b,
                            Settings::pref->filters.item("volnorm").options());
    }
}

void TPlayer::setExtrastereo(bool b) {
    WZDEBUG(QString::number(b));

    if (b != mset.extrastereo_filter) {
        mset.extrastereo_filter = b;
        proc->enableExtrastereo(b);
    }
}

void TPlayer::toggleKaraoke(bool b) {
    WZDEBUG(QString::number(b));
    if (b != mset.karaoke_filter) {
        mset.karaoke_filter = b;
        proc->enableKaraoke(b);
    }
}

void TPlayer::setAudioChannels(int channels) {
    WZDEBUG(QString::number(channels));

    if (channels != mset.audio_use_channels) {
        mset.audio_use_channels = channels;
        if (proc->isRunning())
            restartPlayer();
    }
}

void TPlayer::setStereoMode(int mode) {
    WZDEBUG(QString::number(mode));

    if (mode != mset.stereo_mode) {
        mset.stereo_mode = mode;
        if (proc->isRunning())
            restartPlayer();
    }
}


// Video filters

void TPlayer::setVideoFilter(const QString& filter, bool enable,
                       const QVariant& option) {

    if (Settings::pref->isMPV() && !mdat.video_hwdec) {
        proc->setVideoFilter(filter, enable, option);
    } else {
        restartPlayer();
    }
}

void TPlayer::setFlip(bool b) {
    WZDEBUG(QString::number(b));

    if (mset.flip != b) {
        mset.flip = b;
        setVideoFilter("flip", b, QVariant());
    }
}

void TPlayer::setMirror(bool b) {
    WZDEBUG(QString::number(b));

    if (mset.mirror != b) {
        mset.mirror = b;
        setVideoFilter("mirror", b, QVariant());
    }
}

void TPlayer::setPostprocessing(bool b) {
    WZDEBUG(QString::number(b));

    if (b != mset.postprocessing_filter) {
        mset.postprocessing_filter = b;
        setVideoFilter("postprocessing", b, QVariant());
    }
}

void TPlayer::setDeblock(bool b) {
    WZDEBUG(QString::number(b));

    if (b != mset.deblock_filter) {
        mset.deblock_filter = b;
        setVideoFilter("deblock", b,
                       Settings::pref->filters.item("deblock").options());
    }
}

void TPlayer::setDering(bool b) {
    WZDEBUG(QString::number(b));

    if (b != mset.dering_filter) {
        mset.dering_filter = b;
        setVideoFilter("dering", b, QVariant());
    }
}

void TPlayer::setGradfun(bool b) {
    WZDEBUG(QString::number(b));

    if (b != mset.gradfun_filter) {
        mset.gradfun_filter = b;
        setVideoFilter("gradfun", b,
                       Settings::pref->filters.item("gradfun").options());
    }
}

void TPlayer::setNoise(bool b) {
    WZDEBUG(QString::number(b));

    if (b != mset.noise_filter) {
        mset.noise_filter = b;
        setVideoFilter("noise", b, QVariant());
    }
}

void TPlayer::setAutophase(bool b) {
    WZDEBUG(QString::number(b));

    if (b != mset.phase_filter) {
        mset.phase_filter = b;
        setVideoFilter("phase", b, "A");
    }
}

void TPlayer::setDenoiser(int id) {
    WZDEBUG(QString::number(id));

    if (id != mset.current_denoiser) {
        if (Settings::pref->isMPlayer() || mdat.video_hwdec) {
            mset.current_denoiser = id;
            restartPlayer();
        } else {
            // MPV
            QString dsoft = Settings::pref->filters.item("denoise_soft")
                            .options();
            QString dnormal =
                    Settings::pref->filters.item("denoise_normal").options();
            // Remove previous filter
            switch (mset.current_denoiser) {
                case Settings::TMediaSettings::DenoiseSoft:
                    proc->setVideoFilter("hqdn3d", false, dsoft);
                    break;
                case Settings::TMediaSettings::DenoiseNormal:
                    proc->setVideoFilter("hqdn3d", false, dnormal);
                    break;
            }
            // New filter
            mset.current_denoiser = id;
            switch (mset.current_denoiser) {
                case Settings::TMediaSettings::DenoiseSoft:
                    proc->setVideoFilter("hqdn3d", true, dsoft);
                    break;
                case Settings::TMediaSettings::DenoiseNormal:
                    proc->setVideoFilter("hqdn3d", true, dnormal);
                    break;
            }
        }
    }
}

void TPlayer::setSharpen(int id) {
    WZDEBUG(QString::number(id));

    if (id != mset.current_unsharp) {
        if (Settings::pref->isMPlayer() || mdat.video_hwdec) {
            mset.current_unsharp = id;
            restartPlayer();
        } else {
            // MPV
            // Remove previous filter
            switch (mset.current_unsharp) {
                case 1: proc->setVideoFilter("blur", false); break;
                case 2: proc->setVideoFilter("sharpen", false); break;
            }
            // Set new filter
            mset.current_unsharp = id;
            switch (mset.current_unsharp) {
                case 1: proc->setVideoFilter("blur", true); break;
                case 2: proc->setVideoFilter("sharpen", true); break;
            }
        }
    }
}

void TPlayer::setSoftwareScaling(bool b) {
    WZDEBUG(QString::number(b));

    if (mset.upscaling_filter != b) {
        mset.upscaling_filter = b;
        int width = TDesktop::size(playerwindow).width();
        setVideoFilter("scale", b, QString::number(width) + ":-2");
    }
}

void TPlayer::setStereo3D(const QString& in, const QString& out) {
    WZDEBUG("in '" + in + "' out: '" + out + "'");

    if ((mset.stereo3d_in != in) || (mset.stereo3d_out != out)) {
        if (Settings::pref->isMPlayer() || mdat.video_hwdec) {
            mset.stereo3d_in = in;
            mset.stereo3d_out = out;
            restartPlayer();
        } else {
            // Remove previous filter
            if (mset.stereo3d_in != "none" && !mset.stereo3d_out.isEmpty()) {
                proc->setStereo3DFilter(false, mset.stereo3d_in,
                                           mset.stereo3d_out);
            }

            // New filter
            mset.stereo3d_in = in;
            mset.stereo3d_out = out;
            if (mset.stereo3d_in != "none" && !mset.stereo3d_out.isEmpty()) {
                proc->setStereo3DFilter(true, mset.stereo3d_in,
                                           mset.stereo3d_out);
            }
        }
    }
}

void TPlayer::setBrightness(int value) {
    WZDEBUG(QString::number(value));

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
    WZDEBUG(QString::number(value));

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
    WZDEBUG(QString::number(value));

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
    WZDEBUG(QString::number(value));

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
    WZDEBUG(QString::number(value));

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
    setBrightness(mset.brightness + Settings::pref->min_step);
}

void TPlayer::decBrightness() {
    setBrightness(mset.brightness - Settings::pref->min_step);
}

void TPlayer::incContrast() {
    setContrast(mset.contrast + Settings::pref->min_step);
}

void TPlayer::decContrast() {
    setContrast(mset.contrast - Settings::pref->min_step);
}

void TPlayer::incGamma() {
    setGamma(mset.gamma + Settings::pref->min_step);
}

void TPlayer::decGamma() {
    setGamma(mset.gamma - Settings::pref->min_step);
}

void TPlayer::incHue() {
    setHue(mset.hue + Settings::pref->min_step);
}

void TPlayer::decHue() {
    setHue(mset.hue - Settings::pref->min_step);
}

void TPlayer::incSaturation() {
    setSaturation(mset.saturation + Settings::pref->min_step);
}

void TPlayer::decSaturation() {
    setSaturation(mset.saturation - Settings::pref->min_step);
}

void TPlayer::setColorSpace(int colorSpace) {
    WZDEBUG(QString::number(colorSpace));

    if (colorSpace < Settings::TMediaSettings::TColorSpace::COLORSPACE_AUTO
        || colorSpace
        > Settings::TMediaSettings::TColorSpace::COLORSPACE_MAX) {
        colorSpace = Settings::TMediaSettings::COLORSPACE_AUTO;
    }

    if (Settings::pref->isMPV() && colorSpace != mset.color_space) {

        // setVideoFilter can generate change of VO.
        // Inform TMainWindow::getNewSizeFactor() to keep the current size
        keepSize = true;

        // Remove old filter
        if (mset.color_space
            != Settings::TMediaSettings::TColorSpace::COLORSPACE_AUTO) {
            proc->setVideoFilter("format", false, "colormatrix="
                                 + mset.getColorSpaceOptionString());
        }

        // Set new filter
        mset.color_space = (Settings::TMediaSettings::TColorSpace) colorSpace;
        if (mset.color_space
            != Settings::TMediaSettings::TColorSpace::COLORSPACE_AUTO) {
            setVideoFilter("format", true,
                           "colormatrix=" + mset.getColorSpaceOptionString());
        }

        // Clear keepSize in case main window resize not arriving
        if (keepSize) {
            keepSizeTimer->start();
        }
    }
}

void TPlayer::setSpeed(double value) {
    WZDEBUG(QString::number(value));

    // Min and max used by players
    if (value < 0.10) value = 0.10;
    if (value > 100) value = 100;

    mset.speed = value;
    proc->setSpeed(value);

    Gui::msgOSD(tr("Speed: %1").arg(value));
}

void TPlayer::incSpeed10() {
    setSpeed((double) mset.speed + 0.1);
}

void TPlayer::decSpeed10() {
    setSpeed((double) mset.speed - 0.1);
}

void TPlayer::incSpeed4() {
    setSpeed((double) mset.speed + 0.04);
}

void TPlayer::decSpeed4() {
    setSpeed((double) mset.speed - 0.04);
}

void TPlayer::incSpeed1() {
    setSpeed((double) mset.speed + 0.01);
}

void TPlayer::decSpeed1() {
    setSpeed((double) mset.speed - 0.01);
}

void TPlayer::doubleSpeed() {
    setSpeed((double) mset.speed * 2);
}

void TPlayer::halveSpeed() {
    setSpeed((double) mset.speed / 2);
}

void TPlayer::normalSpeed() {
    setSpeed(1);
}

int TPlayer::getVolume() const {
    return Settings::pref->global_volume ? Settings::pref->volume : mset.volume;
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

void TPlayer::setVolumeEx(int volume, bool unmute) {

    if (volume < 0) {
        volume = 0;
    } else if (volume > 100) {
        volume = 100;
    }

    bool muted;
    if (Settings::pref->global_volume) {
        Settings::pref->volume = volume;
        muted = Settings::pref->mute;
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

void TPlayer::setVolume(int volume) {
    setVolumeEx(volume, true /* unmute */);
}

bool TPlayer::getMute() const {
    return Settings::pref->global_volume ? Settings::pref->mute : mset.mute;
}

void TPlayer::mute(bool b) {
    logger()->debug("mute: %1", b);

    if (Settings::pref->global_volume) {
        Settings::pref->mute = b;
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
    setVolume(getVolume() + Settings::pref->min_step);
}

void TPlayer::decVolume() {
    setVolume(getVolume() - Settings::pref->min_step);
}

Settings::TAudioEqualizerList TPlayer::getAudioEqualizer() const {
    return Settings::pref->global_audio_equalizer
            ? Settings::pref->audio_equalizer
            : mset.audio_equalizer;
}

void TPlayer::setSubDelay(int delay) {
    WZDEBUG(QString::number(delay));

    mset.sub_delay = delay;
    proc->setSubDelay((double) mset.sub_delay/1000);
    Gui::msgOSD(tr("Subtitle delay: %1 ms").arg(delay));
}

void TPlayer::incSubDelay() {
    setSubDelay(mset.sub_delay + 100);
}

void TPlayer::decSubDelay() {
    setSubDelay(mset.sub_delay - 100);
}

void TPlayer::setAudioDelay(int delay) {
    WZDEBUG(QString::number(delay));

    mset.audio_delay = delay;
    proc->setAudioDelay((double) mset.audio_delay/1000);
    Gui::msgOSD(tr("Audio delay: %1 ms").arg(delay));
}

void TPlayer::incAudioDelay() {
    setAudioDelay(mset.audio_delay + 100);
}

void TPlayer::decAudioDelay() {
    setAudioDelay(mset.audio_delay - 100);
}

void TPlayer::incSubPos() {
    WZDEBUG("");

    mset.sub_pos++;
    if (mset.sub_pos > 100) mset.sub_pos = 100;
    proc->setSubPos(mset.sub_pos);
}

void TPlayer::decSubPos() {
    WZDEBUG("");

    mset.sub_pos--;
    if (mset.sub_pos < 0) mset.sub_pos = 0;
    proc->setSubPos(mset.sub_pos);
}

void TPlayer::setSubScale(double value) {
    WZDEBUG(QString::number(value));

    if (value < 0) value = 0;

    if (Settings::pref->use_ass_subtitles) {
        if (value != mset.sub_scale_ass) {
            mset.sub_scale_ass = value;
            proc->setSubScale(mset.sub_scale_ass);
        }
    } else if (Settings::pref->isMPV()) {
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

    if (Settings::pref->use_ass_subtitles) {
        setSubScale(mset.sub_scale_ass + step);
    } else if (Settings::pref->isMPV()) {
        setSubScale(mset.sub_scale_mpv + step);
    } else {
        setSubScale(mset.sub_scale + step);
    }
}

void TPlayer::decSubScale() {

    double step = 0.20;

    if (Settings::pref->use_ass_subtitles) {
        setSubScale(mset.sub_scale_ass - step);
    } else if (Settings::pref->isMPV()) {
        setSubScale(mset.sub_scale_mpv - step);
    } else {
        setSubScale(mset.sub_scale - step);
    }
}

void TPlayer::setOSDScale(double value) {
    WZDEBUG(QString::number(value));

    if (value < 0) value = 0;

    if (Settings::pref->isMPlayer()) {
        if (value != Settings::pref->subfont_osd_scale) {
            Settings::pref->subfont_osd_scale = value;
            if (proc->isRunning())
                restartPlayer();
        }
    } else {
        if (value != Settings::pref->osd_scale) {
            Settings::pref->osd_scale = value;
            if (proc->isRunning())
                proc->setOSDScale(Settings::pref->osd_scale);
        }
    }
}

void TPlayer::incOSDScale() {

    if (Settings::pref->isMPlayer()) {
        setOSDScale(Settings::pref->subfont_osd_scale + 1);
    } else {
        setOSDScale(Settings::pref->osd_scale + 0.10);
    }
}

void TPlayer::decOSDScale() {

    if (Settings::pref->isMPlayer()) {
        setOSDScale(Settings::pref->subfont_osd_scale - 1);
    } else {
        setOSDScale(Settings::pref->osd_scale - 0.10);
    }
}

void TPlayer::incSubStep() {
    proc->setSubStep(1);
}

void TPlayer::decSubStep() {
    proc->setSubStep(-1);
}

void TPlayer::changeExternalSubFPS(int fps_id) {
    WZDEBUG(QString::number(fps_id));

    mset.external_subtitles_fps = fps_id;
    if (hasExternalSubs()) {
        restartPlayer();
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

void TPlayer::setAudioEqualizerEx(const Settings::TAudioEqualizerList& values,
                                  bool restart) {

    if (Settings::pref->global_audio_equalizer) {
        Settings::pref->audio_equalizer = values;
    } else {
        mset.audio_equalizer = values;
    }

    if (restart) {
        restartPlayer();
    } else {
        proc->setAudioEqualizer(equalizerListToString(values));
    }
}

void TPlayer::setAudioEqualizer(const Settings::TAudioEqualizerList& values) {
    setAudioEqualizerEx(values, false);
}

void TPlayer::setAudioAudioEqualizerRestart(
        const Settings::TAudioEqualizerList& values) {
    setAudioEqualizerEx(values, true);
}

void TPlayer::setAudioEq(int eq, int value) {
    WZDEBUG("eq " + QString::number(eq) + " value " + QString::number(value));

    if (Settings::pref->global_audio_equalizer) {
        Settings::pref->audio_equalizer[eq] = value;
        setAudioEqualizer(Settings::pref->audio_equalizer);
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
                WZDEBUG("position " + QString::number(mset.current_sec)
                        + " reached out point "
                        + QString::number(mset.out_point)
                        + ", start seeking in point "
                        + QString::number(mset.in_point));
                seeking = true;
                seekTime(mset.in_point);
            }
        } else {
            WZDEBUG("position " + QString::number(mset.current_sec)
                    + " reached out point " + QString::number(mset.out_point)
                    + ", sending quit");

            proc->quit(Player::Process::TExitMsg::EXIT_OUT_POINT_REACHED);
        }
    } else if (seeking) {
        WZDEBUG("done handling out point, position "
                + QString::number(mset.current_sec)
                + " no longer after out point "
                + QString::number(mset.out_point));
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
        WZDEBUG("emit chapterChanged(" + QString::number(new_chapter_id) + ")");
        emit chapterChanged(new_chapter_id);
    }
}

// TMPVProcess sends it only once after initial start
void TPlayer::onReceivedPause() {
    WZDEBUG("at " + QString::number(mset.current_sec));
    setState(STATE_PAUSED);
}

void TPlayer::setDeinterlace(int ID) {
    WZDEBUG(QString::number(ID));

    if (ID != mset.current_deinterlacer) {
        if (Settings::pref->isMPlayer()) {
            mset.current_deinterlacer = ID;
            restartPlayer();
        } else {
            // MPV: remove previous filter
            switch (mset.current_deinterlacer) {
                case Settings::TMediaSettings::L5:
                    proc->setVideoFilter("l5", false);
                    break;
                case Settings::TMediaSettings::Yadif:
                    proc->setVideoFilter("yadif", false);
                    break;
                case Settings::TMediaSettings::LB:
                    proc->setVideoFilter("lb", false);
                    break;
                case Settings::TMediaSettings::Yadif_1:
                    proc->setVideoFilter("yadif", false, "1");
                    break;
                case Settings::TMediaSettings::Kerndeint:
                    proc->setVideoFilter("kerndeint", false, "5");
                    break;
            }
            mset.current_deinterlacer = ID;
            // Add new filter
            switch (mset.current_deinterlacer) {
                case Settings::TMediaSettings::L5:
                    proc->setVideoFilter("l5", true);
                    break;
                case Settings::TMediaSettings::Yadif:
                    proc->setVideoFilter("yadif", true);
                    break;
                case Settings::TMediaSettings::LB:
                    proc->setVideoFilter("lb", true);
                    break;
                case Settings::TMediaSettings::Yadif_1:
                    proc->setVideoFilter("yadif", true, "1");
                    break;
                case Settings::TMediaSettings::Kerndeint:
                    proc->setVideoFilter("kerndeint", true, "5");
                    break;
            }
        }
    }
}

void TPlayer::setVideoTrack(int id) {
    WZDEBUG(QString::number(id));

    // TODO: fix video tracks having different dimensions. The video out
    // dimension will be the dimension of the last selected video track.
    // When fixed use proc->setVideo() instead of restartPlay().
    if (id != mdat.videos.getSelectedID()) {
        mset.current_video_id = id;
        restartPlayer();
    }
}

void TPlayer::nextVideoTrack() {
    WZDEBUG("");

    setVideoTrack(mdat.videos.nextID(mdat.videos.getSelectedID()));
}

void TPlayer::setAudioTrack(int id) {
    WZDEBUG(QString::number(id));

    mset.current_audio_id = id;
    if (id >= 0) {
        proc->setAudio(id);
        mdat.audios.setSelectedID(id);
        emit audioTrackChanged(id);

        // Workaround for a mplayer problem in windows,
        // volume is too loud after changing audio.
        // Workaround too for a mplayer problem in linux,
        // the volume is reduced if using -softvol-max.
        if (Settings::pref->isMPlayer()
            && !Settings::pref->player_additional_options.contains("-volume")) {
            setVolumeEx(getVolume(), false /* unmute */);
        }
    }
}

void TPlayer::nextAudioTrack() {
    WZDEBUG("");

    setAudioTrack(mdat.audios.nextID(mdat.audios.getSelectedID()));
}

// Note: setSubtitle is by index, not ID
void TPlayer::setSubtitle(int idx) {
    WZDEBUG("idx " + QString::number(idx));

    mset.current_sub_set_by_user = true;
    if (idx >= 0 && idx < mdat.subs.count()) {
        mset.current_sub_idx = idx;
        SubData sub = mdat.subs.itemAt(idx);
        if (sub.ID() != mdat.subs.selectedID()
            || sub.type() != mdat.subs.selectedType()) {
            proc->setSubtitle(sub.type(), sub.ID());
        }
    } else {
        mset.current_sub_idx = Settings::TMediaSettings::SubNone;
        if (mdat.subs.selectedID() >= 0) {
            proc->disableSubtitles();
        }
    }
}

void TPlayer::nextSubtitle() {
    setSubtitle(mdat.subs.nextID());
}

void TPlayer::setSecondarySubtitle(int idx) {
    WZDEBUG("idx " + QString::number(idx));

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
        mset.current_secondary_sub_idx = Settings::TMediaSettings::SubNone;
        if (mdat.subs.selectedSecondaryID() >= 0) {
            proc->disableSecondarySubtitles();
        }
    }

    emit secondarySubtitleTrackChanged(mset.current_secondary_sub_idx);
}

void TPlayer::setTitle(int title) {
    WZDEBUG(QString::number(title));

    if (proc->isRunning()) {
        // Handle CDs with the chapter commands
        if (TMediaData::isCD(mdat.detected_type)) {
            setChapter(title - mdat.titles.firstID()
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

void TPlayer::setChapter(int id) {
    WZDEBUG(QString::number(id));

    if (id >= mdat.chapters.firstID()) {
        proc->setChapter(id);
    }
}

void TPlayer::prevChapter() {
    proc->nextChapter(-1);
}

void TPlayer::nextChapter() {
    proc->nextChapter(1);
}

void TPlayer::setAngle(int angle) {
    WZDEBUG(QString::number(angle));

    mset.current_angle = angle;
    proc->setAngle(angle);
}

void TPlayer::nextAngle() {
    WZDEBUG("");

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

void TPlayer::clearKeepSize() {
    WZDEBUG("");

    keepSizeTimer->stop();
    keepSize = false;
}

void TPlayer::setAspectRatio(int id) {
    WZDEBUG(QString::number(id));

    // Keep id in range
    Settings::TAspectRatio::TMenuID aspect_id;
    if (id < 0 || id > Settings::TAspectRatio::MAX_MENU_ID)
        aspect_id = Settings::TAspectRatio::AspectAuto;
    else
        aspect_id = (Settings::TAspectRatio::TMenuID) id;

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
        WZERROR(mset.aspect_ratio.toString() + ". No video player.");
    }
}

void TPlayer::nextAspectRatio() {
    setAspectRatio(mset.aspect_ratio.nextMenuID());
}

void TPlayer::nextWheelFunction() {

    int a = Settings::pref->wheel_function;

    bool done = false;
    if (((int) Settings::pref->wheel_function_cycle) == 0) {
        return;
    }
    while(!done){
        // get next a
        a = a * 2;
        if (a == 32) {
            a = 2;
        }
        // See if we are done
        if (Settings::pref->wheel_function_cycle.testFlag(
                (Settings::TPreferences::TWheelFunction)a)) {
            done = true;
        }
    }
    Settings::pref->wheel_function = a;
    QString m = "";
    switch(a){
    case Settings::TPreferences::Seeking:
        m = tr("Mouse wheel seeks now");
        break;
    case Settings::TPreferences::Volume:
        m = tr("Mouse wheel changes volume now");
        break;
    case Settings::TPreferences::Zoom:
        m = tr("Mouse wheel changes zoom level now");
        break;
    case Settings::TPreferences::ChangeSpeed:
        m = tr("Mouse wheel changes speed now");
        break;
    }
    Gui::msgOSD(m);
}

void TPlayer::setetterbox(bool b) {
    WZDEBUG(QString::number(b));

    if (mset.add_letterbox != b) {
        mset.add_letterbox = b;
        setVideoFilter("letterbox", b, TDesktop::aspectRatio(playerwindow));
    }
}

void TPlayer::setetterboxOnFullscreen(bool b) {
    WZDEBUG(QString::number(b));
    setVideoFilter("letterbox", b, TDesktop::aspectRatio(playerwindow));
}

void TPlayer::setOSDLevel(int level) {
    WZDEBUG(QString::number(level));

    Settings::pref->osd_level = (Settings::TPreferences::TOSDLevel) level;
    if (proc->isRunning())
        proc->setOSDLevel(level);
    emit osdLevelChanged(level);
}

void TPlayer::nextOSDLevel() {

    int level;
    if (Settings::pref->osd_level >= Settings::TPreferences::SeekTimerTotal) {
        level = Settings::TPreferences::None;
    } else {
        level = Settings::pref->osd_level + 1;
    }
    setOSDLevel(level);
}

void TPlayer::setRotate(int r) {
    WZDEBUG(QString::number(r));

    if (mset.rotate != r) {
        if (Settings::pref->isMPlayer()) {
            mset.rotate = r;
            restartPlayer();
        } else {
            // MPV
            // Remove previous filter
            if (mset.rotate) {
                proc->setVideoFilter("rotate", false, mset.rotate);
            }
            // Set new filter
            mset.rotate = r;
            if (mset.rotate) {
                proc->setVideoFilter("rotate", true, mset.rotate);
            }
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
    WZDEBUG(QString::number(zoom));

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
    WZDEBUG("");

    // Reset zoom and pan of video window
    playerwindow->resetZoomAndPan();
    // Reread modified settings
    getZoomFromPlayerWindow();
    getPanFromPlayerWindow();
    Gui::msgOSD(tr("Zoom and pan reset"));
}

void TPlayer::pan(int dx, int dy) {
    WZDEBUG("dx " + QString::number(dx) + ", dy " + QString::number(dy));

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

void TPlayer::setUseCustomSubStyle(bool b) {
    WZDEBUG(QString::number(b));

    if (Settings::pref->use_custom_ass_style != b) {
        Settings::pref->use_custom_ass_style = b;
        if (proc->isRunning())
            restartPlayer();
    }
}

void TPlayer::toggleForcedSubsOnly(bool b) {
    WZDEBUG(QString::number(b));

    Settings::pref->use_forced_subs_only = b;
    if (proc->isRunning())
        proc->setSubForcedOnly(b);
}

void TPlayer::setClosedCaptionChannel(int c) {
    WZDEBUG(QString::number(c));

    if (c != mset.closed_caption_channel) {
        mset.closed_caption_channel = c;
        if (proc->isRunning())
            restartPlayer();
    }
}

// dvdnav buttons
void TPlayer::dvdnavUp() {
    if (proc->isReady())
        proc->discButtonPressed("up");
}

void TPlayer::dvdnavDown() {
    if (proc->isReady())
        proc->discButtonPressed("down");
}

void TPlayer::dvdnavLeft() {
    if (proc->isReady())
        proc->discButtonPressed("left");
}

void TPlayer::dvdnavRight() {
    if (proc->isReady())
        proc->discButtonPressed("right");
}

void TPlayer::dvdnavMenu() {
    if (proc->isReady())
        proc->discButtonPressed("menu");
}

void TPlayer::dvdnavPrev() {
    if (proc->isReady())
        proc->discButtonPressed("prev");
}

// Slot called by action dvdnav_select
void TPlayer::dvdnavSelect() {
    WZDEBUG("");

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
    WZDEBUG("");

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
    WZDEBUG("'" + filename + "'");

    QFileInfo fi(filename);
    QString text = tr("Screenshot saved as %1").arg(fi.fileName());
    Gui::msgOSD(text);
}

void TPlayer::displayUpdatingFontCache() {
    WZDEBUG("");
    Gui::msg(tr("Updating the font cache. This may take some seconds..."));
}

void TPlayer::displayBuffering() {
    Gui::msg(tr("Buffering..."));
}

void TPlayer::displayBufferingEnded() {
    Gui::msg(tr("Playing from %1")
        .arg(TWZTime::formatTime(qRound(mset.current_sec))));
}

void TPlayer::onReceivedVideoOut() {
    WZDEBUG("");

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
    if (mset.aspect_ratio.ID() == Settings::TAspectRatio::AspectAuto) {
        mdat.video_aspect_original = playerwindow->aspectRatio();
    }
}

bool TPlayer::setPreferredAudio() {

    if (!Settings::pref->audio_lang.isEmpty()) {
        int selected_id = mdat.audios.getSelectedID();
        if (selected_id >= 0) {
            int wanted_id = mdat.audios.findLangID(Settings::pref->audio_lang);
            if (wanted_id >= 0 && wanted_id != selected_id) {
                WZDEBUG("selecting preferred audio id "
                        + QString::number(wanted_id));
                setAudioTrack(wanted_id);
                return true;
            }
        }
    }

    WZDEBUG("no player overrides");
    return false;
}

void TPlayer::onAudioTracksChanged() {
    WZDEBUG("");

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
        if (!Settings::pref->language.isEmpty()) {
            wanted_idx = mdat.subs.findLangIdx(Settings::pref->language);
        }
        // Keep subtitles selected by the player
        if (wanted_idx < 0 && mset.current_sub_idx >= 0) {
            wanted_idx = mset.current_sub_idx;
        }
        // Select first subtitles
        if (wanted_idx < 0 && Settings::pref->select_first_subtitle) {
            wanted_idx = 0;
        }
    }

    if (wanted_idx < 0) {
        WZDEBUG("no player overrides");
    } else if (wanted_idx == mset.current_sub_idx) {
        WZDEBUG("keeping selected subtitles");
    } else {
        WZDEBUG("selecting preferred subtitles");
        setSubtitle(wanted_idx);
        mset.current_sub_set_by_user = false;
    }
}

void TPlayer::onSubtitlesChanged() {
    WZDEBUG("");

    // Need to set current_sub_idx, the subtitle action group checks on it.
    mset.current_sub_idx = mdat.subs.findSelectedIdx();
    mset.current_secondary_sub_idx = mdat.subs.findSelectedSecondaryIdx();
    emit subtitlesChanged();

    if (Settings::pref->isMPlayer()) {
        // MPlayer selected sub will not yet be updated, que the subtitle
        // selection
        WZDEBUG("posting selectPreferredSubtitles()");
        QTimer::singleShot(1500, this, SLOT(selectPreferredSubtitles()));
    } else {
        selectPreferredSubtitles();
    }
}

// Called when player changed subtitle track
void TPlayer::onSubtitleChanged() {
    WZDEBUG("");

    // Need to set current_sub_idx, the subtitle group checks on it.
    int selected_idx = mdat.subs.findSelectedIdx();
    mset.current_sub_idx = selected_idx;

    mset.current_secondary_sub_idx = mdat.subs.findSelectedSecondaryIdx();

    emit subtitleTrackChanged(selected_idx);
}

} // namespace Player

#include "moc_player.cpp"
