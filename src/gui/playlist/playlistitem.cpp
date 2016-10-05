#include "gui/playlist/playlistitem.h"

#include "log4qt/logger.h"

#include <QTime>
#include <QUrl>
#include <QFileInfo>
#include <QApplication>

#include "helper.h"
#include "extensions.h"
#include "config.h"


namespace Gui {
namespace Playlist {

LOG4QT_DECLARE_STATIC_LOGGER(logger, Gui::Playlist::TPlaylistItem)

// TODO: find the file sys func reporting case
Qt::CaseSensitivity caseSensitiveFileNames =
        #ifdef Q_OS_WIN
                    Qt::CaseInsensitive;
        #else
                    Qt::CaseSensitive;
        #endif


// Generate a timestamp for played items
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

static TTimeStamp timeStamper;


// Default constructor
TPlaylistItem::TPlaylistItem() :
    mDuration(0),
    mState(PSTATE_STOPPED),
    mPlayed(false),
    mEdited(false),
    mFolder(false),
    mPlaylist(false),
    mWZPlaylist(false),
    mSymLink(false),
    mPlayedTime(0) {
}

// Constructor from data
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
    mEdited(protectName),
    mFolder(isFolder),
    mPlayedTime(0) {

    if (mName.isEmpty()) {
        mName = Helper::baseNameForURL(mFilename);
    }

    setFileInfo();
}

// Copy constructor
TPlaylistItem::TPlaylistItem(const TPlaylistItem& item) :
    mFilename(item.filename()),
    mName(item.name()),
    mDuration(item.duration()),
    mState(item.state()),
    mPlayed(item.played()),
    mEdited(item.edited()),
    mFolder(item.folder()),
    mPlaylist(item.playlist()),
    mWZPlaylist(item.wzPlaylist()),
    mSymLink(item.symLink()),
    mTarget(item.target()),
    mExt(item.extension()),
    mPlayedTime(item.playedTime()),
    mBlacklist(item.getBlacklist()) {
}

// static
QString TPlaylistItem::playlistItemState(TPlaylistItemState state) {

    switch (state) {
        case PSTATE_STOPPED: return "stopped";
        case PSTATE_LOADING: return "loading";
        case PSTATE_PLAYING: return "playing";
        case PSTATE_FAILED: ;
    }
    return "failed";
}

// Update file name related fields
void TPlaylistItem::setFileInfo() {

    QFileInfo fi(mFilename);
    mPlaylist = extensions.isPlaylist(fi);
    mSymLink = fi.isSymLink();
    mTarget = fi.symLinkTarget();
    if (mSymLink) {
        fi.setFile(fi.symLinkTarget());
    }
    if (fi.isDir()) {
        mExt = "";
        mWZPlaylist = false;
    } else {
        mExt = fi.suffix().toLower();
        if (!mExt.isEmpty() && mName.endsWith(mExt, Qt::CaseInsensitive)) {
            mName = mName.left(mName.length() - mExt.length() - 1);
        }
        mWZPlaylist = fi.fileName() == TConfig::WZPLAYLIST;
    }
}

void TPlaylistItem::setFilename(const QString &filename) {

    mFilename = filename;
    setFileInfo();
}

void TPlaylistItem::setName(const QString& name, bool protectName) {

    mName = name;
    if (protectName) {
        mEdited = true;
    }
}

void TPlaylistItem::setState(TPlaylistItemState state) {

    if (state == PSTATE_PLAYING) {
        mPlayed = true;
        mPlayedTime = timeStamper.getStamp();
        logger()->debug("setState: stamped %1 on '%2'", mPlayedTime, mFilename);
    }
    mState = state;
}

bool TPlaylistItem::blacklisted(const QString& filename) const {
    return mBlacklist.contains(filename, caseSensitiveFileNames);
}

bool TPlaylistItem::whitelist(const QString& filename) {

    int i = mBlacklist.indexOf(QRegExp(filename, caseSensitiveFileNames,
                                       QRegExp::FixedString));
    if (i >= 0) {
        logger()->debug("whitelist: removing '%1' from blacklist", filename);
        mBlacklist.removeAt(i);
        return true;
    }

    logger()->warn("whitelist: '%1' not found in blacklist", filename);
    return false;
}

bool TPlaylistItem::operator == (const TPlaylistItem& item) {

    if (&item == 0) {
        return false;
    }
    return item.filename().compare(mFilename, caseSensitiveFileNames) == 0;
}

} // namespace Playlist
} // namespace Gui
