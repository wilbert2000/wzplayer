#include "iconprovider.h"
#include "extensions.h"
#include "images.h"
#include <QStyle>
#include <QPainter>


TIconProvider iconProvider;

TIconProvider::TIconProvider() {
}

QPixmap TIconProvider::getPixMapEdited(QIcon icon,
                                       QIcon::Mode mode,
                                       QIcon::State state) {

    QSize size(22, 22);
    QPixmap pixmap = icon.pixmap(size, mode, state);
    QPainter painter(&pixmap);
    painter.setPen(QPen(QColor(0, 0, 0)));
    painter.setBrush(QBrush(QColor(0, 0, 255)));
    QSize s = QSize(7, 7);
    painter.drawRect(QRect(QPoint(14, 0), s));
    return pixmap;
}

QIcon TIconProvider::getIconEdited(const QIcon& icon, bool addOnIcon) {

    QIcon newIcon;
    newIcon.addPixmap(getPixMapEdited(icon, QIcon::Normal, QIcon::Off),
                      QIcon::Normal, QIcon::Off);

    if (addOnIcon) {
        newIcon.addPixmap(getPixMapEdited(icon, QIcon::Normal, QIcon::On),
                          QIcon::Normal, QIcon::On);

    }
    return newIcon;
}

void TIconProvider::setStyle(QStyle* aStyle) {

    style = aStyle;

    urlIcon =  Images::icon("open_url");
    urlEditedIcon = getIconEdited(urlIcon);
    fileIcon = style->standardIcon(QStyle::SP_FileIcon);
    // TODO: fix "device independant pixels"?
    iconSize = folderIcon.actualSize(QSize(22, 22));
    fileEditedIcon = getIconEdited(fileIcon);
    fileLinkIcon = style->standardIcon(QStyle::SP_FileLinkIcon);
    fileLinkEditedIcon = getIconEdited(fileLinkIcon);

    folderIcon = QIcon();
    folderIcon.addPixmap(style->standardPixmap(QStyle::SP_DirClosedIcon),
                         QIcon::Normal, QIcon::Off);
    folderIcon.addPixmap(style->standardPixmap(QStyle::SP_DirOpenIcon),
                         QIcon::Normal, QIcon::On);
    folderEditedIcon = getIconEdited(folderIcon, true);

    folderLinkIcon = QIcon();
    folderLinkIcon.addPixmap(style->standardPixmap(QStyle::SP_DirLinkIcon),
                             QIcon::Normal, QIcon::Off);
    folderLinkIcon.addPixmap(style->standardPixmap(QStyle::SP_DirLinkOpenIcon),
                             QIcon::Normal, QIcon::On);
    folderLinkEditedIcon = getIconEdited(folderLinkIcon, true);

    iconPlayed =  style->standardIcon(QStyle::SP_DialogOkButton);
    iconLoading = QIcon();
    iconLoading.addPixmap(Images::icon("loading"), QIcon::Normal, QIcon::Off);
    iconLoading.addPixmap(Images::icon("loading"), QIcon::Disabled, QIcon::Off);
    iconPlaying =  style->standardIcon(QStyle::SP_MediaPlay);
    iconStopping = QIcon();
    iconStopping.addPixmap(Images::icon("stopping"), QIcon::Normal, QIcon::Off);
    iconStopping.addPixmap(Images::icon("stopping"), QIcon::Disabled, QIcon::Off);
    iconFailed =  Images::icon("failed");

    recentIcon = QIcon::fromTheme("document-open-recent",
                                  Images::icon("recent_menu"));
    openIcon = style->standardIcon(QStyle::SP_DialogOpenButton);
    saveIcon = style->standardIcon(QStyle::SP_DialogSaveButton);
    saveAsIcon = QIcon::fromTheme("document-save-as",
        style->standardIcon(QStyle::SP_DriveHDIcon));
    refreshIcon = style->standardIcon(QStyle::SP_BrowserReload);
    newFolderIcon = style->standardIcon(QStyle::SP_FileDialogNewFolder);

    pauseIcon = Images::icon("pause");
    playIcon = Images::icon("play");

    cutIcon = QIcon::fromTheme("edit-cut", Images::icon("cut"));
    copyIcon = QIcon::fromTheme("edit-copy", Images::icon("copy"));
    pasteIcon = QIcon::fromTheme("edit-paste", Images::icon("paste"));
    findIcon = QIcon::fromTheme("edit-find", Images::icon("find"));

    okIcon = style->standardIcon(QStyle::SP_DialogOkButton);
    cancelIcon = style->standardIcon(QStyle::SP_DialogCancelButton);

    trashIcon = style->standardIcon(QStyle::SP_TrashIcon);
    discardIcon = style->standardIcon(QStyle::SP_DialogDiscardButton);
    clearIcon = QIcon::fromTheme("edit-clear",
        style->standardIcon(QStyle::SP_DialogResetButton));

    conflictItem = Images::icon("conflict");
}

/* No longer used
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

    if (fi.filePath().indexOf("://") > 0) {
        return urlIcon;
    }

    return fileIcon;
}

QIcon TIconProvider::iconForFile(const QString& filename) const {

    if (filename.isEmpty()) {
        return fileIcon;
    }
    return icon(QFileInfo(filename));
}
*/
