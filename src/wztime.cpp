#include "wztime.h"


// Format time as [hh:]mm:ss
QString TWZTime::formatTime(int secs) {

    QString negative;
    if (secs < 0) {
        secs = -secs;
        negative = "-";
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
QString TWZTime::formatTimeMS(int ms) {

    QString negative;
    if (ms < 0) {
        ms = -ms;
        negative = "-";
    }

    int hours = ms / 3600000;
    ms -= hours * 3600000;
    int minutes = ms / 60000;
    ms -= minutes * 60000;
    int secs = ms / 1000;
    ms -= secs * 1000;

    return QString("%1%2:%3:%4.%5")
            .arg(negative)
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'))
            .arg(ms, 3, 10, QChar('0'));
}
