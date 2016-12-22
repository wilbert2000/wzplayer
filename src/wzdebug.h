#ifndef WZDEBUG_H
#define WZDEBUG_H

#include <QDebug>
#include "log4qt/logger.h"
#include "log4qt/level.h"


#define WZTRACE(s) logger()->trace(QString(__FUNCTION__) + " " + (s))
#define WZDEBUG(s) logger()->debug(QString(__FUNCTION__) + " " + (s))
#define WZINFO(s) logger()->info(QString(__FUNCTION__) + " " + (s))
#define WZWARN(s) logger()->warn(QString(__FUNCTION__) + " " + (s))
#define WZERROR(s) logger()->error(QString(__FUNCTION__) + " " + (s))

/*
 * TODO: logger()->debug() << msg;
 *
 * Usage:
 * in header add:
 * DECLARE_QCLASS_LOGGER
 * in constructor add:
 * debug(logger())
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
