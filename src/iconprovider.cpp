#include "iconprovider.h"
#include "extensions.h"
#include "images.h"
#include "wzdebug.h"
#include <QApplication>
#include <QStyle>
#include <QPainter>


LOG4QT_DECLARE_STATIC_LOGGER(logger, TIconProvider)


TIconProvider iconProvider;

TIconProvider::TIconProvider() : iconSize(QSize(22, 22)) {
}

QIcon TIconProvider::getIconSymLinked(const QIcon& icon) {

    if (icon.cacheKey() == folderIcon.cacheKey()) {
        return folderSymLinkIcon;
    }
    return fileSymLinkIcon;
}

QIcon TIconProvider::getIconLinked(const QIcon& icon) {

    if (icon.cacheKey() == folderIcon.cacheKey()) {
        return folderSymLinkIcon;
    }
    return fileLinkIcon;
}

static QPen getPen() {
    return QPen(qApp->palette("TPlaylistWidget").color(QPalette::Text));
}

QPixmap TIconProvider::getPixMapBlacklist(QIcon icon,
                                       QIcon::Mode mode,
                                       QIcon::State state) {

    QSize size = icon.actualSize(iconSize);
    QPixmap pixmap = icon.pixmap(size, mode, state);
    QPainter painter(&pixmap);
    painter.setPen(getPen());
    painter.setBrush(QBrush(QColor(255, 0, 0)));
    QSize s = QSize(6, 6);
    QSize o = size - s - QSize(1, 1);
    painter.drawRect(QRect(QPoint(o.width(), o.height()), s));
    return pixmap;
}

QIcon TIconProvider::getIconBlacklisted(const QIcon& icon) {

    QIcon newIcon = iconBlacklistedCache[icon.cacheKey()];
    if (newIcon.isNull()) {
        newIcon.addPixmap(getPixMapBlacklist(icon, QIcon::Normal, QIcon::Off),
                          QIcon::Normal, QIcon::Off);
        newIcon.addPixmap(getPixMapBlacklist(icon, QIcon::Normal, QIcon::On),
                          QIcon::Normal, QIcon::On);
        iconBlacklistedCache[icon.cacheKey()] = newIcon;
        WZTRACE(QString("Created blacklist icon for key %1")
                .arg(icon.cacheKey()));
    } else {
        WZTRACE(QString("Returning icon from cache for key %1")
                .arg(icon.cacheKey()));

    }
    return newIcon;
}

QPixmap TIconProvider::getPixMapEdited(QIcon icon,
                                       QIcon::Mode mode,
                                       QIcon::State state) {

    QSize size = icon.actualSize(iconSize);
    QPixmap pixmap = icon.pixmap(size, mode, state);
    QPainter painter(&pixmap);
    painter.setPen(getPen());
    painter.setBrush(QBrush(QColor(0, 0, 255)));
    QSize s = QSize(6, 6);
    painter.drawRect(QRect(QPoint(size.width() - s.width() - 1, 0), s));
    return pixmap;
}

QIcon TIconProvider::getIconEdited(const QIcon& icon) {

    QIcon& newIcon = iconEditedCache[icon.cacheKey()];
    if (newIcon.isNull()) {
        newIcon.addPixmap(getPixMapEdited(icon, QIcon::Normal, QIcon::Off),
                          QIcon::Normal, QIcon::Off);
        if (icon.availableSizes(QIcon::Normal, QIcon::On).count()) {
            newIcon.addPixmap(getPixMapEdited(icon, QIcon::Normal, QIcon::On),
                              QIcon::Normal, QIcon::On);
        }
        iconEditedCache[icon.cacheKey()] = newIcon;
        WZTRACE(QString("Created edited icon for key %1")
                .arg(icon.cacheKey()));
    } else {
        WZTRACE(QString("Returning icon from cache for key '%1'")
                .arg(icon.cacheKey()));

    }
    return newIcon;
}

QIcon TIconProvider::getFileSymLinkedIcon() {

    QIcon linkIcon = QIcon::fromTheme("emblem-symbolic-link");
    if (linkIcon.isNull()) {
        return style->standardIcon(QStyle::SP_FileLinkIcon);
    }
    QSize size = fileIcon.actualSize(iconSize);
    QPixmap pixmap = fileIcon.pixmap(size);
    QPainter painter(&pixmap);
    linkIcon.paint(&painter, QRect(QPoint(), size));
    return pixmap;
}

QIcon TIconProvider::getIconImage(const QString& name, const QString& name2) {

    QIcon icon = QIcon::fromTheme(name);
    if (icon.isNull()) {
        return Images::icon(name2);
    }
    return icon;
}

QIcon TIconProvider::getIconStd(const QString& name,
                               QStyle::StandardPixmap stdIcon) {

    QIcon icon = QIcon::fromTheme(name);
    if (icon.isNull()) {
        return style->standardIcon(stdIcon);
    }
    return icon;
}


void TIconProvider::setStyle(QStyle* aStyle) {

    style = aStyle;

    urlIcon =  Images::icon("open_url");
    fileIcon = style->standardIcon(QStyle::SP_FileIcon);

    fileSymLinkIcon = getFileSymLinkedIcon();
    fileLinkIcon = getIconStd("emblem-symbolic-link", QStyle::SP_FileLinkIcon);

    folderIcon = QIcon();
    folderIcon.addPixmap(style->standardPixmap(QStyle::SP_DirClosedIcon),
                         QIcon::Normal, QIcon::Off);
    folderIcon.addPixmap(style->standardPixmap(QStyle::SP_DirOpenIcon),
                         QIcon::Normal, QIcon::On);

    folderSymLinkIcon = QIcon();
    folderSymLinkIcon.addPixmap(style->standardPixmap(QStyle::SP_DirLinkIcon),
                             QIcon::Normal, QIcon::Off);
    folderSymLinkIcon.addPixmap(style->standardPixmap(QStyle::SP_DirLinkOpenIcon),
                             QIcon::Normal, QIcon::On);

    iconPlayed =  style->standardIcon(QStyle::SP_DialogOkButton);
    iconLoading = Images::icon("loading");
    iconPlaying =  style->standardIcon(QStyle::SP_MediaPlay);
    iconStopping = Images::icon("stopping");
    iconFailed =  Images::icon("failed");

    recentIcon = getIconImage("document-open-recent", "recent_menu");
    openIcon = style->standardIcon(QStyle::SP_DialogOpenButton);
    saveIcon = style->standardIcon(QStyle::SP_DialogSaveButton);
    saveAsIcon = getIconStd("document-save-as", QStyle::SP_DriveHDIcon);
    refreshIcon = style->standardIcon(QStyle::SP_BrowserReload);
    browseURLIcon = getIconStd("system-file-manager", QStyle::SP_DirOpenIcon);

    newFolderIcon = style->standardIcon(QStyle::SP_FileDialogNewFolder);

    closeIcon = getIconStd("window-close", QStyle::SP_DialogCloseButton);
    quitIcon = getIconImage("application-exit", "exit");

    pauseIcon = Images::icon("pause");
    playIcon = Images::icon("play");

    fullscreenIcon = getIconImage("view-fullscreen", "fullscreen");
    fullscreenExitIcon = getIconImage("view-restore", "fullscreen");
    windowSizeIcon = getIconImage("zoom-in", "window_size_menu");
    optimizeSizeIcon = getIconImage("zoom-fit-best", "size_optimize");
    zoomResetIcon = getIconImage("zoom-original", "reset_zoom_pan");
    zoomInIcon = getIconImage("zoom-in", "inc_zoom");
    zoomOutIcon = getIconImage("zoom-out", "dec_zoom");

    cutIcon = getIconImage("edit-cut", "cut");
    copyIcon = getIconImage("edit-copy", "copy");
    pasteIcon = getIconImage("edit-paste", "paste");
    findIcon = getIconImage("edit-find", "find");

    okIcon = style->standardIcon(QStyle::SP_DialogOkButton);
    cancelIcon = style->standardIcon(QStyle::SP_DialogCancelButton);

    trashIcon = style->standardIcon(QStyle::SP_TrashIcon);
    discardIcon = style->standardIcon(QStyle::SP_DialogDiscardButton);
    clearIcon = getIconStd("edit-clear", QStyle::SP_DialogResetButton);

    conflictItem = Images::icon("conflict");
}
