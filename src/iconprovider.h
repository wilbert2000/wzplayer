#ifndef ICONPROVIDER
#define ICONPROVIDER

#include <QIcon>
#include <QFileInfo>
#include <QSize>


class QStyle;

// No longer use QFileIconProvider. It fails in mysterious ways...
class TIconProvider {
public:
    TIconProvider();

    QSize iconSize;

    QIcon fileIcon;
    QIcon fileLinkIcon;
    QIcon folderIcon;
    QIcon folderLinkIcon;

    QIcon iconPlayed;
    QIcon iconLoading;
    QIcon iconPlaying;
    QIcon iconFailed;

    QIcon saveIcon;
    QIcon saveAsIcon;
    QIcon refreshIcon;
    QIcon newFolderIcon;

    QIcon cutIcon;
    QIcon copyIcon;
    QIcon pasteIcon;
    QIcon findIcon;

    QIcon okIcon;
    QIcon cancelIcon;

    QIcon trashIcon;
    QIcon discardIcon;
    QIcon clearIcon;

    //QIcon icon(const QFileInfo& info) const;
    //QIcon iconForFile(const QString& filename) const;

    void setStyle(QStyle* aStyle);

private:
    QStyle* style;
};

extern TIconProvider iconProvider;

#endif
