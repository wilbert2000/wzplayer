#include "wzdebug.h"


TWZDebug::TWZDebug(Log4Qt::Logger* aLogger) :
    QDebug(&msg),
    logger(aLogger) {
}

TWZDebug::~TWZDebug() {
    logger->log(Log4Qt::Level::DEBUG_INT, msg);
}
