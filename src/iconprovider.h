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

    QIcon urlIcon;
    QIcon urlEditedIcon;
    QIcon fileIcon;
    QIcon fileEditedIcon;
    QIcon fileLinkIcon;
    QIcon fileLinkEditedIcon;
    QIcon folderIcon;
    QIcon folderEditedIcon;
    QIcon folderLinkIcon;
    QIcon folderLinkEditedIcon;

    QIcon iconPlayed;
    QIcon iconLoading;
    QIcon iconPlaying;
    QIcon iconStopping;
    QIcon iconFailed;

    QIcon recentIcon;
    QIcon openIcon;
    QIcon saveIcon;
    QIcon saveAsIcon;
    QIcon refreshIcon;
    QIcon newFolderIcon;

    QIcon pauseIcon;
    QIcon playIcon;

    QIcon cutIcon;
    QIcon copyIcon;
    QIcon pasteIcon;
    QIcon findIcon;

    QIcon okIcon;
    QIcon cancelIcon;

    QIcon trashIcon;
    QIcon discardIcon;
    QIcon clearIcon;

    QIcon conflictItem;

    //QIcon icon(const QFileInfo& info) const;
    //QIcon iconForFile(const QString& filename) const;

    void setStyle(QStyle* aStyle);

private:
    QStyle* style;

    QPixmap getPixMapEdited(QIcon icon, QIcon::Mode mode, QIcon::State state);
    QIcon getIconEdited(const QIcon& icon, bool addOnIcon = false);

};

extern TIconProvider iconProvider;

#endif
