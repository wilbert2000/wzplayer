#include "gui/playlist/playlistwidgetitem.h"

#include <QDir>
#include <QTime>
#include <QString>

#include "log4qt/logger.h"
#include "images.h"
#include "helper.h"

namespace Gui {
namespace Playlist {


QString playlistItemState(TPlaylistItemState state) {

    switch (state) {
        case PSTATE_STOPPED: return "Stopped";
        case PSTATE_LOADING: return "Loading";
        case PSTATE_PLAYING: return "Playing";
        case PSTATE_FAILED: ;
    }
    return "Failed";
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

TPlaylistItem::TPlaylistItem() :
    _duration(0),
    _state(PSTATE_STOPPED),
    _played(false),
    _edited(false),
    _folder(false),
    _playedTime(0) {
}

TPlaylistItem::TPlaylistItem(const QString &filename,
                             const QString &name,
                             double duration,
                             bool isFolder) :
    _filename(filename),
    _name(name),
    _duration(duration),
    _state(PSTATE_STOPPED),
    _played(false),
    _edited(false),
    _folder(isFolder),
    _playedTime(0) {

    if (_name.isEmpty()) {
        _name = _filename;
    }
}

void TPlaylistItem::setState(TPlaylistItemState state) {

    if (state == PSTATE_PLAYING) {
        _played = true;
        _playedTime = timeStamper.getStamp();
        Log4Qt::Logger::logger("Gui::Playlist::TPlaylistItem")->logger()->debug(
                    "setState: stamped %1 on %2", _playedTime, _filename);
    }
    _state = state;
}

bool TPlaylistItem::operator == (const TPlaylistItem& item) {

    if (&item == 0) {
        return false;
    }
    return item.filename() == _filename;
}


const int NAME_TEXT_ALIGN = Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap;

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


// Used as root during scan
TPlaylistWidgetItem::TPlaylistWidgetItem(const QIcon& icon) :
    QTreeWidgetItem(),
    itemIcon(icon) {

    playlistItem.setFolder(true);
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
    setIcon(COL_NAME, itemIcon);
    setTextAlignment(COL_NAME, NAME_TEXT_ALIGN);
}

TPlaylistWidgetItem::TPlaylistWidgetItem(QTreeWidgetItem* parent,
                                         QTreeWidgetItem* after,
                                         const QString& filename,
                                         const QString& name,
                                         double duration,
                                         bool isDir,
                                         const QIcon& icon) :
    QTreeWidgetItem(parent, after),
    playlistItem(QDir::toNativeSeparators(filename), name, duration, isDir),
    itemIcon(icon) {

    Qt::ItemFlags flags = Qt::ItemIsSelectable
                          | Qt::ItemIsDragEnabled
                          | Qt::ItemIsEnabled;
    if (isDir) {
        setFlags(flags | Qt::ItemIsDropEnabled);
    } else {
        // TODO: setFlags(flags | Qt::ItemIsEditable);
        setFlags(flags);
    }

    setIcon(COL_NAME, itemIcon);

    setTextAlignment(COL_NAME, NAME_TEXT_ALIGN);
    setText(COL_NAME, name);
    setToolTip(COL_NAME, playlistItem.filename());

    setDuration(duration);
}

TPlaylistWidgetItem::~TPlaylistWidgetItem() {
}

int TPlaylistWidgetItem::getLevel() const {

    if (parent() == 0) {
        return gRootNodeLevel + 1;
    }
    return static_cast<TPlaylistWidgetItem*>(parent())->getLevel() + 1;
}

void TPlaylistWidgetItem::setState(TPlaylistItemState state) {
    Log4Qt::Logger::logger("Gui::Playlist::TPlaylistWidgetItem")->debug(
        "setState: '" + filename() + "' to " + playlistItemState(state));

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

void TPlaylistWidgetItem::setFileInfo(const QFileInfo& fi) {

    playlistItem.setFilename(QDir::toNativeSeparators(fi.absoluteFilePath()));
    setName(fi.fileName());
}

void TPlaylistWidgetItem::setName(const QString& name) {

    playlistItem.setName(name);
    setText(COL_NAME, name);
    setSzHint(getLevel());
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

QSize TPlaylistWidgetItem::itemSize(const QString& text,
                                    int width,
                                    const QFontMetrics& fm,
                                    const QSize& iconSize,
                                    int level) {

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

    //QSize iconSize = icon(COL_NAME).actualSize(QSize(22, 22));
    setSizeHint(COL_NAME, itemSize(playlistItem.name(),
                                   gNameColumnWidth,
                                   gNameFontMetrics,
                                   gIconSize,
                                   level));
}

} // namespace Playlist
} // namespace Gui
