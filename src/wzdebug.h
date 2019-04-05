#ifndef WZDEBUG_H
#define WZDEBUG_H

#include <QDebug>
#include "log4qt/logger.h"
#include "log4qt/level.h"


#define WZTRACE(s) logger()->trace(QString(__FUNCTION__) + " " + (s))
#define WZTRACEOBJ(s) logger()->trace(QString(__FUNCTION__) \
    + " (" + (objectName()) + ") " + (s))
#define WZDEBUG(s) logger()->debug(QString(__FUNCTION__) + " " + (s))
#define WZDEBUGOBJ(s) logger()->debug(QString(__FUNCTION__) \
    + " (" + (objectName()) + ") " + (s))
#define WZINFO(s) logger()->info(QString(__FUNCTION__) + " " + (s))
#define WZINFOOBJ(s) logger()->info(QString(__FUNCTION__) \
    + " (" + (objectName()) + ") " + (s))
#define WZWARN(s) logger()->warn(QString(__FUNCTION__) + " " + (s))
#define WZWARNOBJ(s) logger()->warn(QString(__FUNCTION__) \
    + " (" + (objectName()) + ") " + (s))
#define WZERROR(s) logger()->error(QString(__FUNCTION__) + " " + (s))
#define WZERROROBJ(s) logger()->error(QString(__FUNCTION__) \
    + " (" + (objectName()) + ") " + (s))

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
    TWZDebug wzdebug; \
private:


class TWZDebug : public QDebug {
public:
    QString msg;
    Log4Qt::Level level;
    Log4Qt::Logger* logger;

    explicit TWZDebug(Log4Qt::Logger* aLogger);
};

TWZDebug& operator << (TWZDebug&, TWZDebug&);

#endif // WZDEBUG_H
