#include "wztimer.h"
#include <QTimerEvent>

TWZTimer::TWZTimer(QObject* parent, const QString& name) : QTimer(parent) {

    setObjectName(name);
}

void TWZTimer::startVoid(){
    WZTRACEOBJ(QString("Starting with interval %1 ms").arg(interval()));
    start();
}

void TWZTimer::timerEvent(QTimerEvent *e) {

    if (e->timerId() == timerId()) {
        WZTRACEOBJ(QString("Timeout with interval %1 ms").arg(interval()));
        QTimer::timerEvent(e);
    }
}
