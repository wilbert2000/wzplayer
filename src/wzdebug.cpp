#include "wzdebug.h"


TWZDebug::TWZDebug(Log4Qt::Logger* aLogger) :
    QDebug(&msg),
    level(Log4Qt::Level::DEBUG_INT),
    logger(aLogger) {
}

TWZDebug& operator << (TWZDebug&, TWZDebug& dest) {

    dest.logger->log(dest.level, dest.msg);
    dest.msg = "";
    return dest;
}

