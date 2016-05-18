#ifndef GUI_PLAYLIST_ADDFILESTHREAD
#define GUI_PLAYLIST_ADDFILESTHREAD

#include <QThread>
#include <QStringList>
#include <QString>
#include "log4qt/logger.h"

class QDir;
class QFileInfo;

namespace Gui {
namespace Playlist {

class TPlaylistWidgetItem;


class TAddFilesThread : public QThread {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    TAddFilesThread(QObject* parent,
                    const QStringList& aFiles,
                    bool recurseSubDirs,
                    bool aSearchForItems);
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
    bool searchForItems;

    QString playlistPath;
    QStringList blackList;

    bool blackListed(QString filename);
    bool blackListed(const QFileInfo& fi);
    bool blackListed(const QDir& dir);
    void whiteList();

    TPlaylistWidgetItem* addFile(TPlaylistWidgetItem* parent,
                                 const QString& filename);
    TPlaylistWidgetItem* addDirectory(TPlaylistWidgetItem* parent,
                                      const QString& dir);
    void addFiles();

    TPlaylistWidgetItem* findFilename(const QString& filename);

    void createPath(TPlaylistWidgetItem* parent,
                    const QFileInfo& fi,
                    const QString& name,
                    double duration,
                    bool protectName);
    void cleanAndAddItem(TPlaylistWidgetItem* parent,
                         QString filename,
                         QString name,
                         double duration);

    bool openM3u(TPlaylistWidgetItem* playlistItem,
                 const QString& playlistFileName,
                 bool utf8);
    bool openPls(TPlaylistWidgetItem* playlistItem,
                 const QString& playlistFileName);
    TPlaylistWidgetItem* openPlaylist(TPlaylistWidgetItem* parent,
                                      const QString& playlistFileName);
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_ADDFILESTHREAD
