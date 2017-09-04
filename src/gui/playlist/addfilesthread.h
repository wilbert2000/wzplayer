#ifndef GUI_PLAYLIST_ADDFILESTHREAD
#define GUI_PLAYLIST_ADDFILESTHREAD

#include <QThread>
#include <QStringList>
#include <QString>
#include <QDir>

#include "wzdebug.h"


class QFileInfo;

namespace Gui {
namespace Playlist {

class TPlaylistWidgetItem;


class TAddFilesThread : public QThread {
    Q_OBJECT
    DECLARE_QCLASS_LOGGER

public:
    TAddFilesThread(QObject* parent,
                    const QStringList& aFiles,
                    const QStringList& nameBlacklist,
                    bool recurseSubDirs,
                    bool videoFiles,
                    bool audioFiles,
                    bool playlists,
                    bool images);
    virtual ~TAddFilesThread();

    virtual void run();
    void abort() { abortRequested = true; stopRequested = true; }
    void stop() { stopRequested = true; }

    // Inputs
    const QStringList& files;

    // Outputs
    TPlaylistWidgetItem* root;
    QString latestDir;

signals:
    void displayMessage(const QString&, int);

private:
    bool abortRequested;
    bool stopRequested;
    bool recurse;
    bool addImages;

    QString playlistPath;

    QStringList lockedFiles;
    QStringList nameFilterList;
    QList<QRegExp> rxNameBlacklist;

    bool nameBlackListed(const QString& name);

    static QDir::SortFlags getSortFlags();

    TPlaylistWidgetItem* addFile(TPlaylistWidgetItem* parent,
                                 const QFileInfo& fi);

    TPlaylistWidgetItem* addDirectory(TPlaylistWidgetItem* parent,
                                      QFileInfo& fi,
                                      QString name,
                                      bool protectName);

    TPlaylistWidgetItem* createPath(TPlaylistWidgetItem* parent,
                                    const QFileInfo& fi,
                                    const QString& name,
                                    double duration,
                                    bool protectName);

    void addNewItems(TPlaylistWidgetItem* playlistItem);

    bool openM3u(TPlaylistWidgetItem* playlistItem,
                 const QString& fileName);
    bool openPls(TPlaylistWidgetItem* playlistItem,
                 const QString& playlistFileName);
    TPlaylistWidgetItem* openPlaylist(TPlaylistWidgetItem* parent,
                                      const QFileInfo& fi,
                                      const QString& name,
                                      bool protectName);

    TPlaylistWidgetItem* addItemNotFound(TPlaylistWidgetItem* parent,
                                         const QString& filename,
                                         QString name,
                                         bool protectName);

    TPlaylistWidgetItem* addItem(TPlaylistWidgetItem* parent,
                                 QString filename,
                                 QString name = "",
                                 double duration = 0);

    void addFiles();
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_ADDFILESTHREAD
