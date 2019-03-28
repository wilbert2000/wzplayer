#ifndef ICONPROVIDER
#define ICONPROVIDER

#include <QIcon>
#include <QFileInfo>
#include <QSize>
#include <QMap>


class QStyle;

// No longer use QFileIconProvider. It fails in mysterious ways...
class TIconProvider {
public:
    TIconProvider();

    QSize iconSize;
    QSize actualIconSize;

    QIcon urlIcon;
    QIcon fileIcon;
    QIcon fileLinkIcon;
    QIcon folderIcon;
    QIcon folderLinkIcon;

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
    QIcon getCachedIcon(const QIcon& icon, bool addOnIcon = false);
    QIcon getIconBlacklist(const QIcon& icon);
    QIcon getIconEdited(const QIcon& icon, bool addOnIcon = false);

private:
    QStyle* style;
    QMap<qint64, QIcon> iconCache;

    QPixmap getPixMapEdited(QIcon icon, QIcon::Mode mode, QIcon::State state);
    QPixmap getPixMapBlacklist(QIcon icon, QIcon::Mode mode, QIcon::State state);
};

extern TIconProvider iconProvider;

#endif
