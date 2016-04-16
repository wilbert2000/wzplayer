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

public:
	TCore(QWidget* parent, TPlayerWindow *mpw);
	virtual ~TCore();

	TMediaData mdat;
	Settings::TMediaSettings mset;

	//! Return the current state
	TCoreState state() const { return _state; }

	//! Return a string with the name of the current state,
	//! so it can be printed on debugging messages.
	QString stateToString() const;

	//! Generic open, with autodetection of type
	void open(QString file, int seek = -1);
	//! Open disc
	void openDisc(TDiscName disc, bool fast_open = false);

	// Stop player if running and save MediaInfo
	void close();

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

	void setAMarker(); //!< Set A marker to current sec
	void setAMarker(int sec);

	void setBMarker(); //!< Set B marker to current sec
	void setBMarker(int sec);
	void clearABMarkers();
	void toggleRepeat(bool b);

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
	void autoZoom();
	void autoZoomFromLetterbox(double video_aspect);
	void autoZoomFor169();
	void autoZoomFor235();

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

	//! Change fullscreen when using the player own window
	void changeFullscreenMode(bool b);

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
	void mediaLoaded();
	void mediaInfoChanged();
	void mediaStopped();
	void mediaEOF(); // Media has arrived to the end.
	//! Player started but finished with exit code != 0
	void playerFinishedWithError(int exitCode);
	//! Player didn't started or crashed
	void playerError(QProcess::ProcessError error);
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
	void ABMarkersChanged();
	void osdLevelChanged(int);
	void videoEqualizerNeedsUpdate();

	void needResize(int w, int h);

protected:
	//! Change the current state (Stopped, Playing or Paused)
	//! And sends the stateChanged() signal.
	void setState(TCoreState s);

	void initVolume();
	void initMediaSettings();
	void initPlaying(int seek = -1);
	void startPlayer(QString file, double seek = -1);
	void stopPlayer();
	void restartPlay();

	void saveMediaSettings();

protected slots:
	void playingStarted();
	void processError(QProcess::ProcessError error);
	void processFinished(bool normal_exit);
	void onReceivedEndOfFile();

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

	int restarting;

	QTime time;

	QString initial_subtitle;
	QMap<QString,QString> forced_titles;

	int cache_size;

	static QString equalizerListToString(const Settings::TAudioEqualizerList& values);

	bool isMPlayer() const;
	bool isMPV() const;

	void openFile(const QString& filename, int seek = -1);
	void openStream(const QString& name);
	void openTV(QString channel_id);

	void playingNewMediaStarted();

	bool haveVideoFilters() const;
	void changeVF(const QString& filter, bool enable, const QVariant& option);

	void setExternalSubs(const QString& filename);
	bool setPreferredAudio();

	void getZoomFromPlayerWindow();
	void getPanFromPlayerWindow();
	void pan(int dx, int dy);

	int getVolumeForPlayer() const;

	void seekCmd(double secs, int mode);

    void enableScreensaver();
    void disableScreensaver();
};

#endif
