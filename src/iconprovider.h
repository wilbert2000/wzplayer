#ifndef ICONPROVIDER
#define ICONPROVIDER

#include <QIcon>
#include <QFileInfo>
#include <QSize>
#include <QMap>
#include <QStyle>


// No longer use QFileIconProvider. It fails in mysterious ways...
class TIconProvider {
public:
    TIconProvider();

    QSize iconSize;

    QIcon urlIcon;
    QIcon fileIcon;
    QIcon fileSymLinkIcon;
    QIcon fileLinkIcon;
    QIcon folderIcon;
    QIcon folderSymLinkIcon;

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
    QIcon browseURLIcon;
    QIcon newFolderIcon;
    QIcon closeIcon;
    QIcon quitIcon;

    QIcon pauseIcon;
    QIcon playIcon;

    QIcon fullscreenIcon;
    QIcon fullscreenExitIcon;
    QIcon windowSizeIcon;
    QIcon optimizeSizeIcon;
    QIcon zoomResetIcon;
    QIcon zoomInIcon;
    QIcon zoomOutIcon;

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

    void setStyle(QStyle* aStyle);
    QIcon getIconSymLinked(const QIcon& icon);
    QIcon getIconLinked(const QIcon& icon);
    QIcon getIconBlacklisted(const QIcon& icon);
    QIcon getIconEdited(const QIcon& icon);

private:
    QStyle* style;
    QMap<qint64, QIcon> iconBlacklistedCache;
    QMap<qint64, QIcon> iconEditedCache;

    QIcon getIconImage(const QString& name, const QString& name2);
    QIcon getIconStd(const QString& name, QStyle::StandardPixmap stdIcon);

    QIcon getFileSymLinkedIcon();
    QPixmap getPixMapEdited(QIcon icon, QIcon::Mode mode, QIcon::State state);
    QPixmap getPixMapBlacklist(QIcon icon, QIcon::Mode mode, QIcon::State state);
};

extern TIconProvider iconProvider;

#endif
