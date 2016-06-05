#include "gui/playlist/playlistitem.h"

#include "log4qt/logger.h"

#include <QTime>
#include <QUrl>
#include <QFileInfo>
#include <QApplication>

#include "helper.h"
#include "extensions.h"


namespace Gui {
namespace Playlist {

LOG4QT_DECLARE_STATIC_LOGGER(logger, Gui::Playlist::TPlaylistItem)


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
        logger()->debug("setState: stamped %1 on '%2'", mPlayedTime, mFilename);
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
        logger()->debug("whitelist: removed '%1' from blacklist", filename);
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
    return item.filename() == mFilename;
}

} // namespace Playlist
} // namespace Gui