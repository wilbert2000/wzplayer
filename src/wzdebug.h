#ifndef WZDEBUG_H
#define WZDEBUG_H

#include <QDebug>
#include "log4qt/logger.h"
#include "log4qt/level.h"


/*
 * I don't get a simple thing working, that is to use:
 * logger()->debug() << msg;
 *
 * Out of despair this kludge is born.
 *
 * Usage:
 *
 * in header add:
 * DECLARE_QCLASS_LOGGER
 *
 * in constructor add:
 * debug(logger())
 *
 * for logging use:
 * debug << msg << debug
 * The first debug captures the msg, the second flushes it to the log
 *
 */

#define DECLARE_QCLASS_LOGGER \
    LOG4QT_DECLARE_QCLASS_LOGGER \
    TWZDebug debug; \
private:


class TWZDebug : public QDebug {
public:
    Log4Qt::Level level;
    QString msg;
    Log4Qt::Logger* logger;

    explicit TWZDebug(Log4Qt::Logger* aLogger);
    virtual ~TWZDebug();
};

TWZDebug& operator << (QDebug&, TWZDebug&);

#endif // WZDEBUG_H
