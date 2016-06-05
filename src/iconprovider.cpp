#include "iconprovider.h"
#include <QStyle>
#include "extensions.h"
#include "images.h"


TIconProvider iconProvider;

TIconProvider::TIconProvider() :
    QFileIconProvider() {
}

void TIconProvider::setStyle(QStyle* aStyle) {

    style = aStyle;

    fileIcon = QIcon(style->standardPixmap(QStyle::SP_FileIcon));
    fileLinkIcon = QIcon(style->standardPixmap(QStyle::SP_FileLinkIcon));

    folderIcon = QIcon();
    folderIcon.addPixmap(style->standardPixmap(QStyle::SP_DirClosedIcon),
                         QIcon::Normal, QIcon::Off);
    folderIcon.addPixmap(style->standardPixmap(QStyle::SP_DirOpenIcon),
                         QIcon::Normal, QIcon::On);

    folderLinkIcon = QIcon(style->standardPixmap(QStyle::SP_DirLinkIcon));
    driveCDIcon = QIcon(style->standardPixmap(QStyle::SP_DriveCDIcon));
    driveDVDIcon = QIcon(style->standardPixmap(QStyle::SP_DriveDVDIcon));

    iconSize = folderIcon.actualSize(QSize(22, 22));
    okIcon = Images::icon("ok", iconSize.width());
    loadingIcon = Images::icon("loading", iconSize.width());
    playIcon = Images::icon("play", iconSize.width());
    failedIcon = Images::icon("failed", iconSize.width());
}

QIcon TIconProvider::icon(const QFileInfo& fi) const {

    if (fi.isSymLink()) {
        if (fi.isDir() || extensions.isPlaylist(fi)) {
            return folderLinkIcon;
        }
        return fileLinkIcon;
    }

    if (fi.isDir() || extensions.isPlaylist(fi)) {
        return folderIcon;
    }

    if (fi.filePath().startsWith("dvd://")
        || fi.filePath().startsWith("dvdnav://")
        || fi.filePath().startsWith("br://")) {
        return driveDVDIcon;
    }

    if (fi.filePath().startsWith("vcd://")
        || fi.filePath().startsWith("cdda://")) {
        return driveCDIcon;
    }

    QIcon i = QFileIconProvider::icon(fi);
    if (i.isNull()) {
        return fileIcon;
    }

    return i;
}

QIcon TIconProvider::iconForFile(const QString& filename) const {
    return icon(QFileInfo(filename));
}

