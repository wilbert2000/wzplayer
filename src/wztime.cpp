#include "wztime.h"

// Format time as hh:mm:ss
QString TWZTime::formatTime(int secs) {

    QString negative;
    if (secs < 0) {
        secs = -secs;
        negative = "-";
    }

    int hours = 0;
    if (secs >= 3600) {
        hours = (int) secs / 3600;
        secs -= hours * 3600;
    }
    int minutes = (int) secs / 60;
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
