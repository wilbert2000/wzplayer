#include "gui/playlist/playlistwidgetitem.h"

#include <QDebug>
#include <QString>
#include <QTime>

#include "images.h"
#include "helper.h"

namespace Gui {
namespace Playlist {


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
        qDebug() << "Gui::Playlist::TPlaylistItem::setState: stamped"
                 << _playedTime << "on" << _filename;
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

QSize gIconSize;
int gNameColumnWidth = 0;
QFontMetrics* gNameFontMetrics = 0;

QIcon folderIcon;
QIcon notPlayedIcon;
QIcon okIcon;
QIcon loadingIcon;
QIcon playIcon;
QIcon failedIcon;


TPlaylistWidgetItem::TPlaylistWidgetItem(QTreeWidgetItem* parent,
                                         QTreeWidgetItem* after,
                                         const QString& filename,
                                         const QString& name,
                                         double duration,
                                         bool isDir) :
    QTreeWidgetItem(parent, after),
    playlistItem(filename, name, duration, isDir) {

    Qt::ItemFlags flags = Qt::ItemIsSelectable
                          | Qt::ItemIsDragEnabled
                          | Qt::ItemIsEnabled;
    if (isDir) {
        setFlags(flags | Qt::ItemIsDropEnabled);
        setIcon(COL_PLAY, folderIcon);
    } else {
        // TODO:
        //setFlags(flags | Qt::ItemIsEditable);
        setFlags(flags);
        setIcon(COL_PLAY, notPlayedIcon);
    }

    setTextAlignment(COL_NAME, NAME_TEXT_ALIGN);
    setName(playlistItem.name());

    setDuration(duration);
    setToolTip(COL_TIME, filename);
}

TPlaylistWidgetItem::~TPlaylistWidgetItem() {
}

int TPlaylistWidgetItem::getLevel() const {

    if (parent() == 0) {
        return 2;
    }
    return static_cast<TPlaylistWidgetItem*>(parent())->getLevel() + 1;
}

void TPlaylistWidgetItem::setState(TPlaylistItemState state) {
    qDebug() << "Gui::TPlaylistWidgetItem::setState:"
             << filename() << state;

    playlistItem.setState(state);

    switch (state) {
        case PSTATE_STOPPED:
            if (playlistItem.played()) {
                setIcon(COL_PLAY, okIcon);
            } else {
                setIcon(COL_PLAY, notPlayedIcon);
            }
            break;
        case PSTATE_LOADING:
            setIcon(COL_PLAY, loadingIcon);
            break;
        case PSTATE_PLAYING:
            setIcon(COL_PLAY, playIcon);
            break;
        case PSTATE_FAILED:
            setIcon(COL_PLAY, failedIcon);
            break;
    }
}

void TPlaylistWidgetItem::setName(const QString& name) {

    playlistItem.setName(name);
    setText(COL_NAME, name);
    setSzHint(getLevel());
    setToolTip(COL_NAME, name);
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
            setIcon(COL_PLAY, okIcon);
        } else {
            setIcon(COL_PLAY, notPlayedIcon);
        }
    }
}

QSize TPlaylistWidgetItem::itemSize(const QString& text,
                                    const QFontMetrics& fm,
                                    int level) {

    const int hm = 4;
    const int vm = 2; // 2 * vertical margin

    int w = gNameColumnWidth - level * (gIconSize.width() + hm) - 2 * hm;
    if (w <= 32) {
        return QSize(gNameColumnWidth, gIconSize.height());
    }

    int maxh = 4 * fm.lineSpacing();
    QRect r = QRect(QPoint(), QSize(w, maxh));
    QRect br = fm.boundingRect(r, NAME_TEXT_ALIGN, text);

    int h = br.height() + vm;
    if (h < gIconSize.height()) {
        h = gIconSize.height();
    }

    return QSize(gNameColumnWidth, h);
};

void TPlaylistWidgetItem::setSzHint(int level) {

    setSizeHint(COL_NAME,
                itemSize(playlistItem.name(), *gNameFontMetrics, level));
}


} // namespace Playlist
} // namespace Gui
