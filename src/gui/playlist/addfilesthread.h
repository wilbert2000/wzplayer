#ifndef GUI_PLAYLIST_ADDFILESTHREAD
#define GUI_PLAYLIST_ADDFILESTHREAD

#include <QThread>
#include <QStringList>
#include <QString>
#include "log4qt/logger.h"

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

    TPlaylistWidgetItem* cleanAndAddItem(QString filename,
                                         QString name,
                                         double duration,
                                         TPlaylistWidgetItem* parent);

    TPlaylistWidgetItem* addFile(TPlaylistWidgetItem* parent, const QString& filename);
    TPlaylistWidgetItem* addDirectory(TPlaylistWidgetItem* parent,
                                      const QString& dir);
    void addFiles();

    TPlaylistWidgetItem* createFolder(TPlaylistWidgetItem* parent,
                                      TPlaylistWidgetItem* after,
                                      const QFileInfo& fi);
    TPlaylistWidgetItem* openM3u(const QString& playlistFileName,
                                 TPlaylistWidgetItem* parent);
    TPlaylistWidgetItem* openPls(const QString& playlistFileName,
                                 TPlaylistWidgetItem* parent);
    TPlaylistWidgetItem* findFilename(const QString& filename);
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_ADDFILESTHREAD
