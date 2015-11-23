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
#include "settings/preferences.h"
#include "core.h"
#include "gui/pref/dialog.h"

#ifdef Q_OS_WIN
#ifdef AVOID_SCREENSAVER
/* Disable screensaver by event */
#include <windows.h>
#endif
#endif


class QWidget;
class QMenu;

class TMediaData;
class TPlayerWindow;

#ifdef FIND_SUBTITLES
class FindSubtitlesWindow;
#endif

#ifdef VIDEOPREVIEW
class VideoPreview;
#endif


namespace Gui {

class TAction;
class TActionGroup;
class TTimeSliderAction;
class TVolumeSliderAction;
class TTimeLabelAction;
class TEditableToolbar;
class TMenuOpen;
class TMenuPlay;
class TMenuVideo;
class TMenuAudio;
class TMenuSubtitle;
class TMenuBrowse;
class TAutoHideTimer;
class TLogWindow;
class TPlaylist;
class TFilePropertiesDialog;
class TAudioEqualizer;
class TVideoEqualizer;
class TUpdateChecker;


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
	TActionList getAllNamedActions();
	QMenu* getToolbarMenu() { return toolbar_menu; }

	virtual void loadConfig();
	virtual void saveConfig();

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

	virtual void showConfigFolder();

	virtual void helpFirstSteps();
	virtual void helpFAQ();
	virtual void helpCLOptions();
	virtual void helpCheckUpdates();
	virtual void helpAbout();

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

	virtual void showPlaylist(bool b);
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

protected slots:
	virtual void closeWindow();
	// Replace for setCaption (in Qt 4 it's not virtual)
	virtual void setWindowCaption(const QString& title);

	virtual void onStateChanged(TCore::State state);

	virtual void onMediaSettingsChanged();
	virtual void onVideoOutResolutionChanged(int w, int h);
	virtual void gotCurrentTime(double);
	virtual void gotDuration(double);
	virtual void newMediaLoaded();
	virtual void updateMediaInfo();
	void gotForbidden();

	virtual void updateVideoEqualizer();
	virtual void updateAudioEqualizer();
	virtual void setDefaultValuesFromVideoEqualizer();
	virtual void changeVideoEqualizerBySoftware(bool b);

	virtual void displayMessage(QString message, int time);
	virtual void displayMessage(QString message);

	virtual void openRecent();
	virtual void exitFullscreenOnStop();
	virtual void playlistHasFinished();

#ifdef CHECK_UPGRADED
	void checkIfUpgraded();
#endif

#ifdef YOUTUBE_SUPPORT
	void YTNoSslSupport();
	void YTNoSignature(const QString&);
	#ifdef YT_USE_YTSIG
	void YTUpdateScript();
	#endif
#endif

	virtual void enableActionsOnPlaying();
	virtual void disableActionsOnStop();

	void changeSize(int precentage);
	void toggleDoubleSize();

	virtual void resizeWindow(int w, int h);

	virtual void displayGotoTime(int);
	//! You can call this slot to jump to the specified percentage in the video, while dragging the slider.
	virtual void goToPosOnDragging(int);

	virtual void showContextMenu();
	virtual void showContextMenu(QPoint p);
	void showStatusBarPopup(const QPoint& pos);

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

	// Single instance stuff
#ifdef SINGLE_INSTANCE
	void handleMessageFromOtherInstances(const QString& message);
#endif

	void showExitCodeFromPlayer(int exit_code);
	void showErrorFromPlayer(QProcess::ProcessError);

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef AVOID_SCREENSAVER
	/* Disable screensaver by event */
	void clear_just_stopped();
#endif
#endif

signals:
	void enableActions(bool stopped, bool video, bool audio);
	void mediaSettingsChanged(Settings::TMediaSettings* mset);

	void frameChanged(int);
	void videoInfoChanged(int width, int height, double fps);
	void timeChanged(QString time_ready_to_print);

	void aboutToEnterFullscreenSignal();
	void didEnterFullscreenSignal();
	void aboutToExitFullscreenSignal();
	void didExitFullscreenSignal();
	void stayOnTopChanged(int);

	//! Sent when another instance requested to play a file
	void openFileRequested();

	void loadTranslation();
	void requestRestart(bool);

protected:
	virtual void closeEvent(QCloseEvent* e);
	virtual void changeEvent(QEvent* event);
	virtual void hideEvent(QHideEvent* event);
	virtual void showEvent(QShowEvent* event);
	virtual void resizeEvent(QResizeEvent* event);

#if QT_VERSION >= 0x050000
	virtual bool event(QEvent* e);
	bool was_minimized;
#endif

#ifdef Q_OS_WIN
#ifdef AVOID_SCREENSAVER
	/* Disable screensaver by event */
	virtual bool winEvent (MSG* m, long* result);
#endif
#endif
	virtual QMenu* createPopupMenu();
	virtual void aboutToEnterFullscreen();
	virtual void didEnterFullscreen();
	virtual void aboutToExitFullscreen();
	virtual void didExitFullscreen();
	virtual QString settingsGroupName() = 0;

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
	TAction* findAction(const QString& name);
	void createMenus();
	void configureDiscDevices();
	void setupNetworkProxy();

protected:
	QWidget* panel;

	// Actions not in menus
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

	TAction* showContextMenuAct;
	TAction* nextWheelFunctionAct;

	// MENUS
	TMenuOpen* openMenu;
	TMenuPlay* playMenu;
	TMenuVideo* videoMenu;
	TMenuAudio* audioMenu;
	TMenuSubtitle* subtitleMenu;
	TMenuBrowse* browseMenu;
	QMenu* optionsMenu;
	QMenu* helpMenu;

	QMenu* popup;

	// Toolbar menu
	TAction* viewMenuBarAct;
	TAction* editToolbarAct;
	TAction* editToolbar2Act;
	TAction* editControlBarAct;
	TAction* viewStatusBarAct;

	QMenu* toolbar_menu;
	QMenu* statusbar_menu;

	TEditableToolbar* toolbar;
	TEditableToolbar* toolbar2;
	TEditableToolbar* controlbar;

	TTimeSliderAction* timeslider_action;
	TVolumeSliderAction* volumeslider_action;
	TTimeLabelAction* time_label_action;

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

#ifdef UPDATE_CHECKER
	TUpdateChecker* update_checker;
#endif

	QString pending_actions_to_run;

private:
	bool menubar_visible;
	bool statusbar_visible;
	bool fullscreen_menubar_visible;
	bool fullscreen_statusbar_visible;

	// Force settings from command line
	int arg_close_on_finish; // -1 = not set, 1 = true, 0 = false
	int arg_start_in_fullscreen; // -1 = not set, 1 = true, 0 = false

	// Variables to restore pos and size of the window
	// when exiting from fullscreen mode.
	QPoint win_pos;
	QSize win_size;
	bool was_maximized;

	bool ignore_show_hide_events;
	bool block_resize;
	bool center_window;
	int block_update_size_factor;

	TAutoHideTimer* auto_hide_timer;

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef AVOID_SCREENSAVER
	/* Disable screensaver by event */
	bool just_stopped;
#endif
#endif

	void createToolbars();
	QMenu* createToolbarMenu();

	QString exitCodeToMessage(int exit_code);

	void setFloatingToolbarsVisible(bool visible);
	void hidePanel();
	void centerWindow();
	bool optimizeSizeFactorPreDef(double factor);
	void optimizeSizeFactor(int w, int h);
	void resizeMainWindow(int w, int h, bool try_twice = true);

	void enterFullscreenOnPlay();
	void checkPendingActionsToRun();
	void retranslateStrings();

private slots:
	void unlockSizeFactor();
};

} // namespace Gui

#endif // _GUI_BASE_H_

