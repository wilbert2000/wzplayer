#include "wztimer.h"
#include <QTimerEvent>

TWZTimer::TWZTimer(QObject* parent, const QString& name, bool logEvents) :
    QTimer(parent),
    log(logEvents) {

    setObjectName(name);
}

void TWZTimer::logStart(){

    if (log) {
        WZTRACEOBJ(QString("Starting with interval %1 ms").arg(interval()));
    }
    start();
}

void TWZTimer::timerEvent(QTimerEvent *e) {

    if (e->timerId() == timerId()) {
        if (log) {
            WZTRACEOBJ(QString("Timeout with interval %1 ms").arg(interval()));
        }
        QTimer::timerEvent(e);
    }
}
