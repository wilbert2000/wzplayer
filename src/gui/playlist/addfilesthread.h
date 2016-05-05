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
                    bool recurseSubDirs,
                    bool aSearchForItems);
    virtual ~TAddFilesThread();

    virtual void run();
    void stop() { stopRequested = true; }

    QTreeWidgetItem* root;
    QTreeWidgetItem* currentItem;
    QString latestDir;

signals:
    void displayMessage(const QString&, int);

private:
    bool stopRequested;
    const QStringList& files;
    bool recurse;
    bool searchForItems;

    QTreeWidgetItem* addFile(QTreeWidgetItem* parent, const QString &filename);
    QTreeWidgetItem* addDirectory(QTreeWidgetItem* parent, const QString& dir);
    void addFiles();


    QTreeWidgetItem* openM3u(const QString& file, QTreeWidgetItem* parent);
    QTreeWidgetItem* openPls(const QString& file, QTreeWidgetItem* parent);
    TPlaylistWidgetItem* findFilename(const QString& filename);
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_ADDFILESTHREAD
