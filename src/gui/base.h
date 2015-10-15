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

#ifndef _GUI_BASE_H_
#define _GUI_BASE_H_

#include <QMainWindow>
#include <QNetworkProxy>

#include "config.h"
#include "mediadata.h"
#include "mediasettings.h"
#include "core.h"
#include "settings/preferences.h"
#include "gui/guiconfig.h"
#include "gui/widgetactions.h"
#include "gui/autohidetoolbar.h"
#include "gui/playlist.h"
#include "gui/logwindow.h"
#include "gui/audioequalizer.h"
#include "gui/videoequalizer.h"
#include "gui/favorites.h"
#include "gui/tvlist.h"
#include "gui/filepropertiesdialog.h"
#include "gui/updatechecker.h"
#include "gui/pref/dialog.h"

#ifdef Q_OS_WIN
#ifdef AVOID_SCREENSAVER
/* Disable screensaver by event */
#include <windows.h>
#endif
#endif

//#define SHARE_MENU

class QWidget;
class QMenu;
class TPlayerWindow;

class QLabel;
#ifdef FIND_SUBTITLES
class FindSubtitlesWindow;
#endif

#ifdef VIDEOPREVIEW
class VideoPreview;
#endif

#ifdef SHAREWIDGET
class ShareWidget;
#endif

namespace Gui {

class TBase : public QMainWindow {
	Q_OBJECT

public:
	TBase();
	virtual ~TBase();

	/* Return true if the window shouldn't show on startup */
	virtual bool startHidden() { return false; }

	//! Execute all actions in \a actions. The actions should be
	//! separated by spaces. Checkable actions could have a parameter:
	//! true or false.
	void runActions(QString actions);

	//! Execute all the actions after the video has started to play
	void runActionsLater(QString actions) { pending_actions_to_run = actions; }

	TCore* getCore() { return core; }
	TPlaylist* getPlaylist() { return playlist; }

	virtual void loadConfig(const QString& gui_group);
	virtual void saveConfig(const QString& gui_group);

	void retranslate() { retranslateStrings(); }

public slots:
	virtual void open(const QString& file); // Generic open, autodetect type.
	virtual void openFile();
	virtual void openFiles(QStringList files);
	virtual void openFavorite(QString file);
	virtual void openURL();
	virtual void openURL(QString url);
	virtual void openVCD();
	virtual void openAudioCD();
	virtual void openDVD();
	virtual void openDVDFromFolder();
	virtual void openDVDFromFolder(const QString& directory);
	void openBluRay();
	void openBluRayFromFolder();
	void openBluRayFromFolder(QString directory);
	virtual void openDirectory();
	virtual void openDirectory(QString directory);

	virtual void helpFirstSteps();
	virtual void helpFAQ();
	virtual void helpCLOptions();
	virtual void helpCheckUpdates();
#ifdef REMINDER_ACTIONS
	virtual void helpDonate();
#endif
	virtual void helpShowConfig();
	virtual void helpAbout();

#ifdef SHARE_MENU
	virtual void shareSMPlayer();
#endif

	virtual void loadSub();
	virtual void loadAudioFile(); // Load external audio file

	void setInitialSubtitle(const QString& subtitle_file);

#ifdef FIND_SUBTITLES
	virtual void showFindSubtitlesDialog();
	virtual void openUploadSubtitlesPage(); //turbos
#endif

#ifdef VIDEOPREVIEW
	virtual void showVideoPreviewDialog();
#endif

#ifdef YOUTUBE_SUPPORT
	virtual void showTubeBrowser();
#endif

	virtual void showPlaylist();
	virtual void showPlaylist(bool b);
	virtual void showVideoEqualizer();
	virtual void showVideoEqualizer(bool b);
	virtual void showAudioEqualizer();
	virtual void showAudioEqualizer(bool b);
	virtual void showLog();
	virtual void showPreferencesDialog();
	virtual void showFilePropertiesDialog();

	virtual void showGotoDialog();
	virtual void showSubDelayDialog();
	virtual void showAudioDelayDialog();
	virtual void showStereo3dDialog();

	virtual void exitFullscreen();
	virtual void toggleFullscreen();
	virtual void toggleFullscreen(bool);

	void setStayOnTop(bool b);
	virtual void changeStayOnTop(int);
	virtual void checkStayOnTop(TCore::State);
	void toggleStayOnTop();

	void setForceCloseOnFinish(int n) { arg_close_on_finish = n; }
	int forceCloseOnFinish() { return arg_close_on_finish; }

	void setForceStartInFullscreen(int n) { arg_start_in_fullscreen = n; }
	int forceStartInFullscreen() { return arg_start_in_fullscreen; }

	void slotNoVideo();

protected slots:
	virtual void closeWindow();

	virtual void setJumpTexts();

	// Replace for setCaption (in Qt 4 it's not virtual)
	virtual void setWindowCaption(const QString& title);

	virtual void openRecent();
	virtual void enterFullscreenOnPlay();
	virtual void exitFullscreenOnStop();
	virtual void exitFullscreenIfNeeded();
	virtual void playlistHasFinished();

	virtual void displayState(TCore::State state);
	virtual void displayMessage(QString message, int time);
	virtual void displayMessage(QString message);
	virtual void gotCurrentTime(double);
	virtual void gotDuration(double);

	virtual void updateWidgets();
	virtual void updateVideoTracks();
	virtual void updateAudioTracks();
	virtual void updateSubtitles();
	virtual void updateTitles();
	virtual void updateChapters();
	virtual void updateAngles();
	virtual void updateVideoEqualizer();
	virtual void updateAudioEqualizer();
	virtual void setDefaultValuesFromVideoEqualizer();
	virtual void changeVideoEqualizerBySoftware(bool b);

	virtual void newMediaLoaded();
	virtual void updateMediaInfo();

	void gotNoFileToPlay();

	void checkPendingActionsToRun();

#if REPORT_OLD_MPLAYER
	void checkMplayerVersion();
	void displayWarningAboutOldMplayer();
#endif

#ifdef CHECK_UPGRADED
	void checkIfUpgraded();
#endif

#if defined(REMINDER_ACTIONS) && !defined(SHAREWIDGET)
	void checkReminder();
#endif

#ifdef YOUTUBE_SUPPORT
	void YTNoSslSupport();
	void YTNoSignature(const QString&);
	#ifdef YT_USE_YTSIG
	void YTUpdateScript();
	#endif
#endif
	void gotForbidden();

	virtual void enableActionsOnPlaying();
	virtual void disableActionsOnStop();
	virtual void togglePlayAction(TCore::State);

	void toggleDoubleSize();
	void resizeMainWindow(int w, int h, bool try_twice = true);
	virtual void resizeWindow(int w, int h);

	virtual void displayGotoTime(int);
	//! You can call this slot to jump to the specified percentage in the video, while dragging the slider.
	virtual void goToPosOnDragging(int);

	virtual void showPopupMenu();
	virtual void showPopupMenu(QPoint p);

	virtual void leftClickFunction();
	virtual void rightClickFunction();
	virtual void doubleClickFunction();
	virtual void middleClickFunction();
	virtual void xbutton1ClickFunction();
	virtual void xbutton2ClickFunction();
	virtual void moveWindow(QPoint diff);
	virtual void processFunction(QString function);

	virtual void dragEnterEvent(QDragEnterEvent*);
	virtual void dropEvent(QDropEvent*);

	virtual void applyNewPreferences();
	virtual void applyFileProperties();

	virtual void clearRecentsList();

	virtual void loadActions();
	virtual void saveActions();

	// Single instance stuff
#ifdef SINGLE_INSTANCE
	void handleMessageFromOtherInstances(const QString& message);
#endif

	//! Called when core can't parse the mplayer version and there's no
	//! version supplied by the user
	void askForMplayerVersion(QString);

	void showExitCodeFromPlayer(int exit_code);
	void showErrorFromPlayer(QProcess::ProcessError);

	// stylesheet
#if ALLOW_CHANGE_STYLESHEET
	virtual QString loadQss(QString filename);
	virtual void changeStyleSheet(QString style);
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef AVOID_SCREENSAVER
	/* Disable screensaver by event */
	void clear_just_stopped();
#endif
#endif

signals:
	void frameChanged(int);
	void ABMarkersChanged(int secs_a, int secs_b);
	void videoInfoChanged(int width, int height, double fps);
	void timeChanged(QString time_ready_to_print);

	//! Sent when another instance requested to play a file
	void openFileRequested();

	void loadTranslation();
	void requestRestart();

protected:
	virtual void retranslateStrings();
	virtual void changeEvent(QEvent* event);
#if QT_VERSION < 0x050000
	virtual void hideEvent(QHideEvent*);
	virtual void showEvent(QShowEvent*);
#else
	virtual bool event(QEvent* e);
	bool was_minimized;
#endif
#ifdef Q_OS_WIN
	#ifdef AVOID_SCREENSAVER
	/* Disable screensaver by event */
	virtual bool winEvent (MSG* m, long* result);
	#endif
#endif

	virtual void aboutToEnterFullscreen();
	virtual void aboutToExitFullscreen();

protected:
	void createCore();
	void createPlayerWindow();
	void createVideoEqualizer();
	void createAudioEqualizer();
	void createPlaylist();
	void createPanel();
	void createPreferencesDialog();
	void createFilePropertiesDialog();
	void setDataToFileProperties();
	void createActions();
	void setActionsEnabled(bool);
	void createMenus();
	void updateRecents();
	void configureDiscDevices();
	void setupNetworkProxy();
	virtual void closeEvent(QCloseEvent* e);

protected:
	QWidget* panel;

	// Menu File
	TAction* openFileAct;
	TAction* openDirectoryAct;
	TAction* openPlaylistAct;
	TAction* openVCDAct;
	TAction* openAudioCDAct;
	TAction* openDVDAct;
	TAction* openDVDFolderAct;
	TAction* openBluRayAct;
	TAction* openBluRayFolderAct;
	TAction* openURLAct;
	TAction* exitAct;
	TAction* clearRecentsAct;

	// Menu Play
	TAction* playAct;
	TAction* playOrPauseAct;
	TAction* pauseAct;
	TAction* stopAct;
	TAction* frameStepAct;
	TAction* frameBackStepAct;
	TAction* rewind1Act;
	TAction* rewind2Act;
	TAction* rewind3Act;
	TAction* forward1Act;
	TAction* forward2Act;
	TAction* forward3Act;
	TAction* repeatAct;
	TAction* setAMarkerAct;
	TAction* setBMarkerAct;
	TAction* clearABMarkersAct;
	TAction* gotoAct;

	// Menu Speed
	TAction* normalSpeedAct;
	TAction* halveSpeedAct;
	TAction* doubleSpeedAct;
	TAction* decSpeed10Act;
	TAction* incSpeed10Act;
	TAction* decSpeed4Act;
	TAction* incSpeed4Act;
	TAction* decSpeed1Act;
	TAction* incSpeed1Act;

	// Menu Video
	TAction* fullscreenAct;
	TAction* videoEqualizerAct;
	TAction* screenshotAct;
	TAction* screenshotsAct;
#ifdef VIDEOPREVIEW
	TAction* videoPreviewAct;
#endif
	TAction* flipAct;
	TAction* mirrorAct;
	TAction* stereo3dAct;
	TAction* postProcessingAct;
	TAction* phaseAct;
	TAction* deblockAct;
	TAction* deringAct;
	TAction* gradfunAct;
	TAction* addNoiseAct;
	TAction* addLetterboxAct;
	TAction* upscaleAct;

	// Menu Audio
	TAction* audioEqualizerAct;
	TAction* muteAct;
	TAction* decVolumeAct;
	TAction* incVolumeAct;
	TAction* decAudioDelayAct;
	TAction* incAudioDelayAct;
	TAction* audioDelayAct; // Ask for delay
#ifdef MPLAYER_SUPPORT
	TAction* extrastereoAct;
	TAction* karaokeAct;
#endif
	TAction* volnormAct;
	TAction* loadAudioAct;
	TAction* unloadAudioAct;

	// Menu Subtitles
	TAction* loadSubsAct;
	TAction* unloadSubsAct;
	TAction* decSubDelayAct;
	TAction* incSubDelayAct;
	TAction* subDelayAct; // Ask for delay
	TAction* decSubPosAct;
	TAction* incSubPosAct;
	TAction* incSubStepAct;
	TAction* decSubStepAct;
	TAction* incSubScaleAct;
	TAction* decSubScaleAct;
#ifdef MPV_SUPPORT
	TAction* seekNextSubAct;
	TAction* seekPrevSubAct;
#endif
	TAction* useCustomSubStyleAct;
	TAction* useForcedSubsOnlyAct;
#ifdef FIND_SUBTITLES
	TAction* showFindSubtitlesDialogAct;
	TAction* openUploadSubtitlesPageAct;//turbos
#endif

	// Menu Options
	TAction* showPlaylistAct;
	TAction* showPropertiesAct;
	TAction* showPreferencesAct;
#ifdef YOUTUBE_SUPPORT
	TAction* showTubeBrowserAct;
#endif
	TAction* showLogAct;

	// Menu Help
	TAction* showFirstStepsAct;
	TAction* showFAQAct;
	TAction* showCLOptionsAct; // Command line options
	TAction* showCheckUpdatesAct;
#if defined(YOUTUBE_SUPPORT) && defined(YT_USE_YTSIG)
	TAction* updateYTAct;
#endif
	TAction* showConfigAct;
#ifdef REMINDER_ACTIONS
	TAction* donateAct;
#endif
	TAction* aboutThisAct;

#ifdef SHARE_MENU
	TAction* facebookAct;
	TAction* twitterAct;
	TAction* gmailAct;
	TAction* hotmailAct;
	TAction* yahooAct;
#endif

	// OSD
	TAction* incOSDScaleAct;
	TAction* decOSDScaleAct;

	// TPlaylist
	TAction* playPrevAct;
	TAction* playNextAct;

	// Actions not in menus
	TAction* exitFullscreenAct;
	TAction* nextOSDLevelAct;
	TAction* decContrastAct;
	TAction* incContrastAct;
	TAction* decBrightnessAct;
	TAction* incBrightnessAct;
	TAction* decHueAct;
	TAction* incHueAct;
	TAction* decSaturationAct;
	TAction* incSaturationAct;
	TAction* decGammaAct;
	TAction* incGammaAct;
	TAction* nextVideoAct;
	TAction* nextAudioAct;
	TAction* nextSubtitleAct;
	TAction* nextChapterAct;
	TAction* prevChapterAct;
	TAction* doubleSizeAct;
	TAction* resetVideoEqualizerAct;
	TAction* resetAudioEqualizerAct;
	TAction* showContextMenuAct;
	TAction* nextAspectAct;
	TAction* nextWheelFunctionAct;

	TAction* showFilenameAct;
	TAction* showTimeAct;
	TAction* toggleDeinterlaceAct;

	// Moving and zoom
	TAction* moveUpAct;
	TAction* moveDownAct;
	TAction* moveLeftAct;
	TAction* moveRightAct;
	TAction* incZoomAct;
	TAction* decZoomAct;
	TAction* resetZoomAct;
	TAction* autoZoomAct;
	TAction* autoZoom169Act;
	TAction* autoZoom235Act;

	// OSD Action Group 
	TActionGroup* osdGroup;
	TAction* osdNoneAct;
	TAction* osdSeekAct;
	TAction* osdTimerAct;
	TAction* osdTotalAct;

	// Denoise Action Group
	TActionGroup* denoiseGroup;
	TAction* denoiseNoneAct;
	TAction* denoiseNormalAct;
	TAction* denoiseSoftAct;

	// Blur-sharpen group
	TActionGroup* unsharpGroup;
	TAction* unsharpNoneAct;
	TAction* blurAct;
	TAction* sharpenAct;

	// Window Size Action Group
	TActionGroup* sizeGroup;
	TAction* size50;
	TAction* size75;
	TAction* size100;
	TAction* size125;
	TAction* size150;
	TAction* size175;
	TAction* size200;
	TAction* size300;
	TAction* size400;

	// Deinterlace Action Group
	TActionGroup* deinterlaceGroup;
	TAction* deinterlaceNoneAct;
	TAction* deinterlaceL5Act;
	TAction* deinterlaceYadif0Act;
	TAction* deinterlaceYadif1Act;
	TAction* deinterlaceLBAct;
	TAction* deinterlaceKernAct;

	// Aspect Action Group
	TActionGroup* aspectGroup;
	TAction* aspectDetectAct;
	TAction* aspectNoneAct;
	TAction* aspect11Act;		// 1:1
	TAction* aspect32Act;		// 3:2
	TAction* aspect43Act;		// 4:3
	TAction* aspect118Act;		// 11:8
	TAction* aspect54Act;		// 5:4
	TAction* aspect149Act;		// 14:9
	TAction* aspect1410Act;	// 14:10
	TAction* aspect169Act;		// 16:9
	TAction* aspect1610Act;	// 16:10
	TAction* aspect235Act;		// 2.35:1

	// Rotate Group
	TActionGroup* rotateGroup;
	TAction* rotateNoneAct;
	TAction* rotateClockwiseFlipAct;
	TAction* rotateClockwiseAct;
	TAction* rotateCounterclockwiseAct;
	TAction* rotateCounterclockwiseFlipAct;

	// Menu StayOnTop
	TActionGroup* onTopActionGroup;
	TAction* onTopAlwaysAct;
	TAction* onTopNeverAct;
	TAction* onTopWhilePlayingAct;
	TAction* toggleStayOnTopAct;

#if USE_ADAPTER
	// Screen Group
	TActionGroup* screenGroup;
	TAction* screenDefaultAct;
#endif

	// Closed Captions Group
	TActionGroup* ccGroup;
	TAction* ccNoneAct;
	TAction* ccChannel1Act;
	TAction* ccChannel2Act;
	TAction* ccChannel3Act;
	TAction* ccChannel4Act;

	// External sub fps Group
	TActionGroup* subFPSGroup;
	TAction* subFPSNoneAct;
	/* TAction* subFPS23Act; */
	TAction* subFPS23976Act;
	TAction* subFPS24Act;
	TAction* subFPS25Act;
	TAction* subFPS29970Act;
	TAction* subFPS30Act;

	// Audio Channels Action Group
	TActionGroup* channelsGroup;
	/* TAction* channelsDefaultAct; */
	TAction* channelsStereoAct;
	TAction* channelsSurroundAct;
	TAction* channelsFull51Act;
	TAction* channelsFull61Act;
	TAction* channelsFull71Act;

	// Stereo Mode Action Group
	TActionGroup* stereoGroup;
	TAction* stereoAct;
	TAction* leftChannelAct;
	TAction* rightChannelAct;
	TAction* monoAct;
	TAction* reverseAct;

	// Other groups
#if PROGRAM_SWITCH
	TActionGroup* programTrackGroup;
#endif
	TActionGroup* videoTrackGroup;
	TActionGroup* audioTrackGroup;
	TActionGroup* subtitleTrackGroup;
#ifdef MPV_SUPPORT
	TActionGroup* secondarySubtitleTrackGroup;
#endif
	TActionGroup* titleGroup;
	TActionGroup* angleGroup;
	TActionGroup* chapterGroup;

	TAction* dvdnavUpAct;
	TAction* dvdnavDownAct;
	TAction* dvdnavLeftAct;
	TAction* dvdnavRightAct;
	TAction* dvdnavMenuAct;
	TAction* dvdnavSelectAct;
	TAction* dvdnavPrevAct;
	TAction* dvdnavMouseAct;

	// MENUS
	QMenu *openMenu;
	QMenu *playMenu;
	QMenu *videoMenu;
	QMenu *audioMenu;
	QMenu *subtitlesMenu;
	QMenu *browseMenu;
	QMenu *optionsMenu;
	QMenu *helpMenu;

	QMenu* disc_menu;
	QMenu* subtitles_track_menu;
#ifdef MPV_SUPPORT
	QMenu* secondary_subtitles_track_menu;
#endif
#if PROGRAM_SWITCH
	QMenu* programtrack_menu;
#endif
	QMenu* videotrack_menu;
	QMenu* audiotrack_menu;
	QMenu* titles_menu;
	QMenu* chapters_menu;
	QMenu* angles_menu;
	QMenu* aspect_menu;
	QMenu* osd_menu;
	QMenu* deinterlace_menu;
	QMenu* denoise_menu;
	QMenu* unsharp_menu;
	QMenu* videosize_menu;
	QMenu* audiochannels_menu;
	QMenu* stereomode_menu;

	QMenu* speed_menu;
	QMenu* ab_menu; // A-B menu
	QMenu* videofilter_menu;
	QMenu* audiofilter_menu;
	QMenu* zoom_menu;
	QMenu* rotate_menu;
	QMenu* ontop_menu;
#if USE_ADAPTER
	QMenu* screen_menu;
#endif
	QMenu* closed_captions_menu;
	QMenu* subfps_menu;

#ifdef SHARE_MENU
	QMenu* share_menu;
#endif

	QMenu* popup;
	QMenu* recentfiles_menu;

	TAction* viewMenuBarAct;
	TAction* editToolbarAct;
	TAction* editControlBarAct;
	TAction* viewStatusBarAct;

	TEditableToolbar* toolbar;

	TTimeSliderAction* timeslider_action;
	TVolumeSliderAction* volumeslider_action;
	TTimeLabelAction* time_label_action;
	TAutohideToolbar* controlbar;

	TLogWindow* log_window;
	TLogWindow* clhelp_window;

	Pref::TDialog* pref_dialog;
	TFilePropertiesDialog* file_dialog;
	TPlaylist* playlist;
	TVideoEqualizer* video_equalizer;
	TAudioEqualizer* audio_equalizer;
#ifdef FIND_SUBTITLES
	FindSubtitlesWindow* find_subs_dialog;
#endif
#ifdef VIDEOPREVIEW
	VideoPreview* video_preview;
#endif

	TCore* core;
	TPlayerWindow* playerwindow;

	TFavorites* favorites;
	TTVList* tvlist;
	TTVList* radiolist;

#ifdef UPDATE_CHECKER
	TUpdateChecker* update_checker;
#endif

#ifdef SHAREWIDGET
	ShareWidget* sharewidget;
#endif

	QStringList actions_list;
	QString pending_actions_to_run;

private:
	bool menubar_visible;
	bool statusbar_visible;
	bool fullscreen_menubar_visible;
	bool fullscreen_statusbar_visible;

	// Force settings from command line
	int arg_close_on_finish; // -1 = not set, 1 = true, 0 = false
	int arg_start_in_fullscreen; // -1 = not set, 1 = true, 0 = false

	QString default_style;

	// Variables to restore pos and size of the window
	// when exiting from fullscreen mode.
	QPoint win_pos;
	QSize win_size;
	bool was_maximized;

	bool ignore_show_hide_events;
	bool block_resize;

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef AVOID_SCREENSAVER
	/* Disable screensaver by event */
	bool just_stopped;
#endif
#endif

	void reconfigureControlBar();
	void createToolbars();
	void hidePanel();
};

} // namespace Gui

#endif // _GUI_BASE_H_

