#include "gui/timelabel.h"
#include "player/player.h"
#include "wztime.h"
#include <QLocale>


namespace Gui {

TTimeLabel::TTimeLabel(QWidget* parent) :
    QLabel(parent),
    resolution(RES_SECONDS),
    lastSec(-1) {

    setFont(QFont("Monospace"));
    setDuration(0);
}

static QString decimalPoint = QLocale().decimalPoint();

QString TTimeLabel::getSuffix(int ms, int secs, const QString& fuzzyTime) {

    QString s;
    if (resolution != RES_SECONDS) {
        if (ms < 0) {
            ms = -ms;
            secs = -secs;
        }
        ms = ms - secs * 1000;
        if (resolution == RES_FRAMES && player->mdat.video_fps > 0) {
            int frame = qRound(ms * player->mdat.video_fps / 1000);
            if (frame < 10) s = ":0"; else s = ":";
            s += QString::number(frame) + fuzzyTime;
        } else {
            s = QString("%1%2").arg(decimalPoint).arg(ms, 3, 10, QChar('0'));
        }
    }
    return s;
}

void TTimeLabel::setPos(int ms, bool changed) {

    int s = ms / 1000;
    if (s != lastSec) {
        lastSec = s;
        positionText = TWZTime::formatSec(s);
        changed = true;
    }

    QString suf = getSuffix(ms, s, player->mdat.fuzzy_time);

    if (changed || !suf.isEmpty()) {
        setText(positionText + suf + durationText);
    }
}

void TTimeLabel::setPosition(int ms) {
    setPos(ms, false);
}

void TTimeLabel::setDuration(int ms) {

    if (resolution == RES_SECONDS) {
        durationText = "/" + TWZTime::formatSec(qRound(double(ms) / 1000));
    } else {
        int s = ms / 1000;
        durationText = "/" + TWZTime::formatSec(s) + getSuffix(ms, s, "");
    }
    setPos(player ? player->mdat.pos_sec_gui * 1000 : 0, true);
}

void TTimeLabel::setTimeResolution(int aResolution) {

    resolution = TTimeResolution(aResolution);
    setDuration(player->mdat.durationMS());
}

} // namespace Gui
