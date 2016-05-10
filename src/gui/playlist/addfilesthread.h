#ifndef GUI_PLAYLIST_ADDFILESTHREAD
#define GUI_PLAYLIST_ADDFILESTHREAD

#include <QThread>
#include <QStringList>
#include <QString>


namespace Gui {
namespace Playlist {

class TPlaylistWidgetItem;


class TAddFilesThread : public QThread {
    Q_OBJECT
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

    TPlaylistWidgetItem* addFile(TPlaylistWidgetItem* parent, QString filename);
    TPlaylistWidgetItem* addDirectory(TPlaylistWidgetItem* parent,
                                      const QString& dir);
    void addFiles();

    TPlaylistWidgetItem* openM3u(const QString& playlistFileName,
                                 TPlaylistWidgetItem* parent);
    TPlaylistWidgetItem* openPls(const QString& playlistFileName,
                                 TPlaylistWidgetItem* parent);
    TPlaylistWidgetItem* findFilename(const QString& filename);
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_ADDFILESTHREAD
