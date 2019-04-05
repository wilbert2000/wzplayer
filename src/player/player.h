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

#ifndef PLAYER_PLAYER_H
#define PLAYER_PLAYER_H

#include <QProcess>
#include <QTime>

#include "wzdebug.h"
#include "config.h"
#include "mediadata.h"
#include "player/state.h"
#include "settings/mediasettings.h"


class QTimer;

namespace Gui {
class TPlayerWindow;
}

namespace Player {

namespace Process {
    class TPlayerProcess;
}


class TPlayer : public QObject {
    Q_OBJECT
    DECLARE_QCLASS_LOGGER

public:
    TPlayer(QWidget* parent, Gui::TPlayerWindow *pw);
    virtual ~TPlayer() override;

    TMediaData mdat;
    Settings::TMediaSettings mset;
    bool keepSize;

    //! Return the current state
    TState state() const { return _state; }
    //! Set the current state and send the stateChanged() signal if it changed
    void setState(TState s);
    //! Return a string with the name of the current state.
    static QString stateToString(TState state);
    QString stateToString() const;

    bool statePOP() const {
        return _state == STATE_PLAYING || _state == STATE_PAUSED;
    }
    bool stateReady() const {
        return _state == STATE_STOPPED
            || _state == STATE_PLAYING
            || _state == STATE_PAUSED;
    }

    bool hasVideo() const {
        return mdat.hasVideo();
    }
    bool hasAudio() const {
        return mdat.hasAudio();
    }
    bool hasExternalSubs() const;

    int getVolume() const;
    bool getMute() const;
    Settings::TAudioEqualizerList getAudioEqualizer() const;

    bool videoFiltersEnabled(bool displayMessage = false);

    //! Generic open, with autodetection of type
    void open(QString filename = "", bool loopImage = false);
    //! Open disc
    void openDisc(TDiscName disc, bool fast_open = false);

    // Stop player if running and save MediaInfo
    void close();

    void restart(); // Restart current file
    void reload(); // Reopen current file

    void addForcedTitle(const QString& file, const QString& title) {
        forced_titles[file] = title;
    }

    // Force the use of the specified subtitle file.
    // Used when the next video starts playing.
    void setInitialSubtitle(const QString& subtitle_file) {
        initial_subtitle = subtitle_file;
    }

    void setStartPausedOnce() { startPausedOnce = true; }
    void saveRestartState();


public slots:
    // Play
    void play();
    void playOrPause();
    void pause();
    void stop();

    void frameStep();
    void frameBackStep();
    void forward1();    // + 10 seconds
    void rewind1();     // - 10 seconds
    void forward2();     // + 1 minute
    void rewind2();      // -1 minute
    void forward3(); // + 10 minutes
    void rewind3();  // - 10 minutes

    void forward(int secs);
    void rewind(int secs);

    void seekRelative(double secs);
    void seekPercentage(double perc);
    void seekTime(double sec);

    void setSpeed(double value);
    void incSpeed10(); //!< Inc speed 10%
    void decSpeed10(); //!< Dec speed 10%
    void incSpeed4();  //!< Inc speed 4%
    void decSpeed4();  //!< Dec speed 4%
    void incSpeed1();  //!< Inc speed 1%
    void decSpeed1();  //!< Dec speed 1%
    void doubleSpeed();
    void halveSpeed();
    void normalSpeed();

    void setInPoint(); //!< Set in point to current sec
    void seekInPoint();
    void clearInPoint();
    void setOutPoint(); //!< Set out point to current sec
    void seekOutPoint();
    void clearOutPoint();
    void setRepeat(bool b);
    void clearInOutPoints();


    // Video
    void setAspectRatio(int id);
    void nextAspectRatio();
    void setZoom(double zoom); // Zoom on playerwindow
    void incZoom();
    void decZoom();

    void panLeft();
    void panRight();
    void panUp();
    void panDown();

    void resetZoomAndPan();

    void setBrightness(int value);
    void incBrightness();
    void decBrightness();
    void setContrast(int value);
    void incContrast();
    void decContrast();
    void setGamma(int value);
    void incGamma();
    void decGamma();
    void setHue(int value);
    void incHue();
    void decHue();
    void setSaturation(int value);
    void incSaturation();
    void decSaturation();

    void setColorSpace(int colorSpace);

    void setDeinterlace(int);
    void toggleDeinterlace();

    // Video transform
    void setFlip(bool b);
    void setMirror(bool b);
    void setRotate(int r);

    // Video filters
    void setPostprocessing(bool b);
    void setDeblock(bool b);
    void setDering(bool b);
    void setGradfun(bool b);
    void setNoise(bool b);
    void setAutophase(bool b);
    void setDenoiser(int);
    void setetterbox(bool);
    void setetterboxOnFullscreen(bool);
    void setSoftwareScaling(bool);
    void setSharpen(int);
    void setStereo3D(const QString& in, const QString& out);

    void setVideoTrack(int id);
    void nextVideoTrack();

    // Screenshot
    void screenshot();    //!< Take a screenshot of current frame
    void screenshots();    //!< Start/stop taking screenshot of each frame
    void switchCapturing();


    // Audio
    void setVolume(int volume);
    void mute(bool b);
    void incVolume();
    void decVolume();

    void setAudioDelay(int delay);
    void incAudioDelay();
    void decAudioDelay();

    void setAudioEqualizer(const Settings::TAudioEqualizerList& values);
    void setAudioAudioEqualizerRestart(const Settings::TAudioEqualizerList&);
    void setAudioEq(int eq, int value);
    void setAudioEq0(int value);
    void setAudioEq1(int value);
    void setAudioEq2(int value);
    void setAudioEq3(int value);
    void setAudioEq4(int value);
    void setAudioEq5(int value);
    void setAudioEq6(int value);
    void setAudioEq7(int value);
    void setAudioEq8(int value);
    void setAudioEq9(int value);

    void setStereoMode(int mode);
    void setAudioChannels(int channels);

    // Audio filters
    void setVolnorm(bool b);
    void setExtrastereo(bool b);
    void toggleKaraoke(bool b);

    void setAudioTrack(int id);
    void nextAudioTrack();

    void loadAudioFile(const QString& audiofile);
    void unloadAudioFile();


    // Subtitles
    void incSubPos();
    void decSubPos();
    void setSubScale(double value);
    void incSubScale();
    void decSubScale();
    void setSubDelay(int delay);
    void incSubDelay();
    void decSubDelay();

    void incSubStep(); // Select next line in subtitle file
    void decSubStep(); // Select previous line in subtitle file
    void seekToNextSub();
    void seekToPrevSub();

    void setSubtitle(int idx);
    void nextSubtitle();
    void setSecondarySubtitle(int idx);
    void setClosedCaptionChannel(int);
    /*
    void nextClosedCaptionChannel();
    void prevClosedCaptionChannel();
    */
    void toggleForcedSubsOnly(bool);

    void loadSub(const QString& sub);
    void unloadSub();
    void changeExternalSubFPS(int fps_id);

    void setUseCustomSubStyle(bool);

    // Browse
    void setTitle(int title);
    void setChapter(int id);
    void prevChapter();
    void nextChapter();
    void setAngle(int);
    void nextAngle();

    // dvdnav buttons
    void dvdnavUp();
    void dvdnavDown();
    void dvdnavLeft();
    void dvdnavRight();
    void dvdnavMenu();
    void dvdnavSelect();
    void dvdnavPrev();
    void dvdnavMouse();
    void dvdnavMousePos(const QPoint& pos);


    // Window
    void setOSDLevel(int level);
    void nextOSDLevel();
    void setOSDScale(double value);
    void incOSDScale();
    void decOSDScale();
    void showFilenameOnOSD();
    void showTimeOnOSD();

    void clearOSD();
    void displayTextOnOSD(const QString& text,
                          int duration = TConfig::MESSAGE_DURATION,
                          int level = 1);

    void wheelUpFunc(Settings::TPreferences::TWheelFunction function);
    void wheelUpSeeking();
    void wheelUp();
    void wheelDownFunc(Settings::TPreferences::TWheelFunction function);
    void wheelDownSeeking();
    void wheelDown();
    void nextWheelFunction();

    void clearKeepSize();

signals:
    void stateChanged(Player::TState state);
    void mediaSettingsChanged();
    void videoOutResolutionChanged(int w, int h);
    void newMediaStartedPlaying();
    void mediaInfoChanged();
    void streamingTitleChanged();
    void mediaEOF(); // Media has arrived to the end.
    void playerError(int exitCode);
    //! Sent when requested to play, but there is no file to play
    void noFileToPlay();

    void positionChanged(double sec);
    void durationChanged(double);

    void aspectRatioChanged(int);

    void volumeChanged(int);
    void muteChanged(bool);

    void videoTracksChanged();
    void videoTrackChanged(int);
    void audioTracksChanged();
    void audioTrackChanged(int);
    void subtitlesChanged();
    void subtitleTrackChanged(int);
    void secondarySubtitleTrackChanged(int);
    void titleTracksChanged();
    void titleTrackChanged(int);
    void chaptersChanged();
    void chapterChanged(int);
    void anglesChanged();
    void InOutPointsChanged();
    void osdLevelChanged(int);
    void videoEqualizerNeedsUpdate();

    void videoBitRateChanged(int bitrate);
    void audioBitRateChanged(int bitrate);

private:
    static double restartTime;
    static bool startPausedOnce;

    Player::Process::TPlayerProcess* proc;
    Gui::TPlayerWindow* playerwindow;

    TState _state;
    bool seeking;
    QTime time;
    QTimer* keepSizeTimer;

    QString initial_subtitle;
    QMap<QString,QString> forced_titles;

    int cache_size;

    static QString equalizerListToString(const Settings::TAudioEqualizerList&
                                         values);

    void openFile(const QString& filename, bool loopImage);
    void openStream(const QString& name);
    void openTV(QString channel_id);

    void startPlayer(bool loopImage = false);
    void stopPlayer();
    void restartPlayer(TState state = STATE_RESTARTING);
    void setInPointSec(double sec);
    void setOutPointSec(double sec);

    void saveMediaSettings();
    void initVolume();
    void initMediaSettings();
    void onPlayingStartedNewMedia();

    bool haveVideoFilters() const;
    void setVideoFilter(const QString& filter, bool enable,
                        const QVariant& option);

    void setVolumeEx(int volume, bool unmute);
    void setAudioEqualizerEx(const Settings::TAudioEqualizerList& values,
                             bool restart);

    void setExternalSubs(const QString& filename);
    bool setPreferredAudio();

    void getZoomFromPlayerWindow();
    void getPanFromPlayerWindow();
    void pan(int dx, int dy);

    void seekCmd(double secs, int mode);
    void handleOutPoint();
    void updateLoop();

private slots:
    void onPlayingStarted();

    void onProcessError(QProcess::ProcessError error);
    void onProcessFinished(bool normal_exit, int exit_code, bool eof);

    void onReceivedMessage(const QString& s);
    void onReceivedPosition(double sec);
    void onReceivedPause();
    void onReceivedVideoOut();
    void onAudioTracksChanged();

    void onSubtitlesChanged();
    void onSubtitleChanged();
    void selectPreferredSubtitles();

    void displayScreenshotName(const QString& filename);
    void displayUpdatingFontCache();
    void displayBuffering();
    void displayBufferingEnded();
};

} // namespace Player

extern Player::TPlayer* player;

#endif // PLAYER_PLAYER_H
