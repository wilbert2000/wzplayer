#ifndef GUI_PLAYLIST_PLIST_H
#define GUI_PLAYLIST_PLIST_H

#include "gui/action/menu/menu.h"
#include <QWidget>


class QToolBar;
class QToolButton;
class QTreeWidgetItem;
class QTextStream;

namespace Gui {

class TMainWindow;
class TDockWidget;

namespace Action {
class TAction;
class TEditableToolbar;
}

namespace Playlist {

class TAddFilesThread;
class TPlaylistItem;
class TPlaylistWidget;
class TMenuAddRemoved;

class TPList : public QWidget {
    Q_OBJECT
    friend class TMenuAddRemoved;
public:
    explicit TPList(TDockWidget* parent,
                    const QString& name,
                    const QString& aShortName,
                    const QString& aTransName);
    virtual ~TPList() override;

    bool hasPlayableItems() const;
    TPlaylistWidget* getPlaylistWidget() const { return playlistWidget; }

    void abortThread();
    void add(const QStringList& files,
             bool startPlay = false,
             TPlaylistItem* target = 0,
             const QString& fileToPlay = "");
    bool isBusy() const;

    bool maybeSave();
    void setContextMenuToolbar(Action::Menu::TMenu* menu);

    virtual void loadSettings();
    virtual void saveSettings();

public slots:
    virtual void enableActions();

    virtual void stop();
    void play();
    void playNext(bool loop_playlist = true);
    void playPrev();
    virtual bool findPlayingItem();

signals:
    void addedItems();
    void busyChanged();

protected:
    TDockWidget* dock;
    TPlaylistWidget* playlistWidget;
    Action::TEditableToolbar* toolbar;
    QString playlistFilename;
    TAddFilesThread* thread;
    int disableEnableActions;
    bool reachedEndOfPlaylist;

    QAction* openAct;
    Action::TAction* saveAct;
    Action::TAction* saveAsAct;
    Action::TAction* refreshAct;
    Action::TAction* browseDirAct;

    Action::TAction* playAct;
    Action::TAction* playInNewWindowAct;
    Action::TAction* findPlayingAct;
    QAction* repeatAct;
    QAction* shuffleAct;

    Action::TAction* editNameAct;
    Action::TAction* resetNameAct;
    Action::TAction* editURLAct;
    Action::TAction* newFolderAct;

    Action::TAction* cutAct;
    Action::TAction* copyAct;
    Action::TAction* pasteAct;

    Action::Menu::TMenu* playlistAddMenu;
    Action::TAction* addPlayingFileAct;

    Action::Menu::TMenu* playlistRemoveMenu;
    Action::TAction* removeSelectedAct;
    Action::TAction* removeSelectedFromDiskAct;
    Action::TAction* removeAllAct;

    virtual void clear(bool clearFilename = true);
    virtual void playItem(TPlaylistItem* item, bool keepPaused = false) = 0;
    void playEx();
    void openPlaylist(const QString& filename);
    void makeActive();
    void setPlaylistFilename(const QString& filename);

protected slots:
    virtual void openPlaylistDialog();
    bool save(bool allowFail);
    virtual bool saveAs();
    virtual void refresh() = 0;
    virtual void removeAll();

    void startPlay();
    void setPLaylistTitle();

private:
    QString shortName;
    QString tranName;

    QToolButton* add_button;
    QToolButton* remove_button;

    QStringList addFiles;
    TPlaylistItem* addTarget;
    QString addFileToPlay;
    bool addStartPlay;
    bool restartThread;

    bool isFavList;
    bool skipRemainingMessages;

    void createTree();
    void createActions();
    void createToolbar();

    TPlaylistItem* getRandomItem() const;

    void enableRemoveFromDiskAction();
    void enableActionsCurrentItem();

    QUrl getBrowseURL();
    void copySelection(const QString& actionName);

    void addStartThread();

    bool saveM3uFolder(TPlaylistItem* folder,
                       const QString& path,
                       QTextStream& stream,
                       bool linkFolders,
                       bool allowFail,
                       bool& savedMetaData);
    bool saveM3uFile(TPlaylistItem* folder,
                     bool linkFolders,
                     bool allowFail);
    bool saveM3u(bool allowFail);

private slots:
    void playInNewWindow();
    void editName();
    void resetName();
    void editURL();
    void newFolder();

    void cut();
    void copySelected();
    void enablePaste();
    void paste();

    void addPlayingFile();
    void addFilesDialog();
    void addDirectoryDialog();
    void addUrlsDialog();

    void removeSelected(bool deleteFromDisk = false);
    void removeSelectedFromDisk();

    void enableRemoveMenu();

    void browseDir();

    void onCurrentItemChanged(QTreeWidgetItem* current,
                              QTreeWidgetItem* previous);
    void onItemActivated(QTreeWidgetItem* i, int column);
    void onThreadFinished();
    void onPlaylistWidgetBusyChanged();
};

class TMenuAddRemoved : public Action::Menu::TMenu {
    Q_OBJECT
public:
    explicit TMenuAddRemoved(TPList* pl, const QString& name);
private:
    TPList* plist;
    TPlaylistItem* parentItem;

private slots:
    void onAboutToShow();
    void onCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem*);
    void onTriggered(QAction* action);
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_PLIST_H
