#include "wzdebug.h"


TWZDebug::TWZDebug(Log4Qt::Logger* aLogger) :
    QDebug(&msg),
    level(Log4Qt::Level::DEBUG_INT),
    logger(aLogger) {
}

void TWZDebug::flush() {

    logger->log(level, msg);
    msg = "";
}

TWZDebug& operator << (TWZDebug&, TWZDebug& dest) {

    dest.flush();
    return dest;
}

