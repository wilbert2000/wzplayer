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

#include "gui/action/menu/menu.h"
#include "settings/mediasettings.h"
#include "player/state.h"
#include "config.h"
#include "wzdebug.h"

#include <QMainWindow>


class QWidget;
class QLabel;
class QMenu;
class TWZTimer;

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
class TMenuView;

}
}

namespace Pref {
class TDialog;
}

namespace Playlist {
class TPlaylist;
class TFavList;
}

class TPlayerWindow;
class TAutoHideTimer;
class TDockWidget;
class TLogWindow;
class THelpWindow;
class TPropertiesDialog;
class TAudioEqualizer;
class TVideoEqualizer;
class TUpdateChecker;
class TTimeLabel;


class TMainWindow : public QMainWindow {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER
public:
    TWZTimer* optimizeSizeTimer;

    TMainWindow();
    virtual ~TMainWindow() override;

    virtual void loadSettings();
    virtual void saveSettings();

    Playlist::TPlaylist* getPlaylist() const { return playlist; }
    QAction* requireAction(const QString& name);

    bool dockNeedsResize(TDockWidget* dock, Qt::DockWidgetArea area) const;

    Action::TAction* seekIntToAction(int i) const;

    //! Execute actions after the video has started to play
    void runActionsLater(const QString& actions,
                         bool postCheck,
                         bool prepend = false);

public slots:
    void openRecent();
    virtual void closeWindow();

    void stop();
    void updateSeekDefaultAction(QAction* action);

    void exitFullscreen();
    void setFullscreen(bool);
    void toggleFullscreen();

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
    void browseConfigFolder();
    void browseDataFolder();
    void showSettingsDialog();

    void helpCLOptions();
    void helpCheckUpdates();
    void helpAbout();

    void wheelUpSeeking();
    void wheelDownSeeking();

    void setForceCloseOnFinish(int n) { optionCloseOnFinish = n; }
    int forceCloseOnFinish() { return optionCloseOnFinish; }

    // Handle message from new intance send by TApp
    virtual void onReceivedMessage(const QString& msg);

    void save();

signals:
    void settingsChanged();
    void requestRestart();
    void resizedMainWindow();

    void fullscreenChanged();
    void aboutToEnterFullscreenSignal();
    void didEnterFullscreenSignal();
    void didExitFullscreenSignal();

    void setWindowSizeToolTip(QString tip);

    void videoTrackGroupChanged(Action::TActionGroup* group);
    void audioTrackGroupChanged(Action::TActionGroup* group);
    void subtitleTrackGroupsChanged(Action::TActionGroup* subGroup,
                                    Action::TActionGroup* secSubGroup);

    void stayOnTopChanged(int);

    void seekForwardDefaultActionChanged(QAction* action);
    void seekRewindDefaultActionChanged(QAction* action);

protected:
    Action::TAction* playPauseStopAct;
    Action::TAction* closeAct;
    Action::TAction* quitAct;
    Action::Menu::TMenuView* viewMenu;

    Action::Menu::TMenu* createContextMenu(const QString& name,
                                           const QString& text);

    virtual Action::Menu::TMenu* createPopupMenu() override;
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

private:
    TPlayerWindow* playerWindow;
    TPlayerWindow* previewWindow;

    TDockWidget* logDock;
    TLogWindow* logWindow;
    TDockWidget* playlistDock;
    Playlist::TPlaylist* playlist;
    TDockWidget* favListDock;
    Playlist::TFavList* favlist;

    TAutoHideTimer* autoHideTimer;
    TWZTimer* titleUpdateTimer;
    TUpdateChecker* update_checker;

    Action::TAction* readyAction;
    QString pending_actions;
    // Pass settings from command line
    int optionCloseOnFinish; // -1 = not set, 1 = true, 0 = false

    bool ignore_show_hide_events;
    QString first_fullscreen_filename;

    // Fiddle width size and pos
    bool save_size;
    bool force_resize;
    bool center_window;
    QPoint center_window_pos;

    // Windows
    TPropertiesDialog* propertiesDialog;
    Pref::TDialog* prefDialog;
    THelpWindow* help_window;

    // Equalizers
    TVideoEqualizer* video_equalizer;
    TAudioEqualizer* audio_equalizer;

    // Slider actions for toolbars
    Action::TTimeSliderAction* timeslider_action;
    Action::TVolumeSliderAction* volumeslider_action;

    // Menu bar
    Action::Menu::TMenuFile* fileMenu;
    Action::Menu::TMenuPlay* playMenu;
    Action::Menu::TMenuVideo* videoMenu;
    Action::Menu::TMenuAudio* audioMenu;
    Action::Menu::TMenuSubtitle* subtitleMenu;
    Action::Menu::TMenuBrowse* browseMenu;
    // Action::Menu::TMenuView* viewMenu; protected

    // File menu
    Action::TAction* clearRecentsAct;

#ifdef Q_OS_LINUX
    Action::TAction* saveThumbnailAct;
#endif

    // closeAct is protected
    // quitAct is protected

    // Play menu
    Action::TAction* stopAct;
    Action::TAction* playAct;
    Action::TAction* pauseAct;
    Action::TAction* playPauseAct;
    // Action::TAction* playPauseStopAct; protected

    Action::TAction* playNextAct;
    Action::TAction* playPrevAct;

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

    // Video menu
    Action::TAction* fullscreenAct;

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


    // Browse menu
    Action::TActionGroup* titleGroup;
    Action::TAction* prevChapterAct;
    Action::TAction* nextChapterAct;
    Action::TActionGroup* chapterGroup;
    Action::TAction* nextAngleAct;
    Action::TActionGroup* angleGroup;

    // DVDNAV
    Action::TAction* dvdnavUpAct;
    Action::TAction* dvdnavDownAct;
    Action::TAction* dvdnavLeftAct;
    Action::TAction* dvdnavRightAct;

    Action::TAction* dvdnavMenuAct;
    Action::TAction* dvdnavPrevAct;
    Action::TAction* dvdnavSelectAct;
    Action::TAction* dvdnavMouseAct;


    // View menu
    // OSD
    Action::TActionGroup* osdGroup;
    Action::TAction* osdShowFilenameAct;
    Action::TAction* osdShowTimeAct;

    // View properties
    Action::TAction* viewPropertiesAct;
    Action::Menu::TMenu* toolbarMenu;
    Action::Menu::TMenu* editToolbarMenu;


    // Help menu
    QMenu* helpMenu;
    Action::Menu::TMenu* contextMenu;

    // Statusbar labels
    QLabel* video_info_label;
    QLabel* in_out_points_label;
    TTimeLabel* timeLabel;

    // Visible state bars
    bool menubar_visible;
    bool statusbar_visible;
    bool fullscreen_menubar_visible;
    bool fullscreen_statusbar_visible;

    // Statusbar menu
    Action::TAction* viewVideoInfoAct;
    Action::TAction* viewInOutPointsAct;
    Action::TAction* viewVideoTimeAct;
    Action::TActionGroup* timeResGroup;
    QMenu* statusbarMenu;

    // Toolbars
    Action::TEditableToolbar* toolbar;
    Action::TEditableToolbar* toolbar2;
    Action::TEditableToolbar* controlbar;

    // Toolbar menu
    Action::TAction* viewMenuBarAct;
    Action::TAction* viewStatusBarAct;


    void createStatusBar();
    void createLogDock();
    void createPlayerWindows();
    void createPlayers();
    void createPlaylist();
    void createFavList();
    void createVideoEqualizer();
    void createAudioEqualizer();
    void createActions();
    void createToolbars();
    void createMenus();
    void setupNetworkProxy();

    void createSettingsDialog();
    void setFilePropertiesData();
    void createFilePropertiesDialog();
    void configureDiscDevices();

    void checkActionValid(QString& action, const QString& def);
    QList<QAction*> findNamedActions() const;
    void processAction(QString action_name);
    void postAction(const QString& actionName, bool hasArg, bool arg);

    void enableSubtitleActions();
    void enableActions();

    bool haveDockedDocks() const;
    void hidePlayerWindow();
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
    void checkPendingActions();
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

    void play();
    void playPauseStop();
    void showSeekToDialog();

    void enablePlayPauseStop();
    void updateVideoTracks();
    void updateAudioTracks();
    void updateSubtitleTracks();
    void updateTitle();

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

    void wheelUpFunc(Settings::TPreferences::TWheelFunction function);
    void wheelUp();
    void wheelDownFunc(Settings::TPreferences::TWheelFunction function);
    void wheelDown();
    void nextWheelFunction();

    // Status bar
    void displayVideoOut();
    void displayInOutPoints();

    void onStateChanged(Player::TState state);
    void onVideoOutResolutionChanged(int w, int h);
    void onNewMediaStartedPlaying();
    void onMediaStartedPlaying();
    void onMediaSettingsChanged();
    void onPlaylistFinished();
};

extern TMainWindow* mainWindow;

} // namespace Gui

#endif // GUI_MAINWINDOW_H

