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
    iconForFile("/");
}

QIcon TIconProvider::icon(IconType type) const {

    if (type == QFileIconProvider::Folder) {
        return folderIcon;
    }
    return QFileIconProvider::icon(type);
}

QIcon TIconProvider::icon(const QFileInfo& fi) const {

    if (fi.filePath().startsWith("dvd://")
        || fi.filePath().startsWith("dvdnav://")
        || fi.filePath().startsWith("br://")) {
        QIcon i;
        i.addPixmap(style->standardPixmap(QStyle::SP_DriveDVDIcon));
        return i;
    }
    if (fi.filePath().startsWith("vcd://")
        || fi.filePath().startsWith("cdda://")) {
        QIcon i;
        i.addPixmap(style->standardPixmap(QStyle::SP_DriveCDIcon));
        return i;
    }

    QIcon i = QFileIconProvider::icon(fi);
    if (i.isNull()) {
        if (fi.isDir()) {
            i = icon(QFileIconProvider::Folder);
        } else {
            i = icon(QFileIconProvider::File);
        }
    }
    return i;
}

QIcon TIconProvider::iconForFile(const QString& filename) const {

    /*
    QStyle::SP_DriveCDIcon	18	The CD icon.
    QStyle::SP_DriveDVDIcon	19	The DVD icon.
    */


    return icon(QFileInfo(filename));
}

