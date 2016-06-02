#include "gui/playlist/playlistwidgetitem.h"

#include <QApplication>
#include <QDir>
#include <QTime>
#include <QString>
#include <QUrl>

#include "log4qt/logger.h"
#include "iconprovider.h"
#include "images.h"
#include "helper.h"
#include "extensions.h"
#include "config.h"


namespace Gui {
namespace Playlist {


QString playlistItemState(TPlaylistItemState state) {

    switch (state) {
        case PSTATE_STOPPED: return "stopped";
        case PSTATE_LOADING: return "loading";
        case PSTATE_PLAYING: return "playing";
        case PSTATE_FAILED: ;
    }
    return "failed";
}

class TTimeStamp : public QTime {
public:
    TTimeStamp();
    virtual ~TTimeStamp();

    int getStamp();
};

TTimeStamp::TTimeStamp() : QTime() {
}

TTimeStamp::~TTimeStamp() {
}

int TTimeStamp::getStamp() {

    if (isNull()) {
        start();
        return 1;
    }
    return elapsed();
}

TTimeStamp timeStamper;

LOG4QT_DECLARE_STATIC_LOGGER(itemLogger, Gui::Playlist::TPlaylistItem)


TPlaylistItem::TPlaylistItem() :
    mDuration(0),
    mState(PSTATE_STOPPED),
    mPlayed(false),
    mEdited(false),
    mFolder(false),
    mPlaylist(false),
    mPlayedTime(0) {
}

TPlaylistItem::TPlaylistItem(const QString &filename,
                             const QString &name,
                             double duration,
                             bool isFolder,
                             bool protectName) :
    mFilename(filename),
    mName(name),
    mDuration(duration),
    mState(PSTATE_STOPPED),
    mPlayed(false),
    mEdited(false),
    mFolder(isFolder),
    mPlayedTime(0) {

    if (!mFilename.isEmpty() && mName.isEmpty()) {
        mName = QUrl(mFilename).toString(QUrl::RemoveScheme
                                         | QUrl::RemoveAuthority
                                         | QUrl::RemoveQuery
                                         | QUrl::RemoveFragment
                                         | QUrl::StripTrailingSlash);
        if (mName.isEmpty()) {
            mName = mFilename;
        } else {
            QString s = QFileInfo(mName).fileName();
            if (!s.isEmpty()) {
                mName = s;
            }
        }
    }

    if (protectName) {
        mEdited = true;
    } else {
        mName = Helper::cleanName(mName);
        if (mName.isEmpty()) {
            mName = qApp->translate("Gui::Playlist::TPlaylistItem", "No name");
        }
    }

    mPlaylist = extensions.isPlaylist(mFilename);
}

void TPlaylistItem::setFilename(const QString &filename) {

    mFilename = filename;
    mPlaylist = extensions.isPlaylist(mFilename);
}

void TPlaylistItem::setName(const QString& name, bool protectName) {

    if (protectName) {
        mName = name;
        mEdited = true;
    } else {
        mName = Helper::cleanName(name);
    }
}

void TPlaylistItem::setState(TPlaylistItemState state) {

    if (state == PSTATE_PLAYING) {
        mPlayed = true;
        mPlayedTime = timeStamper.getStamp();
        itemLogger()->debug("setState: stamped %1 on '%2'",
                            mPlayedTime, mFilename);
    }
    mState = state;
}

// TODO: find the file sys func reporting case
Qt::CaseSensitivity blacklistCaseSensitivity =
        #ifdef Q_OS_WIN
                    Qt::CaseInsensitive;
        #else
                    Qt::CaseSensitive;
        #endif

bool TPlaylistItem::blacklisted(const QString& filename) const {
    return mBlacklist.contains(filename, blacklistCaseSensitivity);
}

bool TPlaylistItem::whitelist(const QString& filename) {

    int i = mBlacklist.indexOf(QRegExp(filename, blacklistCaseSensitivity,
                                       QRegExp::FixedString));
    if (i >= 0) {
        itemLogger()->debug("whitelist: removed '%1' from blacklist", filename);
        mBlacklist.removeAt(i);
        return true;
    }

    itemLogger()->warn("whitelist: '%1' not found in blacklist", filename);
    return false;
}

bool TPlaylistItem::operator == (const TPlaylistItem& item) {

    if (&item == 0) {
        return false;
    }
    return item.filename() == mFilename;
}


LOG4QT_DECLARE_STATIC_LOGGER(widgetLogger, Gui::Playlist::TPlaylistWidgetItem)

const int NAME_TEXT_ALIGN = Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap;
const int TIME_TEXT_ALIGN = Qt::AlignRight | Qt::AlignVCenter;

// Level of the root node in the tree view, where level means the number of
// icons indenting the item. With root decoration on, toplevel items appear on
// level 2, being gRootNodeLevel + 1.
int gRootNodeLevel = 1;
// Set by TPlaylistWidget event handlers
int gNameColumnWidth = 0;
// Set by TPlaylistWidget constructor
QFontMetrics gNameFontMetrics = QFontMetrics(QFont());
// Set by TPlaylistWidget constructor
QSize gIconSize(16, 16);

// Set by TPlaylistWidget constructor
QIcon okIcon;
QIcon loadingIcon;
QIcon playIcon;
QIcon failedIcon;


// Used as root
TPlaylistWidgetItem::TPlaylistWidgetItem() :
    QTreeWidgetItem(),
    itemIcon(iconProvider.folderIcon),
    mModified(false) {

    playlistItem.setFolder(true);
    setFlags(ROOT_FLAGS);
    setIcon(COL_NAME, itemIcon);
    setTextAlignment(COL_NAME, NAME_TEXT_ALIGN);
    setTextAlignment(COL_TIME, TIME_TEXT_ALIGN);
}

TPlaylistWidgetItem::TPlaylistWidgetItem(QTreeWidgetItem* parent,
                                         const QString& filename,
                                         const QString& name,
                                         double duration,
                                         bool isDir,
                                         const QIcon& icon,
                                         bool protectName) :
    QTreeWidgetItem(parent),
    playlistItem(QDir::toNativeSeparators(filename), name, duration, isDir,
                 protectName),
    itemIcon(icon),
    mModified(false) {

    Qt::ItemFlags flags = Qt::ItemIsSelectable
                          | Qt::ItemIsEnabled
                          | Qt::ItemIsDragEnabled;

    if (playlistItem.folder()) {
        setFlags(flags | Qt::ItemIsDropEnabled);
    } else {
        // TODO: setFlags(flags | Qt::ItemIsEditable);
        setFlags(flags);
    }

    setIcon(COL_NAME, itemIcon);
    setTextAlignment(COL_NAME, NAME_TEXT_ALIGN);
    setText(COL_NAME, playlistItem.name());
    setToolTip(COL_NAME, playlistItem.filename());

    setTextAlignment(COL_TIME, TIME_TEXT_ALIGN);
    setDuration(playlistItem.duration());
}

TPlaylistWidgetItem::~TPlaylistWidgetItem() {
}

bool TPlaylistWidgetItem::isRoot() const {
    return parent() == 0;
}

int TPlaylistWidgetItem::getLevel() const {

    if (plParent() == 0) {
        return gRootNodeLevel;
    }
    return plParent()->getLevel() + 1;
}

void TPlaylistWidgetItem::setFilename(const QString& filename) {
    playlistItem.setFilename(filename);
}

void TPlaylistWidgetItem::setName(const QString& name, bool protectName) {

    playlistItem.setName(name, protectName);
    if (mModified) {
        setText(COL_NAME, playlistItem.name() + "*");
    } else {
        setText(COL_NAME, playlistItem.name());
    }
    setSzHint(getLevel());
}

void TPlaylistWidgetItem::setState(TPlaylistItemState state) {
    widgetLogger()->debug("setState: '%1' to %2",
                          filename(), playlistItemState(state));

    playlistItem.setState(state);

    switch (state) {
        case PSTATE_STOPPED:
            if (playlistItem.played()) {
                setIcon(COL_NAME, okIcon);
            } else {
                setIcon(COL_NAME, itemIcon);
            }
            break;
        case PSTATE_LOADING:
            setIcon(COL_NAME, loadingIcon);
            break;
        case PSTATE_PLAYING:
            setIcon(COL_NAME, playIcon);
            break;
        case PSTATE_FAILED:
            setIcon(COL_NAME, failedIcon);
            break;
    }
}

void TPlaylistWidgetItem::setDuration(double d) {

    playlistItem.setDuration(d);
    if (d > 0) {
        setText(COL_TIME, Helper::formatTime(qRound(d)));
    } else {
        setText(COL_TIME, "");
    }
}

void TPlaylistWidgetItem::setPlayed(bool played) {

    playlistItem.setPlayed(played);
    if (playlistItem.state() == PSTATE_STOPPED) {
        if (played) {
            setIcon(COL_NAME, okIcon);
        } else {
            setIcon(COL_NAME, itemIcon);
        }
    }
}

void TPlaylistWidgetItem::setModified(bool modified,
                                      bool recurse,
                                      bool markParents) {
    widgetLogger()->debug("setModified: modified set to %1 for '%2'",
                          modified, filename());

    mModified = modified;
    if (mModified) {
        setText(COL_NAME, playlistItem.name() + "*");
    } else {
        setText(COL_NAME, playlistItem.name());
    }

    if (recurse) {
        for(int c = 0; c < childCount(); c++) {
            TPlaylistWidgetItem* child = plChild(c);
            if (child->modified() != modified) {
                plChild(c)->setModified(modified, recurse, false);
            }
        }
    }

    if (mModified && markParents && plParent() && !plParent()->modified()) {
        plParent()->setModified(true, false, true);
    }
}

QString TPlaylistWidgetItem::path() const {

    QFileInfo fi(filename());
    if (isPlaylist()) {
        return fi.absolutePath();
    }
    return fi.absoluteFilePath();
}

QString TPlaylistWidgetItem::fname() const {

    QString fn = filename();
    if (!fn.isEmpty()) {
        QFileInfo fi(fn);
        fn = fi.fileName();
        if (fn == TConfig::WZPLAYLIST) {
            fn = fi.dir().dirName();
        }
    }
    return fn;
}

bool TPlaylistWidgetItem::isWZPlaylist() const {
    return QFileInfo(filename()).fileName() == TConfig::WZPLAYLIST;
}

bool TPlaylistWidgetItem::whitelist(const QString& filename) {
    return playlistItem.whitelist(filename);
}

// static
QSize TPlaylistWidgetItem::itemSize(const QString& text,
                                    int width,
                                    const QFontMetrics& fm,
                                    const QSize& iconSize,
                                    int level) {

    // TODO: get from where?
    const int hm = 4;
    const int vm = 1;

    int w = width - level * (iconSize.width() + hm) - 2 * hm;
    if (w <= 32) {
        return QSize(width, iconSize.height());
    }

    int maxh = 4 * fm.lineSpacing();
    QRect r = QRect(QPoint(), QSize(w, maxh));
    QRect br = fm.boundingRect(r, NAME_TEXT_ALIGN, text);

    int h = br.height() + 2 * vm;
    if (h < iconSize.height()) {
        h = iconSize.height();
    }

    return QSize(width, h);
};

void TPlaylistWidgetItem::setSzHint(int level) {

    if (parent()) {
        //QSize iconSize = icon(COL_NAME).actualSize(QSize(22, 22));
        setSizeHint(COL_NAME, itemSize(text(COL_NAME), gNameColumnWidth,
                                       gNameFontMetrics, gIconSize, level));
    }
}

bool TPlaylistWidgetItem::operator <(const QTreeWidgetItem& other) const {

    const TPlaylistWidgetItem* o = static_cast<const TPlaylistWidgetItem*>(&other);

    if (o == 0) {
        return false;
    }

    if (isFolder()) {
        if (o->isFolder()) {
            return QString::localeAwareCompare(filename(), o->filename()) < 0;
        }
        return true;
    }

    if (o->isFolder()) {
        return false;
    }

    return QString::localeAwareCompare(filename(), o->filename()) < 0;
}


} // namespace Playlist
} // namespace Gui
