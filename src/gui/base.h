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

#ifndef GUI_BASE_H
#define GUI_BASE_H

#include <QTimer>
#include <QMainWindow>
#include <QProcess>

#include "config.h"
#include "corestate.h"
#include "gui/action/actionlist.h"

#if defined(Q_OS_WIN) && defined(DISABLE_SCREENSAVER)
/* Disable screensaver by event */
#include <windows.h>
#endif


class QWidget;
class QMenu;

class TCore;
class TPlayerWindow;

#ifdef FIND_SUBTITLES
class FindSubtitlesWindow;
#endif

namespace Settings {
class TMediaSettings;
}


namespace Gui {

namespace Action {
class TAction;
class TMenuOpen;
class TMenuPlay;
class TMenuVideo;
class TMenuAudio;
class TMenuSubtitle;
class TMenuBrowse;
class TEditableToolbar;
class TTimeSliderAction;
class TVolumeSliderAction;
class TTimeLabelAction;
}

namespace Pref {
class TDialog;
}

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
	Action::TActionList getAllNamedActions();
	QMenu* getToolbarMenu() { return toolbar_menu; }

	virtual void loadConfig();
	virtual void saveConfig();

public slots:
	virtual void open(const QString& file); // Generic open, autodetect type.
	virtual void openFile();
	virtual void openFiles(QStringList files, int item = -1);
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
	virtual void checkStayOnTop(TCoreState);
	void toggleStayOnTop();

	void changeSize(double factor);
	void changeSize(int percentage);

	void setForceCloseOnFinish(int n) { arg_close_on_finish = n; }
	int forceCloseOnFinish() { return arg_close_on_finish; }

	void setForceStartInFullscreen(int n) { arg_start_in_fullscreen = n; }
	int forceStartInFullscreen() { return arg_start_in_fullscreen; }

signals:
	void enableActions(bool stopped, bool video, bool audio);

	void timeChanged(QString time_ready_to_print);
	void frameChanged(int);

	void mediaSettingsChanged(Settings::TMediaSettings* mset);
	void mediaFileTitleChanged(const QString& filename, const QString& title);
	void videoInfoChanged(int width, int height, double fps);

	void fullscreenChanged();
	void fullscreenChangedDone();
	void aboutToEnterFullscreenSignal();
	void didEnterFullscreenSignal();
	void aboutToExitFullscreenSignal();
	void didExitFullscreenSignal();

	void videoSizeFactorChanged();
	void mainWindowResizeEvent(QResizeEvent* event);
	void stayOnTopChanged(int);

	//! Sent when another instance requested to play a file
	void openFileRequested();

	void loadTranslation();
	void requestRestart(bool);

protected slots:
	virtual void closeWindow();
	// Replace for setCaption (in Qt 4 it's not virtual)
	virtual void setWindowCaption(const QString& title);

	virtual void onStateChanged(TCoreState state);

	virtual void onMediaSettingsChanged();
	virtual void onVideoOutResolutionChanged(int w, int h);
	virtual void gotCurrentTime(double);
	virtual void gotDuration(double);
	virtual void onNewMediaStartedPlaying();
	virtual void updateMediaInfo();

	virtual void updateVideoEqualizer();
	virtual void updateAudioEqualizer();
	virtual void setDefaultValuesFromVideoEqualizer();
	virtual void changeVideoEqualizerBySoftware(bool b);

	virtual void displayMessage(const QString& message, int time = TConfig::MESSAGE_DURATION);

	virtual void openRecent();
	virtual void exitFullscreenOnStop();
	virtual void playlistHasFinished();

	virtual void enableActionsOnPlaying();
	virtual void disableActionsOnStop();

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

	void onPlayerFinishedWithError(int exit_code);
	void onPlayerError(QProcess::ProcessError);

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

#if defined(Q_OS_WIN) && defined(DISABLE_SCREENSAVER)
	/* Disable screensaver by event */
	virtual bool winEvent (MSG* m, long* result);
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
	Action::TAction* findAction(const QString& name);
	void createMenus();
	void configureDiscDevices();
	void setupNetworkProxy();

protected:
	QWidget* panel;
	TPlayerWindow* playerwindow;
	TCore* core;

	Action::TAction* showContextMenuAct;
	Action::TAction* nextWheelFunctionAct;

	// MENUS
	Action::TMenuOpen* openMenu;
	Action::TMenuPlay* playMenu;
	Action::TMenuVideo* videoMenu;
	Action::TMenuAudio* audioMenu;
	Action::TMenuSubtitle* subtitleMenu;
	Action::TMenuBrowse* browseMenu;
	QMenu* optionsMenu;
	QMenu* helpMenu;

	QMenu* popup;

	// Toolbar menu
	Action::TAction* viewMenuBarAct;
	Action::TAction* editToolbarAct;
	Action::TAction* editToolbar2Act;
	Action::TAction* editControlBarAct;
	Action::TAction* viewStatusBarAct;

	QMenu* toolbar_menu;
	QMenu* statusbar_menu;

	Action::TEditableToolbar* toolbar;
	Action::TEditableToolbar* toolbar2;
	Action::TEditableToolbar* controlbar;

	Action::TTimeSliderAction* timeslider_action;
	Action::TVolumeSliderAction* volumeslider_action;
	Action::TTimeLabelAction* time_label_action;

	TPlaylist* playlist;
	TLogWindow* log_window;
	TLogWindow* help_window;

	Pref::TDialog* pref_dialog;
	TFilePropertiesDialog* file_properties_dialog;
	TVideoEqualizer* video_equalizer;
	TAudioEqualizer* audio_equalizer;

#ifdef FIND_SUBTITLES
	FindSubtitlesWindow* find_subs_dialog;
#endif

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

	bool force_resize;
	bool block_resize;
	bool center_window;

	QPoint move_window_diff;
	QTimer move_window_timer;

	TAutoHideTimer* auto_hide_timer;


	void createToolbars();
	QMenu* createToolbarMenu();

	void restartSMPlayer(bool reset_style);

	void setFloatingToolbarsVisible(bool visible);
	double getNewSizeFactor();
	void hidePanel();
	void centerWindow();
	void resizeMainWindow(int w, int h, bool try_twice = true);

	void enterFullscreenOnPlay();
	void checkPendingActionsToRun();
	void retranslateStrings();

private slots:
	void moveWindowMerged();
};

} // namespace Gui

#endif // GUI_BASE_H

