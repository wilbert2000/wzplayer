#include "iconprovider.h"
#include <QStyle>

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

    driveCDIcon = QIcon(style->standardPixmap(QStyle::SP_DriveCDIcon));
    driveDVDIcon = QIcon(style->standardPixmap(QStyle::SP_DriveDVDIcon));
}

QIcon TIconProvider::icon(IconType type) const {

    if (type == QFileIconProvider::Folder) {
        return folderIcon;
    }
    if (type == QFileIconProvider::File) {
        return fileIcon;
    }
    return QFileIconProvider::icon(type);
}

QIcon TIconProvider::icon(const QFileInfo& fi) const {

    if (fi.isDir()) {
        return folderIcon;
    }

    if (fi.isSymLink()) {
        return fileLinkIcon;
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

