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

#ifndef GUI_MAINWINDOW_H
#define GUI_MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include "wzdebug.h"
#include "gui/action/actionlist.h"
#include "config.h"
#include "player/state.h"


class QWidget;
class QLabel;
class QMenu;

namespace Settings {
class TMediaSettings;
}

namespace Gui {

namespace Action {
class TAction;
class TMenuFile;
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

namespace Playlist {
class TPlaylist;
}

class TPlayerWindow;
class TAutoHideTimer;
class TLogWindow;
class THelpWindow;
class TFilePropertiesDialog;
class TAudioEqualizer;
class TVideoEqualizer;
class TUpdateChecker;


class TMainWindow : public QMainWindow {
    Q_OBJECT
    DECLARE_QCLASS_LOGGER

public:
    TMainWindow();
    virtual ~TMainWindow();

    /* Return true if the window shouldn't show on startup */
    virtual bool startHidden() { return false; }

    //! Execute all actions in \a actions. The actions should be
    //! separated by spaces. Checkable actions could have a parameter:
    //! true or false.
    void runActions(QString actions);

    //! Execute all the actions after the video has started to play
    void runActionsLater(const QString& actions, bool postCheck);

    Playlist::TPlaylist* getPlaylist() { return playlist; }
    Action::TActionList getAllNamedActions();
    QMenu* getToolbarMenu() { return toolbar_menu; }

    virtual void loadConfig();
    virtual void saveConfig();

    void openFiles(const QStringList& files, const QString& current = "");

public slots:
    virtual void open(const QString& file); // Generic open, autodetect type.
    virtual void openFile();
    virtual void openURL();
    virtual void openVCD();
    virtual void openAudioCD();
    virtual void openDVD();
    virtual void openDVDFromFolder();
    virtual void openDVDFromFolder(const QString& directory);
    void openBluRay();
    void openBluRayFromFolder();
    virtual void openDirectory();

    virtual void showConfigFolder();

    virtual void helpCLOptions();
    virtual void helpCheckUpdates();
    virtual void helpAbout();

    virtual void loadSub();
    virtual void loadAudioFile(); // Load external audio file

    void setInitialSubtitle(const QString& subtitle_file);

    virtual void showPlaylist(bool b);
    virtual void showPreferencesDialog();
    virtual void showFilePropertiesDialog(bool checked);

    virtual void showSeekToDialog();
    virtual void showSubDelayDialog();
    virtual void showAudioDelayDialog();
    virtual void showStereo3dDialog();

    virtual void exitFullscreen();
    virtual void toggleFullscreen();
    virtual void toggleFullscreen(bool);

    void setStayOnTop(bool b);
    virtual void changeStayOnTop(int);
    virtual void checkStayOnTop(Player::TState);
    void toggleStayOnTop();

    void changeSize(double factor);
    void changeSize(int percentage);
    void optimizeSizeFactor();

    void setForceCloseOnFinish(int n) { arg_close_on_finish = n; }
    int forceCloseOnFinish() { return arg_close_on_finish; }

signals:
    void enableActions();

    void timeChanged(QString time_ready_to_print);

    void preferencesChanged();
    void mediaSettingsChanged(Settings::TMediaSettings* mset);
    void mediaFileTitleChanged(const QString& filename, const QString& title);

    void fullscreenChanged();
    void aboutToEnterFullscreenSignal();
    void didEnterFullscreenSignal();
    void didExitFullscreenSignal();

    void stayOnTopChanged(int);

    //! Sent when another instance requested to play a file
    void openFileRequested();
    void requestRestart();

protected slots:
    virtual void closeWindow();

    // Replace for setCaption (in Qt 4 it's not virtual)
    virtual void setWindowCaption(const QString& title);

    virtual void onStateChanged(Player::TState state);

    virtual void onPositionChanged(double, bool changed = false);
    virtual void onDurationChanged(double duration);

    virtual void onMediaSettingsChanged();
    virtual void onVideoOutResolutionChanged(int w, int h);

    virtual void onNewMediaStartedPlaying();
    virtual void onMediaInfoChanged();

    virtual void updateVideoEqualizer();
    virtual void updateAudioEqualizer();
    virtual void setDefaultValuesFromVideoEqualizer();
    virtual void changeVideoEqualizerBySoftware(bool b);

    virtual void openRecent();
    virtual void exitFullscreenOnStop();
    virtual void playlistHasFinished();

    void toggleDoubleSize();

    virtual void resizeWindow(int w, int h);
    void resizeWindowToVideo();

    virtual void onDragPositionChanged(double);

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
    virtual void processAction(QString action_name);

    virtual void dragEnterEvent(QDragEnterEvent*);
    virtual void dropEvent(QDropEvent*);

    virtual void applyNewPreferences();
    virtual void applyFileProperties();

    // Single instance stuff
    void handleMessageFromOtherInstances(const QString& message);

    void onPlayerError(int exit_code);

protected:
    virtual void closeEvent(QCloseEvent* e);
    virtual void changeEvent(QEvent* event);
    virtual void hideEvent(QHideEvent* event);
    virtual void showEvent(QShowEvent* event);

#if defined(Q_OS_WIN)
    // Disable screensaver
    virtual bool winEvent(MSG* m, long* result);
#endif

    virtual QMenu* createPopupMenu();
    virtual void aboutToEnterFullscreen();
    virtual void didEnterFullscreen();
    virtual void aboutToExitFullscreen();
    virtual void didExitFullscreen();

    void createPlayer();
    void createPlayerWindow();
    void createVideoEqualizer();
    void createAudioEqualizer();
    void createPlaylist();
    void createPanel();
    void createPreferencesDialog();
    void createFilePropertiesDialog();
    void setDataToFileProperties();
    void createActions();
    void createMenus();
    void configureDiscDevices();
    void setupNetworkProxy();
    double getNewSizeFactor();

protected:
    QWidget* panel;
    TPlayerWindow* playerwindow;

    Action::TAction* showContextMenuAct;
    Action::TAction* nextWheelFunctionAct;

    // MENUS
    Action::TMenuFile* fileMenu;
    Action::TMenuPlay* playMenu;
    Action::TMenuVideo* videoMenu;
    Action::TMenuAudio* audioMenu;
    Action::TMenuSubtitle* subtitleMenu;
    Action::TMenuBrowse* browseMenu;
    QMenu* windowMenu;
    QMenu* helpMenu;

    QMenu* popup;

    // Toolbar menu
    Action::TAction* viewMenuBarAct;
    Action::TAction* editToolbarAct;
    Action::TAction* editToolbar2Act;
    Action::TAction* editControlBarAct;
    Action::TAction* viewStatusBarAct;

    QMenu* toolbar_menu;

    Action::TEditableToolbar* toolbar;
    Action::TEditableToolbar* toolbar2;
    Action::TEditableToolbar* controlbar;

    Action::TTimeSliderAction* timeslider_action;
    Action::TVolumeSliderAction* volumeslider_action;

    Playlist::TPlaylist* playlist;
    TLogWindow* log_window;
    THelpWindow* help_window;

    Pref::TDialog* pref_dialog;
    TFilePropertiesDialog* file_properties_dialog;
    TVideoEqualizer* video_equalizer;
    TAudioEqualizer* audio_equalizer;

    QString pending_actions_to_run;

    bool state_restored;
    bool switching_to_fullscreen;

private:
    QLabel* video_info_label;
    QLabel* in_out_points_label;
    QLabel* time_label;

    Action::TAction* viewVideoInfoAct;
    Action::TAction* viewInOutPointsAct;
    Action::TAction* viewVideoTimeAct;
    Action::TAction* viewFramesAct;

    QMenu* statusbar_menu;

    QString positionText;
    QString durationText;

    bool menubar_visible;
    bool statusbar_visible;
    bool fullscreen_menubar_visible;
    bool fullscreen_statusbar_visible;

    // Force settings from command line
    int arg_close_on_finish; // -1 = not set, 1 = true, 0 = false

    bool was_maximized;

    bool ignore_show_hide_events;

    bool save_size;
    bool force_resize;
    bool center_window;
    QPoint center_window_pos;

    QPoint move_window_diff;
    QTimer move_window_timer;

    TAutoHideTimer* auto_hide_timer;
    TUpdateChecker* update_checker;

    void createStatusBar();
    void createToolbars();
    QMenu* createToolbarMenu();

    static QString settingsGroupName();

    void sendEnableActions();

    void save();
    void restartApplication();

    void setFloatingToolbarsVisible(bool visible);
    void hidePanel();
    bool optimizeSizeFactorPreDef(int factor, int predef_factor);
    void getNewGeometry(int w, int h);
    void resizeMainWindow(int w, int h, double size_factor, bool try_twice = true);

    void enterFullscreenOnPlay();
    void retranslateStrings();

private slots:
    void displayVideoInfo();
    void displayInOutPoints();
    void displayFrames(bool);

    void moveWindowMerged();
    void checkPendingActionsToRun();
};

} // namespace Gui

#endif // GUI_MAINWINDOW_H

