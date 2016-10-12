#include "gui/playlist/playlistitem.h"

#include "log4qt/logger.h"

#include <QApplication>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QTime>

#include "config.h"
#include "helper.h"
#include "extensions.h"


namespace Gui {
namespace Playlist {

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


LOG4QT_DECLARE_STATIC_LOGGER(logger, Gui::Playlist::TPlaylistItem)

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

// Copy constructor
TPlaylistItem::TPlaylistItem(const TPlaylistItem& item) :
    mFilename(item.filename()),
    mBaseName(item.baseName()),
    mExt(item.extension()),
    mDuration(item.duration()),
    mState(item.state()),
    mPlayed(item.played()),
    mEdited(item.edited()),
    mFolder(item.folder()),
    mPlaylist(item.playlist()),
    mWZPlaylist(item.wzPlaylist()),
    mSymLink(item.symLink()),
    mTarget(item.target()),
    mPlayedTime(item.playedTime()),
    mBlacklist(item.getBlacklist()) {
}

// Constructor from arguments
TPlaylistItem::TPlaylistItem(const QString &filename,
                             const QString &name,
                             double duration,
                             bool isFolder,
                             bool protectName) :
    mFilename(filename),
    mBaseName(name),
    mDuration(duration),
    mState(PSTATE_STOPPED),
    mPlayed(false),
    mEdited(protectName),
    mFolder(isFolder),
    mPlayedTime(0) {

    if (mBaseName.isEmpty()) {
        mBaseName = Helper::nameForURL(mFilename, false);
    }

    setFileInfo();
}

TPlaylistItem::~TPlaylistItem() {
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

void TPlaylistItem::setState(TPlaylistItemState state) {

    if (state == PSTATE_PLAYING) {
        mPlayed = true;
        mPlayedTime = timeStamper.getStamp();
        logger()->debug("setState: stamped %1 on '%2'", mPlayedTime, mFilename);
    }
    mState = state;
}

void TPlaylistItem::setBaseName(const QString& baseName, bool protectName) {

    mBaseName = baseName;
    if (protectName) {
        mEdited = true;
    }
}

// Update fields depending on file name
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

        // Remove extension from base name
        if (!mExt.isEmpty() && mBaseName.endsWith(mExt, Qt::CaseInsensitive)) {
            mBaseName = mBaseName.left(mBaseName.length() - mExt.length() - 1);
        }

        // Handle WZPlaylist
        mWZPlaylist = fi.fileName().compare(TConfig::WZPLAYLIST,
                                            caseSensitiveFileNames) == 0;
        if (mWZPlaylist) {
            mBaseName = fi.dir().dirName();
        }
    }
}

void TPlaylistItem::setFilename(const QString &filename) {

    mFilename = filename;
    setFileInfo();
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
