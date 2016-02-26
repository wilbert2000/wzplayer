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

#ifndef CORE_H
#define CORE_H

#include <QProcess>
#include <QTime>

#include "config.h"
#include "mediadata.h"
#include "settings/mediasettings.h"

class TDiscData;
class TMediaData;
class TPlayerWindow;

namespace Proc {
class TPlayerProcess;
}

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef SCREENSAVER_OFF
class WinScreenSaver;
#endif
#endif

#ifdef YOUTUBE_SUPPORT
class RetrieveYoutubeUrl;
#endif


class TCore : public QObject {
	Q_OBJECT

public:
	enum State { Stopped = 0, Playing = 1, Paused = 2 };

	TCore(QWidget* parent, TPlayerWindow *mpw);
	virtual ~TCore();

	TMediaData mdat;
	Settings::TMediaSettings mset;

	//! Return the current state
	State state() { return _state; }

	//! Return a string with the name of the current state,
	//! so it can be printed on debugging messages.
	QString stateToString();

	// Stop player if running and save MediaInfo
	void close();

	void addForcedTitle(const QString& file, const QString& title) {
		forced_titles[file] = title;
	}
	bool haveExternalSubs();
	int positionMax() { return pos_max; }
	int getVolume();
	bool getMute();
	bool videoFiltersEnabled(bool displayMessage = false);

public slots:
	//! Generic open, with autodetection of type
	void open(QString file, int seek = -1, bool fast_open = true);
	void openStream(QString name);
	void openTV(QString channel_id);

#ifdef YOUTUBE_SUPPORT
	void openYT(const QString& url);
#endif

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
#ifdef CAPTURE_STREAM
	void switchCapturing();
#endif

	void displayMessage(const QString& text, int duration = 3500, int osd_level = 1);

	//! Public restart, for the GUI.
	void restart();

	//! Reopens the file (no restart)
	void reload();

	void goToPosition(int pos);
	void goToPos(double perc);
	// void goToPos(int perc);
	void goToSec(double sec);

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
#ifdef MPLAYER_SUPPORT
	void toggleKaraoke();
	void toggleKaraoke(bool b);
	void toggleExtrastereo();
	void toggleExtrastereo(bool b);
#endif
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

	void seek(int secs);
	void sforward(); 	// + 10 seconds
	void srewind(); 	// - 10 seconds
    void forward(); 	// + 1 minute
    void rewind(); 		// -1 minute
    void fastforward();	// + 10 minutes
    void fastrewind();	// - 10 minutes
	void forward(int secs);
	void rewind(int secs);
#ifdef MPV_SUPPORT
	void seekToNextSub();
	void seekToPrevSub();
#endif
	void wheelUp(Settings::TPreferences::WheelFunction function = Settings::TPreferences::DoNothing);
	void wheelDown(Settings::TPreferences::WheelFunction function = Settings::TPreferences::DoNothing);

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

	void setVolume(int volume, bool force = false);
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

	void setOSDPos(const QPoint &pos);
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
#ifdef MPV_SUPPORT
	void changeSecondarySubtitle(int idx);
#endif

#if PROGRAM_SWITCH
	void changeProgram(int ID);
	void nextProgram();
#endif
	void changeTitle(int title);
	void changeChapter(int id);
	void prevChapter();
	void nextChapter();
	void changeAngle(int);
	void nextAngle();
	void changeAspectRatio(int id);
	void nextAspectRatio();
	void changeOSDLevel(int level);
	void nextOSDLevel();
	void nextWheelFunction();

	void changeZoom(double); // Zoom on playerwindow

	void changeRotate(int r);

#if USE_ADAPTER
	void changeAdapter(int n);
#endif

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
						  int duration = 3000,
						  int level = 1);
	void clearOSD();

signals:
	void stateChanged(TCore::State state);
	void mediaSettingsChanged();
	void aboutToStartPlaying(); // Signal emited just before starting player
	void buffering();
	void receivedForbidden();
	void videoOutResolutionChanged(int w, int h);
	void newMediaStartedPlaying();
	void mediaLoaded();
	void mediaInfoChanged();
	//! Sends the filename and title of the stream playing in this moment
	void mediaPlaying(const QString& filename, const QString& title);
	void mediaStopped();
	void mediaEOF(); // Media has arrived to the end.
	//! Player started but finished with exit code != 0
	void playerFinishedWithError(int exitCode);
	//! Player didn't started or crashed
	void playerFailed(QProcess::ProcessError error);
	//! Sent when requested to play, but there is no file to play
	void noFileToPlay();

	void showTime(double sec);
	void positionChanged(int); // To connect a slider
	void durationChanged(double); // Duration has changed
	void showFrame(int frame);

	void showMessage(const QString& text);
	void showMessage(const QString& text, int time);

	void aspectRatioChanged(Settings::TAspectRatio::TMenuID id);
	void zoomChanged(double);

	void volumeChanged(int);
	void muteChanged(bool);

	void videoTrackInfoChanged();
	void videoTrackChanged(int);
	void audioTrackInfoChanged();
	void audioTrackChanged(int);
	void subtitleInfoChanged();
	void subtitleTrackChanged(int);
#ifdef MPV_SUPPORT
	void secondarySubtitleTrackChanged(int);
#endif
	void titleTrackInfoChanged();
	void titleTrackChanged(int);
	void chapterInfoChanged();
	void chapterChanged(int);
	void ABMarkersChanged();
	void osdLevelChanged(int);
	void videoEqualizerNeedsUpdate();

	void needResize(int w, int h);

#ifdef YOUTUBE_SUPPORT
	void signatureNotFound(const QString&);
	void noSslSupport();
#endif

protected:
	//! Change the current state (Stopped, Playing or Paused)
	//! And sends the stateChanged() signal.
	void setState(State s);

	void initVolume();
	void initMediaSettings();
	void initPlaying(int seek = -1);
	void startPlayer(QString file, double seek = -1);
	void stopPlayer();
	void restartPlay();

	void newMediaPlayingStarted();
	void saveMediaInfo();

	int adjustVolume(int volume);
	void seek_cmd(double secs, int mode);

protected slots:
	void playingStarted();
	void processError(QProcess::ProcessError error);
	void processFinished(bool normal_exit);
	void fileReachedEnd();

	void gotCurrentSec(double sec);
	void onReceivedPause();
	void gotVideoOutResolution(int w, int h);
	void gotVO(const QString&);
	void gotAO(const QString&);

	void displayScreenshotName(const QString& filename);
	void displayUpdatingFontCache();
	void displayBuffering();
	void displayBufferingEnded();

	void streamTitleChanged(const QString&);
	void streamTitleAndUrlChanged(const QString&, const QString&);

	// Catches mediaInfoChanged and sends mediaPlaying signal
	void sendMediaInfo();
	
	void watchState(TCore::State state);

	//! Called when a video has just started to play.
	//! This function checks if the codec of video is ffh264 and if
	//! the resolution is HD
	void checkIfVideoIsHD();

	void gotAudioTrackInfo();
	void gotAudioTrackChanged(int id);

	void selectPreferredSub();
	void gotSubtitleInfo();
	void gotSubtitleChanged();

	void dvdnavUpdateMousePos(QPoint);
	void dvdnavSeek();
	void dvdnavRestoreTitle();
	void changeTitleLeaveMenu();

#ifdef YOUTUBE_SUPPORT
	void connectingToYT(const QString& host);
	void YTFailed(int error_number, const QString& error_str);
	void YTNoVideoUrl();
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef SCREENSAVER_OFF
	void enableScreensaver();
	void disableScreensaver();
#endif
#endif

protected:
	Proc::TPlayerProcess* proc;
	TPlayerWindow* playerwindow;

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef SCREENSAVER_OFF
	WinScreenSaver* win_screensaver;
#endif
#endif

#ifdef YOUTUBE_SUPPORT
	RetrieveYoutubeUrl* yt;
#endif

private:
	State _state;

	bool we_are_restarting;
	// Get DVDNAV to restart
	int title;
	int title_to_select;
	double title_time;
	bool title_was_menu;
	bool block_dvd_nav;
	int menus_selected;

	bool change_volume_after_unpause;
	QTime time;

	QString initial_subtitle;
	QMap<QString,QString> forced_titles;

	// Max value [0.. pos_max) for gotoPosition() and positionChanged()
	int pos_max;

	int cache_size;

	void openDisc(TDiscData &disc, bool fast_open);
	void openFile(const QString& filename, int seek = -1);

	bool isMPlayer();
	bool isMPV();
	bool haveVideoFilters();
	void changeVF(const QString& filter, bool enable, const QVariant& option);
	void getZoomFromPlayerWindow();
	void getPanFromPlayerWindow();
	void pan(int dx, int dy);
	void setExternalSubs(const QString& filename);
	bool setPreferredAudio();
};

#endif
