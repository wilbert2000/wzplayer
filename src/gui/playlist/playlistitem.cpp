#include "gui/playlist/playlistitem.h"

#include "log4qt/logger.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QTime>

#include "config.h"
#include "name.h"
#include "extensions.h"
#include "wzdebug.h"


namespace Gui {
namespace Playlist {

// TODO: find the file sys func reporting case
Qt::CaseSensitivity caseSensitiveFileNames =
        #ifdef Q_OS_WIN
                    Qt::CaseInsensitive;
        #else
                    Qt::CaseSensitive;
        #endif

int itemOrder = 0;

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
    mOrder(++itemOrder),
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
    mOrder(item.order()),
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
                             bool protectName) :
    mOrder(++itemOrder),
    mFilename(filename),
    mBaseName(name),
    mDuration(duration),
    mState(PSTATE_STOPPED),
    mPlayed(false),
    mEdited(protectName),
    mPlayedTime(0) {

    if (mBaseName.isEmpty()) {
        // setFileInfo removes the extension
        mBaseName = TName::nameForURL(mFilename);
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
        WZDEBUG(QString("stamped %1 on '%2'").arg(mPlayedTime).arg(mFilename));
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
    if (mFilename.isEmpty() || fi.isDir()) {
        mExt = "";
        mFolder = true;
        mWZPlaylist = false;
    } else {
        mExt = fi.suffix().toLower();

        // Remove extension from base name
        if (!mExt.isEmpty() && mBaseName.endsWith(mExt, Qt::CaseInsensitive)) {
            mBaseName = mBaseName.left(mBaseName.length() - mExt.length() - 1);
        }

        mFolder = mPlaylist;
        mWZPlaylist = fi.fileName().compare(TConfig::WZPLAYLIST,
                                            caseSensitiveFileNames) == 0;
        if (mWZPlaylist) {
            mBaseName = fi.dir().dirName();
        }
    }
}

void TPlaylistItem::setFilename(const QString &fileName,
                                const QString& baseName) {

    mFilename = fileName;
    mBaseName = baseName;
    setFileInfo();
}

bool TPlaylistItem::blacklisted(const QString& filename) const {
    return mBlacklist.contains(filename, caseSensitiveFileNames);
}

bool TPlaylistItem::whitelist(const QString& filename) {

    int i = mBlacklist.indexOf(QRegExp(filename, caseSensitiveFileNames,
                                       QRegExp::FixedString));
    if (i >= 0) {
        WZDEBUG("removing '" + filename + "' from blacklist");
        mBlacklist.removeAt(i);
        return true;
    }

    WZWARN("'" + filename + "' not found in blacklist");
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
