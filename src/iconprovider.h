#ifndef ICONPROVIDER
#define ICONPROVIDER

#include <QFileIconProvider>
#include <QFileInfo>
#include <QSize>


class QStyle;

class TIconProvider: public QFileIconProvider {
public:
    TIconProvider();

    QIcon fileIcon;
    QIcon fileLinkIcon;
    QIcon folderIcon;
    QIcon folderLinkIcon;
    QIcon driveCDIcon;
    QIcon driveDVDIcon;

    QIcon okIcon;
    QIcon loadingIcon;
    QIcon playIcon;
    QIcon failedIcon;

    QSize iconSize;

    virtual QIcon icon(const QFileInfo& info) const;

    QIcon iconForFile(const QString& filename) const;

    void setStyle(QStyle* aStyle);

private:
    QStyle* style;
};

extern TIconProvider iconProvider;

#endif
