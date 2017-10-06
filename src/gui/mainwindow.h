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
class TEditableToolbar;
class TTimeSliderAction;
class TVolumeSliderAction;
class TTimeLabelAction;

namespace Menu {

class TMenuFile;
class TMenuPlay;
class TMenuVideo;
class TMenuAudio;
class TMenuSubtitle;
class TMenuBrowse;

}
}

namespace Pref {
class TDialog;
}

namespace Playlist {
class TPlaylist;
}

class TPlayerWindow;
class TAutoHideTimer;
class TDockWidget;
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

    virtual void loadConfig();
    virtual void saveConfig();

    void openFiles(const QStringList& files, const QString& current = "");

    Playlist::TPlaylist* getPlaylist() const { return playlist; }
    Action::TActionList getAllNamedActions() const;
    QMenu* getToolbarMenu() const { return toolbar_menu; }

    //! Execute all the actions after the video has started to play
    void runActionsLater(const QString& actions, bool postCheck);

public slots:
    virtual void open(const QString& fileName); // Generic open, autodetect type.
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

    void setSize(double factor);
    void setSize(int percentage);
    void optimizeSizeFactor();

    void setForceCloseOnFinish(int n) { arg_close_on_finish = n; }
    int forceCloseOnFinish() { return arg_close_on_finish; }

    // Handle message from new intance send by TApp
    void handleMessageFromOtherInstances(const QString& message);

signals:
    void enableActions();

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

protected:
    Action::Menu::TMenuFile* fileMenu;
    Action::Menu::TMenuPlay* playMenu;
    Action::Menu::TMenuVideo* videoMenu;
    Action::Menu::TMenuAudio* audioMenu;
    Action::Menu::TMenuSubtitle* subtitleMenu;
    Action::Menu::TMenuBrowse* browseMenu;
    QMenu* windowMenu;

    virtual void closeEvent(QCloseEvent* e);
    virtual void hideEvent(QHideEvent* event);
    virtual void showEvent(QShowEvent* event);
    virtual void dragEnterEvent(QDragEnterEvent*);
    virtual void dropEvent(QDropEvent*);

#if defined(Q_OS_WIN)
    // Disable screensaver
    virtual bool winEvent(MSG* m, long* result);
#endif

    virtual QMenu* createPopupMenu();
    virtual void aboutToEnterFullscreen();
    virtual void didEnterFullscreen();
    virtual void aboutToExitFullscreen();
    virtual void didExitFullscreen();

    QMenu* createContextMenu();

protected slots:
    virtual void closeWindow();
    // Replace for setCaption (in Qt 4 it's not virtual)
    virtual void setWindowCaption(const QString& title);
    virtual void onMediaInfoChanged();

    void openRecent();
    void toggleDoubleSize();

private:
    QWidget* panel;
    TPlayerWindow* playerwindow;
    TDockWidget* playlistDock;
    Playlist::TPlaylist* playlist;
    TDockWidget* logDock;
    TLogWindow* log_window;

    Action::TAction* showContextMenuAct;
    Action::TAction* nextWheelFunctionAct;

    QMenu* helpMenu;
    QMenu* contextMenu;

    // Statusbar labels
    QLabel* video_info_label;
    QLabel* in_out_points_label;
    QLabel* time_label;

    // Cache timestamp
    QString positionText;
    QString durationText;

    // Visible state bars
    bool menubar_visible;
    bool statusbar_visible;
    bool fullscreen_menubar_visible;
    bool fullscreen_statusbar_visible;

    // Statusbar menu
    Action::TAction* viewVideoInfoAct;
    Action::TAction* viewInOutPointsAct;
    Action::TAction* viewVideoTimeAct;
    Action::TAction* viewFramesAct;
    QMenu* statusbar_menu;

    // Toolbars
    Action::TEditableToolbar* toolbar;
    Action::TEditableToolbar* toolbar2;
    Action::TEditableToolbar* controlbar;

    // Toolbar menu
    Action::TAction* viewMenuBarAct;
    Action::TAction* editToolbarAct;
    Action::TAction* editToolbar2Act;
    Action::TAction* editControlBarAct;
    Action::TAction* viewStatusBarAct;
    QMenu* toolbar_menu;

    // Slider actions for toolbars
    Action::TTimeSliderAction* timeslider_action;
    Action::TVolumeSliderAction* volumeslider_action;

    // Equalizers
    TVideoEqualizer* video_equalizer;
    TAudioEqualizer* audio_equalizer;

    // Windows
    TFilePropertiesDialog* file_properties_dialog;
    Pref::TDialog* pref_dialog;
    THelpWindow* help_window;

    QString first_fullscreen_filename;
    QString pending_actions_to_run;
    // Pass settings from command line
    int arg_close_on_finish; // -1 = not set, 1 = true, 0 = false

    bool ignore_show_hide_events;

    // Fiddel size and pos
    bool save_size;
    bool force_resize;
    bool center_window;
    QPoint center_window_pos;

    TAutoHideTimer* auto_hide_timer;
    TUpdateChecker* update_checker;

    static QString settingsGroupName();

    void createLogDock();
    void createPanel();
    void createPlayer();
    void createPlayerWindow();
    void createPlaylist();
    void createStatusBar();
    void createToolbars();
    QMenu* createToolbarMenu();
    void createActions();
    void createMenus();
    void createVideoEqualizer();
    void createAudioEqualizer();
    void createPreferencesDialog();
    void createFilePropertiesDialog();
    void setDataToFileProperties();
    void configureDiscDevices();
    void setupNetworkProxy();

    void processAction(QString action_name);
    void sendEnableActions();
    //! Execute all actions in \a actions. The actions should be
    //! separated by spaces. Checkable actions could have a parameter:
    //! true or false.
    void runActions(QString actions);

    void enterFullscreenOnPlay();
    void updateAudioEqualizer();

    void setFloatingToolbarsVisible(bool visible);
    void hidePanel();

    double optimizeSize(double size) const;
    double getDefaultSize() const;
    void resizeStickyWindow(int w, int h);
    void resizeMainWindow(int w, int h, double size_factor,
                          bool try_twice = true);

    void retranslateStrings();

    void save();
    void restartApplication();

private slots:
    void onPlayerError(int exit_code);

    // Mouse buttons
    void leftClickFunction();
    void rightClickFunction();
    void doubleClickFunction();
    void middleClickFunction();
    void xbutton1ClickFunction();
    void xbutton2ClickFunction();

    void showContextMenu();
    void showCustomContextMenu(const QPoint& pos);
    void showStatusBarPopup(const QPoint& pos);

    void displayVideoInfo();
    void displayInOutPoints();
    void displayFrames(bool);

    void applyFileProperties();
    void applyNewPreferences();

    void setDefaultValuesFromVideoEqualizer();
    void changeVideoEqualizerBySoftware(bool b);
    void updateVideoEqualizer();

    void exitFullscreenOnStop();
    void checkPendingActionsToRun();

    void onStateChanged(Player::TState state);
    void onDurationChanged(double duration);
    void onPositionChanged(double, bool changed = false);
    void onVideoOutResolutionChanged(int w, int h);
    void onNewMediaStartedPlaying();
    void onMediaSettingsChanged();
    void onPlaylistFinished();
    void onDragPositionChanged(double);
    void onPlaylistTitleChanged(QString title);
};

} // namespace Gui

#endif // GUI_MAINWINDOW_H

