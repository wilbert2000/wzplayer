#include "wztime.h"
#include <QApplication>
#include <QLocale>


static QString negativeSign = QLocale().negativeSign();
static QString decimalPoint = QLocale().decimalPoint();

// Format time as [hh:]mm:ss
QString TWZTime::formatTimeSec(int secs) {

    QString negative;
    if (secs < 0) {
        secs = -secs;
        negative = negativeSign;
    }

    int hours = 0;
    if (secs >= 3600) {
        hours = secs / 3600;
        secs -= hours * 3600;
    }
    int minutes = secs / 60;
    secs -= minutes * 60;

    if (hours == 0) {
        return QString("%1%2:%3")
                .arg(negative)
                .arg(minutes, 2, 10, QChar('0'))
                .arg(secs, 2, 10, QChar('0'));
    }

    return QString("%1%2:%3:%4")
            .arg(negative)
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
}

// Format time as hh:mm:ss.zzz
QString TWZTime::formatTimeMS(const double& aSecs,
                              bool wantMinutes,
                              bool zeroPadded) {

    int ms = qRound(aSecs * 1000);
    QString negative;
    if (ms < 0) {
        ms = -ms;
        negative = negativeSign;
    }

    int hours = ms / 3600000;
    ms -= hours * 3600000;
    int minutes = ms / 60000;
    ms -= minutes * 60000;
    int secs = ms / 1000;
    ms -= secs * 1000;

    if (zeroPadded) {
        return QString("%1%2:%3:%4%5%6")
                .arg(negative)
                .arg(hours, 2, 10, QChar('0'))
                .arg(minutes, 2, 10, QChar('0'))
                .arg(secs, 2, 10, QChar('0'))
                .arg(decimalPoint)
                .arg(ms, 3, 10, QChar('0'));
    }

    QString mSecs;
    if (ms) {
        mSecs = QString("%1%2")
                .arg(decimalPoint)
                .arg(ms, 3, 10, QChar('0'));
    }

    if (hours) {
        return QString("%1%2:%3:%4%5")
                .arg(negative)
                .arg(hours)
                .arg(minutes, 2, 10, QChar('0'))
                .arg(secs, 2, 10, QChar('0'))
                .arg(mSecs);
    }

    if (wantMinutes) {
        return QString("%1%2:%3%4%5")
                .arg(negative)
                .arg(minutes, 2, 10, QChar('0'))
                .arg(secs, 2, 10, QChar('0'))
                .arg(decimalPoint)
                .arg(ms, 3, 10, QChar('0'));
    }

    if (minutes) {
        return QString("%1%2:%3%4")
                .arg(negative)
                .arg(minutes)
                .arg(secs, 2, 10, QChar('0'))
                .arg(mSecs);
    }

    return qApp->translate("TWZTime", "%1%2%3 seconds")
            .arg(negative)
            .arg(secs)
            .arg(mSecs);
}
