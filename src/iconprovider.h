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

    QIcon fileIcon;
    QIcon fileLinkIcon;
    QIcon folderIcon;
    QIcon folderLinkIcon;
    QIcon driveCDIcon;
    QIcon driveDVDIcon;
    QIcon urlIcon;

    QIcon okIcon;
    QIcon loadingIcon;
    QIcon playIcon;
    QIcon failedIcon;

    QSize iconSize;

    QIcon icon(const QFileInfo& info) const;
    QIcon iconForFile(const QString& filename) const;

    void setStyle(QStyle* aStyle);

private:
    QStyle* style;
};

extern TIconProvider iconProvider;

#endif
