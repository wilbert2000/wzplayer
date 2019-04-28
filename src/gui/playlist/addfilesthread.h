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

class TPlaylistItem;


class TAddFilesThread : public QThread {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    explicit TAddFilesThread(QObject* parent,
                             const QStringList& aFiles,
                             const QStringList& nameBlacklist,
                             bool recurseSubDirs,
                             bool videoFiles,
                             bool audioFiles,
                             bool playlists,
                             bool images,
                             bool favList);
    virtual ~TAddFilesThread() override;

    virtual void run();
    void abort() { abortRequested = true; stopRequested = true; }
    void stop() { stopRequested = true; }

    // Inputs
    const QStringList& files;

    // Outputs
    TPlaylistItem* root;
    QString latestDir;

signals:
    void displayMessage(const QString&, int);

private:
    bool abortRequested;
    bool stopRequested;
    bool recurse;
    bool addImages;
    bool isFavList;

    QString playlistPath;

    QStringList lockedFiles;
    QStringList nameFilterList;
    QVector<QRegExp> rxNameBlacklist;

    bool nameBlackListed(const QString& name);

    static QDir::SortFlags getSortFlags();

    TPlaylistItem* addFile(TPlaylistItem* parent, const QFileInfo& fi);

    TPlaylistItem* addDirectory(TPlaylistItem* parent,
                                QFileInfo& fi,
                                QString name,
                                bool protectName);

    TPlaylistItem* createPath(TPlaylistItem* parent,
                              const QFileInfo& fi,
                              const QString& name,
                              int durationMS,
                              bool protectName);

    void addNewItems(TPlaylistItem* playlistItem);

    bool openM3u(TPlaylistItem* playlistItem, const QString& fileName);
    TPlaylistItem* openPlaylist(TPlaylistItem* parent,
                                const QFileInfo& fi,
                                const QString& name,
                                bool protectName);

    TPlaylistItem* addItem(TPlaylistItem* parent,
                           QString filename,
                           QString name,
                           int durationMS,
                           bool protectName,
                           bool useBlackList);

    void addFiles();
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_ADDFILESTHREAD
