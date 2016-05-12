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

#ifndef CORE_H
#define CORE_H

#include <QProcess>
#include <QTime>

#include "wzdebug.h"
#include "config.h"
#include "corestate.h"
#include "mediadata.h"
#include "settings/mediasettings.h"


class TMediaData;
class TPlayerWindow;

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef DISABLE_SCREENSAVER
class WinScreenSaver;
#endif
#endif

namespace Proc {
class TPlayerProcess;
}


class TCore : public QObject {
    Q_OBJECT
    DECLARE_QCLASS_LOGGER

public:
	TCore(QWidget* parent, TPlayerWindow *mpw);
	virtual ~TCore();

	TMediaData mdat;
	Settings::TMediaSettings mset;

	//! Return the current state
	TCoreState state() const { return _state; }
    //! Change the current state and sends the stateChanged() signal.
    void setState(TCoreState s);
    //! Return a string with the name of the current state.
    static QString stateToString(TCoreState state);
    QString stateToString() const;

    bool statePOP() const {
        return _state == STATE_PLAYING || _state == STATE_PAUSED;
    }
    bool stateReady() const {
        return _state == STATE_STOPPED || _state == STATE_PLAYING || _state == STATE_PAUSED;
    }
    bool hasVideo() const {
        return mdat.hasVideo();
    }
    bool hasAudio() const {
        return mdat.hasAudio();
    }

    //! Generic open, with autodetection of type
    void open(QString file = "", int seek = -1);
	//! Open disc
	void openDisc(TDiscName disc, bool fast_open = false);

	// Stop player if running and save MediaInfo
    void close(TCoreState next_state);

	void addForcedTitle(const QString& file, const QString& title) {
		forced_titles[file] = title;
	}
	bool haveExternalSubs() const;
	int getVolume() const;
	bool getMute() const;
	Settings::TAudioEqualizerList getAudioEqualizer() const;
	bool videoFiltersEnabled(bool displayMessage = false);

public slots:
	void loadSub(const QString& sub);
	void unloadSub();

	//! Forces to use the specified subtitle file. It's not loaded immediately but stored
	//! and will be used for the next video. After that the variable is cleared.  
	void setInitialSubtitle(const QString& subtitle_file) { initial_subtitle = subtitle_file; }

	void loadAudioFile(const QString& audiofile);
	void unloadAudioFile();

	void stop();
    void play();
	void playOrPause();
	void pause();

	void frameStep();
	void frameBackStep();
	void screenshot();	//!< Take a screenshot of current frame
	void screenshots();	//!< Start/stop taking screenshot of each frame
	void switchCapturing();

	void displayMessage(const QString& text, int duration = TConfig::MESSAGE_DURATION, int osd_level = 1);

	//! Public restart, for the GUI.
	void restart();

	//! Reopens the file (no restart)
	void reload();

    void setInPoint(); //!< Set in point to current sec
    void setInPoint(double sec);
    void seekInPoint();
    void clearInPoint();
    void setOutPoint(); //!< Set out point to current sec
    void setOutPoint(double sec);
    void seekOutPoint();
    void clearOutPoint();
    void toggleRepeat(bool b);
    void clearInOutPoints();

	void toggleFlip();
	void toggleFlip(bool b);

	void toggleMirror();
	void toggleMirror(bool b);

	// Audio filters
	void toggleKaraoke();
	void toggleKaraoke(bool b);
	void toggleExtrastereo();
	void toggleExtrastereo(bool b);
	void toggleVolnorm();
	void toggleVolnorm(bool b);

	void setAudioChannels(int channels);
	void setStereoMode(int mode);

	// Video filters
	void toggleAutophase();
	void toggleAutophase(bool b);
	void toggleDeblock();
	void toggleDeblock(bool b);
	void toggleDering();
	void toggleDering(bool b);
	void toggleGradfun();
	void toggleGradfun(bool b);
	void toggleNoise();
	void toggleNoise(bool b);
	void togglePostprocessing();
	void togglePostprocessing(bool b);
	void changeDenoise(int);
	void changeUnsharp(int);
	void changeLetterbox(bool);
	void changeLetterboxOnFullscreen(bool);
	void changeUpscale(bool);
	void changeStereo3d(const QString& in, const QString& out);

    void seekRelative(double secs);
    void seekPercentage(double perc);
    void seekTime(double sec);
	void sforward(); 	// + 10 seconds
	void srewind(); 	// - 10 seconds
    void forward(); 	// + 1 minute
    void rewind(); 		// -1 minute
    void fastforward();	// + 10 minutes
    void fastrewind();	// - 10 minutes
	void forward(int secs);
	void rewind(int secs);
	void seekToNextSub();
	void seekToPrevSub();
	void wheelUp(Settings::TPreferences::TWheelFunction function = Settings::TPreferences::DoNothing);
	void wheelDown(Settings::TPreferences::TWheelFunction function = Settings::TPreferences::DoNothing);

	void setSpeed(double value);
	void incSpeed10();	//!< Inc speed 10%
	void decSpeed10();	//!< Dec speed 10%
	void incSpeed4();	//!< Inc speed 4%
	void decSpeed4();	//!< Dec speed 4%
	void incSpeed1();	//!< Inc speed 1%
	void decSpeed1();	//!< Dec speed 1%
	void doubleSpeed();
	void halveSpeed();
	void normalSpeed();

	void setVolume(int volume, bool unmute = true);
	void mute(bool b);
	void incVolume();
	void decVolume();

	void setBrightness(int value);
	void setContrast(int value);
	void setGamma(int value);
	void setHue(int value);
	void setSaturation(int value);

	void incBrightness();
	void decBrightness();
	void incContrast();
	void decContrast();
	void incGamma();
	void decGamma();
	void incHue();
	void decHue();
	void incSaturation();
	void decSaturation();

	void setSubDelay(int delay);
	void incSubDelay();
	void decSubDelay();

	void setAudioDelay(int delay);
	void incAudioDelay();
	void decAudioDelay();

	void incSubPos();
	void decSubPos();

	void changeSubScale(double value);
	void incSubScale();
	void decSubScale();

	void changeOSDScale(double value);
	void incOSDScale();
	void decOSDScale();

	//! Select next line in subtitle file
	void incSubStep();
	//! Select previous line in subtitle file
	void decSubStep();

	void changeExternalSubFPS(int fps_id);

	//! Audio equalizer
	void setAudioEqualizer(const Settings::TAudioEqualizerList& values, bool restart = false);
	void setAudioAudioEqualizerRestart(const Settings::TAudioEqualizerList& values) {
		setAudioEqualizer(values, true);
	}

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

	void changeDeinterlace(int);

	void changeVideoTrack(int id);
	void nextVideoTrack();
	void changeAudioTrack(int id);
	void nextAudioTrack();
	void changeSubtitle(int idx, bool selected_by_user = true);
	void nextSubtitle();
	void changeSecondarySubtitle(int idx);

#if PROGRAM_SWITCH
	void changeProgram(int ID);
	void nextProgram();
#endif
	void changeTitle(int title);
	void changeChapter(int id);
	void prevChapter();
	void nextChapter();
	void setAngle(int);
	void nextAngle();
	void setAspectRatio(int id);
	void nextAspectRatio();
	void changeOSDLevel(int level);
	void nextOSDLevel();
	void nextWheelFunction();

	void setZoom(double); // Zoom on playerwindow
	void setZoomAndPan(double zoom, double pan_x, double pan_y);

	void changeRotate(int r);

	void incZoom();
	void decZoom();

	void panLeft();
	void panRight();
	void panUp();
	void panDown();

	void resetZoomAndPan();

	void showFilenameOnOSD();
	void showTimeOnOSD();
	void toggleDeinterlace();

	void changeUseCustomSubStyle(bool);
	void toggleForcedSubsOnly(bool);

	void changeClosedCaptionChannel(int);
	/*
	void nextClosedCaptionChannel();
	void prevClosedCaptionChannel();
	*/

	// dvdnav buttons
	void dvdnavUp();
	void dvdnavDown();
	void dvdnavLeft();
	void dvdnavRight();
	void dvdnavMenu();
	void dvdnavSelect();
	void dvdnavPrev();
	void dvdnavMouse();

	//! Wrapper for the osd_show_text slave command
	void displayTextOnOSD(const QString& text,
						  int duration = TConfig::MESSAGE_DURATION,
						  int level = 1);
	void clearOSD();

signals:
	void stateChanged(TCoreState state);
	void mediaSettingsChanged();
	void videoOutResolutionChanged(int w, int h);
    void newMediaStartedPlaying();
	void mediaInfoChanged();
	void mediaStopped();
	void mediaEOF(); // Media has arrived to the end.
    void playerError(int exitCode);
    //! Sent when requested to play, but there is no file to play
	void noFileToPlay();

    void positionChanged(double sec);
    void durationChanged(double);
	void showFrame(int frame);

	void showMessage(const QString& text);
	void showMessage(const QString& text, int time);

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

protected:
	void initVolume();
	void initMediaSettings();
	void initPlaying(int seek = -1);
	void startPlayer(QString file, double seek = -1);
    void stopPlayer();
	void restartPlay();

	void saveMediaSettings();

protected slots:
	void playingStarted();
	void onProcessError(QProcess::ProcessError error);
    void onProcessFinished(bool normal_exit, int exit_code, bool eof);

    void onReceivedPosition(double sec);
	void onReceivedPause();
	void onReceivedVideoOut();

	void displayScreenshotName(const QString& filename);
	void displayUpdatingFontCache();
	void displayBuffering();
	void displayBufferingEnded();

	void onAudioTracksChanged();

	void selectPreferredSubtitles();
	void onSubtitlesChanged();
	void onSubtitleChanged();

	void dvdnavUpdateMousePos(QPoint);

protected:
	Proc::TPlayerProcess* proc;
	TPlayerWindow* playerwindow;

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef DISABLE_SCREENSAVER
	WinScreenSaver* win_screensaver;
#endif
#endif

private:
	TCoreState _state;
    bool seeking;
	QTime time;

	QString initial_subtitle;
	QMap<QString,QString> forced_titles;

	int cache_size;

	static QString equalizerListToString(const Settings::TAudioEqualizerList& values);

	void openFile(const QString& filename, int seek = -1);
	void openStream(const QString& name);
	void openTV(QString channel_id);

    void playingStartedNewMedia();

	bool haveVideoFilters() const;
	void changeVF(const QString& filter, bool enable, const QVariant& option);

	void setExternalSubs(const QString& filename);
	bool setPreferredAudio();

	void getZoomFromPlayerWindow();
	void getPanFromPlayerWindow();
	void pan(int dx, int dy);

    void seekCmd(double secs, int mode);
    void handleOutPoint();
    void updateLoop();

	int getVolumeForPlayer() const;


    void enableScreensaver();
    void disableScreensaver();
};

#endif
