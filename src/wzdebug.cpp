#include "wzdebug.h"


TWZDebug::TWZDebug(Log4Qt::Logger* aLogger, Log4Qt::Level aLevel) :
    QDebug(&msg),
    logger(aLogger),
    level(aLevel) {
}

TWZDebug::~TWZDebug() {
    logger->log(level, msg);
}
