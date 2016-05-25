#include "wzdebug.h"

TWZDebug::TWZDebug(Log4Qt::Logger* aLogger) :
    QDebug(&msg),
    level(Log4Qt::Level::DEBUG_INT),
    logger(aLogger) {
}

TWZDebug::~TWZDebug() {
}

TWZDebug& operator << (QDebug&, TWZDebug& d) {

    d.logger->log(d.level, d.msg);
    d.msg = "";
    return d;
}

