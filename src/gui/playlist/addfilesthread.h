#ifndef GUI_PLAYLIST_ADDFILESTHREAD
#define GUI_PLAYLIST_ADDFILESTHREAD

#include <QThread>
#include <QStringList>
#include <QString>


class QTreeWidgetItem;

namespace Gui {
namespace Playlist {

class TPlaylistWidgetItem;


class TAddFilesThread : public QThread {
    Q_OBJECT
public:
    TAddFilesThread(QObject* parent,
                    const QStringList& aFiles,
                    const QString& aPlaylistPath,
                    bool recurseSubDirs,
                    bool aSearchForItems);
    virtual ~TAddFilesThread();

    virtual void run();
    void stop() { stopRequested = true; }

    // Inputs
    const QStringList& files;
    QString playlistPath;

    // Outputs
    QTreeWidgetItem* root;
    QTreeWidgetItem* currentItem;
    QString playlistFileName;
    QString latestDir;

signals:
    void displayMessage(const QString&, int);
    void setWinTitle(QString title);

private:
    bool stopRequested;
    bool recurse;
    bool searchForItems;

    QTreeWidgetItem* cleanAndAddItem(QString filename,
                                     QString name,
                                     double duration,
                                     QTreeWidgetItem* parent);

    QTreeWidgetItem* addFile(QTreeWidgetItem* parent, QString filename);
    QTreeWidgetItem* addDirectory(QTreeWidgetItem* parent, const QString& dir);
    void addFiles();


    QTreeWidgetItem* openM3u(const QString& aPlaylistFileName, QTreeWidgetItem* parent);
    QTreeWidgetItem* openPls(const QString& aPlaylistFileName, QTreeWidgetItem* parent);
    TPlaylistWidgetItem* findFilename(const QString& filename);
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_ADDFILESTHREAD
