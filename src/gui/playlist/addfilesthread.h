#ifndef GUI_PLAYLIST_ADDFILESTHREAD
#define GUI_PLAYLIST_ADDFILESTHREAD

#include <QThread>
#include <QStringList>
#include <QString>
#include <QDir>

#include "wzdebug.h"


class QDir;
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
    TPlaylistWidgetItem* currentItem;
    QString latestDir;

signals:
    void displayMessage(const QString&, int);

private:
    bool abortRequested;
    bool stopRequested;
    bool recurse;
    Qt::CaseSensitivity caseSensitiveNames;

    QString playlistPath;
    QStringList blacklist;
    QStringList nameFilterList;
    QList<QRegExp*> rxNameBlacklist;

    bool blacklisted(QString filename);
    bool blacklisted(const QFileInfo& fi);
    bool blacklisted(const QDir& dir);
    void whitelist();

    bool nameBlackListed(const QString& name);

    QDir::SortFlags getSortFlags();

    TPlaylistWidgetItem* addFile(TPlaylistWidgetItem* parent, QFileInfo& fi);
    TPlaylistWidgetItem* addDirectory(TPlaylistWidgetItem* parent,
                                      QFileInfo& fi,
                                      const QString& name);
    void addFiles();

    TPlaylistWidgetItem* addItemNotFound(TPlaylistWidgetItem* parent,
                                         const QString& filename,
                                         QString name,
                                         const QFileInfo& fi,
                                         bool protectName,
                                         bool wzplaylist);
    TPlaylistWidgetItem* createPath(TPlaylistWidgetItem* parent,
                                    const QFileInfo& fi,
                                    const QString& name,
                                    double duration,
                                    bool protectName);
    TPlaylistWidgetItem* addItem(TPlaylistWidgetItem* parent,
                                 QString filename,
                                 QString name = "",
                                 double duration = 0,
                                 bool wzplaylist = false);

    void addNewItems(TPlaylistWidgetItem* playlistItem,
                     const QFileInfo& playlistInfo);
    bool openM3u(TPlaylistWidgetItem* playlistItem, const QFileInfo& fi,
                 bool utf8);
    bool openPls(TPlaylistWidgetItem* playlistItem,
                 const QString& playlistFileName);
    TPlaylistWidgetItem* openPlaylist(TPlaylistWidgetItem* parent,
                                      QFileInfo& fi);
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_ADDFILESTHREAD
