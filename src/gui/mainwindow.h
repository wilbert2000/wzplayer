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
class TActionGroup;
class TEditableToolbar;
class TTimeSliderAction;
class TVolumeSliderAction;
class TTimeLabelAction;

namespace Menu {

class TMenuExec;
class TMenu;
class TMenuFile;
class TMenuPlay;
class TWindowSizeGroup;
class TZoomAndPanGroup;
class TColorSpaceGroup;
class TDeinterlaceGroup;
class TRotateGroup;
class TFilterGroup;
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

    virtual void loadSettings();
    virtual void saveSettings();

    Playlist::TPlaylist* getPlaylist() const { return playlist; }
    QList<QAction*> getNamedActions() const { return allActions; }
    QAction* findAction(const QString& name);

    Action::TAction* seekIntToAction(int i) const;

    //! Execute all the actions after the video has started to play
    void runActionsLater(const QString& actions, bool postCheck);

public slots:
    void openRecent();
    virtual void closeWindow();

    void updateSeekDefaultAction(QAction* action);

    void exitFullscreen();
    void setFullscreen(bool);
    void toggleFullscreen();

    void updateAspectMenu();
    void updateWindowSizeMenu();
    void updateTransformMenu();
    void updateFilters();

    void setSizeFactor(double factor);
    void setSizePercentage(int percentage);
    void toggleDoubleSize();
    void optimizeSizeFactor();

    void showStereo3dDialog();

    void showAudioDelayDialog();
    void loadAudioFile(); // Load external audio file

    void showSubDelayDialog();
    void loadSub();

    void setStayOnTop(bool b);
    void changeStayOnTop(int);
    void checkStayOnTop(Player::TState);
    void toggleStayOnTop();

    void showFilePropertiesDialog(bool checked);
    void showConfigFolder();
    void showSettingsDialog();

    void helpCLOptions();
    void helpCheckUpdates();
    void helpAbout();

    void setForceCloseOnFinish(int n) { arg_close_on_finish = n; }
    int forceCloseOnFinish() { return arg_close_on_finish; }

    // Handle message from new intance send by TApp
    void handleMessageFromOtherInstances(const QString& message);

    void save();

signals:
    void enableActions();
    void settingsChanged();
    void mediaSettingsChanged(Settings::TMediaSettings* mset);
    void gotMessageFromOtherInstance();
    void requestRestart();

    void fullscreenChanged();
    void aboutToEnterFullscreenSignal();
    void didEnterFullscreenSignal();
    void didExitFullscreenSignal();

    void setAspectToolTip(QString tip);
    void setWindowSizeToolTip(QString tip);

    void videoTrackGroupChanged(Action::TActionGroup* group);
    void audioTrackGroupChanged(Action::TActionGroup* group);
    void subtitleTrackGroupsChanged(Action::TActionGroup* subGroup,
                                    Action::TActionGroup* secSubGroup);

    void stayOnTopChanged(int);

    void seekForwardDefaultActionChanged(QAction* action);
    void seekRewindDefaultActionChanged(QAction* action);

protected:
    Playlist::TPlaylist* playlist;
    Action::Menu::TMenuFile* fileMenu;
    Action::Menu::TMenuPlay* playMenu;
    Action::Menu::TMenuVideo* videoMenu;
    Action::Menu::TMenuAudio* audioMenu;
    Action::Menu::TMenuSubtitle* subtitleMenu;
    Action::Menu::TMenuBrowse* browseMenu;
    QMenu* viewMenu;
    Action::Menu::TMenuExec* createContextMenu();

    virtual QMenu* createPopupMenu() override;
    virtual void closeEvent(QCloseEvent* e) override;
    virtual void hideEvent(QHideEvent* event) override;
    virtual void showEvent(QShowEvent* event) override;
    virtual void dragEnterEvent(QDragEnterEvent*) override;
    virtual void dropEvent(QDropEvent*) override;

#if defined(Q_OS_WIN)
    // Disable screensaver
    virtual bool winEvent(MSG* m, long* result);
#endif

protected slots:
    // Replacement for setCaption (in Qt 4 it's not virtual)
    virtual void setWindowCaption(const QString& title);
    virtual void onMediaInfoChanged();

private:
    static QString settingsGroupName() { return "mainwindow"; }

    QWidget* panel;
    TPlayerWindow* playerWindow;
    TDockWidget* playlistDock;
    TDockWidget* logDock;
    TLogWindow* logWindow;

    TAutoHideTimer* autoHideTimer;
    TUpdateChecker* update_checker;

    QList<QAction*> allActions;
    QString pending_actions_to_run;
    // Pass settings from command line
    int arg_close_on_finish; // -1 = not set, 1 = true, 0 = false

    bool ignore_show_hide_events;
    QString first_fullscreen_filename;

    // Fiddel size and pos
    bool save_size;
    bool force_resize;
    bool center_window;
    QPoint center_window_pos;

    // Windows
    TFilePropertiesDialog* file_properties_dialog;
    Pref::TDialog* prefDialog;
    THelpWindow* help_window;

    // Equalizers
    TVideoEqualizer* video_equalizer;
    TAudioEqualizer* audio_equalizer;

    // Slider actions for toolbars
    Action::TTimeSliderAction* timeslider_action;
    Action::TVolumeSliderAction* volumeslider_action;

    // File menu
    Action::TAction* clearRecentsAct;
#ifdef Q_OS_LINUX
    Action::TAction* saveThumbnailAct;
#endif

    // Play menu
    // Seek forward menu
    Action::TAction* seekFrameAct;
    Action::TAction* seek1Act;
    Action::TAction* seek2Act;
    Action::TAction* seek3Act;

    // Seek backwards menu
    Action::TAction* seekBackFrameAct;
    Action::TAction* seekBack1Act;
    Action::TAction* seekBack2Act;
    Action::TAction* seekBack3Act;

    Action::TAction* seekToTimeAct;

    // Play speed menu
    QActionGroup* playSpeedGroup;
    // In-out menu
    QActionGroup* inOutGroup;
    Action::TAction* repeatInOutAct;

    // Video menu
    Action::TAction* fullscreenAct;

    // Aspect menu
    Action::TActionGroup* aspectGroup;
    Action::TAction* aspectAutoAct;
    Action::TAction* aspectDisabledAct;
    Action::TAction* nextAspectAct;

    // Window size menu
    Action::Menu::TWindowSizeGroup* windowSizeGroup;
    Action::TAction* doubleSizeAct;
    Action::TAction* optimizeSizeAct;
    Action::TAction* resizeOnLoadAct;

    // Zoom and pan
    Action::Menu::TZoomAndPanGroup* zoomAndPanGroup;

    // Video equalizer
    Action::TAction* equalizerAct;
    Action::TAction* resetVideoEqualizerAct;

    Action::TAction* decContrastAct;
    Action::TAction* incContrastAct;
    Action::TAction* decBrightnessAct;
    Action::TAction* incBrightnessAct;
    Action::TAction* decHueAct;
    Action::TAction* incHueAct;
    Action::TAction* decSaturationAct;
    Action::TAction* incSaturationAct;
    Action::TAction* decGammaAct;
    Action::TAction* incGammaAct;

    // Color space
    Action::Menu::TColorSpaceGroup* colorSpaceGroup;

    // Deinterlace
    Action::Menu::TDeinterlaceGroup* deinterlaceGroup;
    Action::TAction* toggleDeinterlaceAct;

    // Transform
    Action::TAction* flipAct;
    Action::TAction* mirrorAct;
    Action::Menu::TRotateGroup* rotateGroup;

    // Video filters
    Action::Menu::TFilterGroup* filterGroup;
    Action::TActionGroup* denoiseGroup;
    Action::TActionGroup* sharpenGroup;

    // Stereo 3D
    Action::TAction* stereo3DAct;

    // Video tracks
    Action::TAction* nextVideoTrackAct;
    Action::TActionGroup* videoTrackGroup;

    // Screen shots
    Action::TAction* screenshotAct;
    Action::TAction* screenshotsAct;
    Action::TAction* capturingAct;

    // Audio menu
    // Volume
    Action::TAction* muteAct;
    Action::TAction* decVolumeAct;
    Action::TAction* incVolumeAct;

    // Delay
    Action::TAction* decAudioDelayAct;
    Action::TAction* incAudioDelayAct;
    Action::TAction* audioDelayAct;

    // Equalizer
    Action::TAction* audioEqualizerAct;
    Action::TAction* resetAudioEqualizerAct;

    // Stereo
    Action::TActionGroup* stereoGroup;

    // Channels
    Action::TActionGroup* audioChannelGroup;

    // Audio filters
    Action::TAction* volnormAct;
    Action::TAction* extrastereoAct;
    Action::TAction* karaokeAct;

    // Audio tracks
    Action::TAction* nextAudioTrackAct;
    Action::TActionGroup* audioTrackGroup;

    // External audio
    Action::TAction* loadAudioAct;
    Action::TAction* unloadAudioAct;


    // Subtitles
    Action::TAction* decSubPosAct;
    Action::TAction* incSubPosAct;
    Action::TAction* decSubScaleAct;
    Action::TAction* incSubScaleAct;

    Action::TAction* decSubDelayAct;
    Action::TAction* incSubDelayAct;
    Action::TAction* subDelayAct;

    Action::TAction* incSubStepAct;
    Action::TAction* decSubStepAct;

    Action::TAction* seekNextSubAct;
    Action::TAction* seekPrevSubAct;

    Action::TAction* nextSubtitleAct;
    Action::TActionGroup* subtitleTrackGroup;
    Action::TActionGroup* secondarySubtitleTrackGroup;

    Action::TActionGroup* closedCaptionsGroup;
    Action::TAction* useForcedSubsOnlyAct;

    Action::TAction* loadSubsAct;
    Action::TAction* unloadSubsAct;
    Action::TActionGroup* subFPSGroup;
    Action::TAction* useCustomSubStyleAct;


    // Help menu
    QMenu* helpMenu;
    Action::Menu::TMenuExec* contextMenu;

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


    void createLogDock();
    void createPanel();
    void createPlayer();
    void createPlayerWindow();
    void createPlaylist();
    void createStatusBar();
    void createToolbars();
    Gui::Action::Menu::TMenu* createToolbarMenu(const QString& name);
    void createActions();
    void createMenus();
    void createVideoEqualizer();
    void createAudioEqualizer();
    void createSettingsDialog();
    void createFilePropertiesDialog();
    void setDataToFileProperties();
    void setupNetworkProxy();
    void configureDiscDevices();
    void setTimeLabel(double sec, bool changed);

    QList<QAction*> findNamedActions() const;
    void processAction(QString action_name);
    //! Execute all actions in \a actions. The actions should be
    //! separated by spaces. Checkable actions could have a parameter:
    //! true or false.
    void runActions(QString actions);

    void enableSubtitleActions();
    void setEnableActions();

    void hidePanel();
    void setFloatingToolbarsVisible(bool visible);

    void removeThumbnail(QString fn);
    void saveThumbnailToIni(const QString& fn, const QString& time);

    QString timeForJumps(int secs, const QString& seekSign) const;
    void setSeekTexts();

    void aboutToEnterFullscreen();
    void didEnterFullscreen();
    void aboutToExitFullscreen();
    void didExitFullscreen();
    void enterFullscreenOnPlay();

    double optimizeSize(double size) const;
    double getDefaultSize() const;
    void resizeStickyWindow(int w, int h);
    void resizeMainWindow(int w, int h, double size_factor,
                          bool try_twice = true);
    void onResizeOnLoadTriggered(bool b);

    void updateAudioEqualizer();

    void restartApplication();

private slots:
    void onPlayerError(int exit_code);
    void checkPendingActionsToRun();
    void showContextMenu();

    void clearRecentsListDialog();
    void openURL();
    void openDVD();
    void openDVDFromISO();
    void openDVDFromFolder();
    void openBluRay();
    void openBluRayFromISO();
    void openBluRayFromFolder();
    void openVCD();
    void openAudioCD();
    void saveThumbnail();

    void showSeekToDialog();
    void updateInOutMenu();

    void updateVideoTracks();
    void updateAudioTracks();
    void updateSubtitleTracks();

    void startStopScreenshots();
    void startStopCapture();

    void setDefaultValuesFromVideoEqualizer();
    void changeVideoEqualizerBySoftware(bool b);
    void updateVideoEqualizer();

    void applyFileProperties();
    void applyNewSettings();

    // Mouse buttons
    void leftClickFunction();
    void rightClickFunction();
    void doubleClickFunction();
    void middleClickFunction();
    void xbutton1ClickFunction();
    void xbutton2ClickFunction();

    // Status bar
    void displayVideoInfo();
    void displayInOutPoints();
    void displayFrames(bool);

    void onStateChanged(Player::TState state);
    void onDurationChanged(double duration);
    void onPositionChanged(double);
    void onVideoOutResolutionChanged(int w, int h);
    void onNewMediaStartedPlaying();
    void onMediaSettingsChanged();
    void onPlaylistFinished();
    void onDragPositionChanged(double);
    void onPlaylistTitleChanged(QString title);
};

} // namespace Gui

#endif // GUI_MAINWINDOW_H

