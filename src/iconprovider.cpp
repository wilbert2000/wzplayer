#include "iconprovider.h"
#include <QStyle>

TIconProvider iconProvider;

TIconProvider::TIconProvider() :
    QFileIconProvider() {
}

void TIconProvider::setStyle(QStyle* aStyle) {

    style = aStyle;

    // Need to reset in case restarting
    folderIcon = QIcon();
    folderIcon.addPixmap(style->standardPixmap(QStyle::SP_DirClosedIcon),
                         QIcon::Normal, QIcon::Off);
    folderIcon.addPixmap(style->standardPixmap(QStyle::SP_DirOpenIcon),
                         QIcon::Normal, QIcon::On);

    // To prevent: Fatal KGlobal::locale() must be called from the main thread
    // before using i18n() in threads, init it from here, the main thread.
    QFileIconProvider::icon(QFileInfo("/"));
}

QIcon TIconProvider::icon(IconType type) const {

    if (type == QFileIconProvider::Folder) {
        return folderIcon;
    }
    return QFileIconProvider::icon(type);
}

QIcon TIconProvider::icon(const QFileInfo& fi) const {

    // TODO: handle the different kind of folders.
    // For now need icon with open and close pixmap.
    if (fi.isDir()) {
        return folderIcon;
    }

    QIcon i;
    if (fi.filePath().startsWith("dvd://")
        || fi.filePath().startsWith("dvdnav://")
        || fi.filePath().startsWith("br://")) {
        i.addPixmap(style->standardPixmap(QStyle::SP_DriveDVDIcon));
    } else if (fi.filePath().startsWith("vcd://")
               || fi.filePath().startsWith("cdda://")) {
        i.addPixmap(style->standardPixmap(QStyle::SP_DriveCDIcon));
    } else {
        i = QFileIconProvider::icon(fi);
        if (i.isNull()) {
            i = icon(QFileIconProvider::File);
        }
    }
    return i;
}

QIcon TIconProvider::iconForFile(const QString& filename) const {
    return icon(QFileInfo(filename));
}

