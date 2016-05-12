#include "wzdebug.h"

TWZDebug::TWZDebug(Log4Qt::Logger* aLogger) :
    QDebug(&msg),
    logger(aLogger) {
}

TWZDebug::~TWZDebug() {
}

TWZDebug& operator <<(QDebug&, TWZDebug& d) {

    d.logger->debug(d.msg);
    d.msg = "";
    return d;
}

