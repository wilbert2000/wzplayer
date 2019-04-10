#ifndef WZDEBUG_H
#define WZDEBUG_H

#include <QDebug>
#include "log4qt/logger.h"
#include "log4qt/level.h"

#define WZLOG(level, s) logger()->log((level), QString("%1 %2")\
    .arg(__FUNCTION__).arg(s))
#define WZLOGOBJ(level, s) logger()->log((level), QString("%1 (%2) %3")\
    .arg(__FUNCTION__).arg(objectName()).arg(s))

#define WZTRACE(s) WZLOG(Log4Qt::Level::TRACE_INT, s)
#define WZTRACEOBJ(s) WZLOGOBJ(Log4Qt::Level::TRACE_INT, s)
#define WZDEBUG(s) WZLOG(Log4Qt::Level::DEBUG_INT, s)
#define WZDEBUGOBJ(s) WZLOGOBJ(Log4Qt::Level::DEBUG_INT, s)
#define WZINFO(s) WZLOG(Log4Qt::Level::INFO_INT, s)
#define WZINFOOBJ(s) WZLOGOBJ(Log4Qt::Level::INFO_INT, s)
#define WZWARN(s) WZLOG(Log4Qt::Level::WARN_INT, s)
#define WZWARNOBJ(s) WZLOGOBJ(Log4Qt::Level::WARN_INT, s)
#define WZERROR(s) WZLOG(Log4Qt::Level::ERROR_INT, s)
#define WZERROROBJ(s) WZLOGOBJ(Log4Qt::Level::ERROR_INT, s)

/*
 *
 * Usage:
 * in header add:
 * DECLARE_QCLASS_LOGGER
 * in constructor add:
 * debug(logger())
 * for logging use:
 * debug << msg << "more msg"
 * debug.flush();
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

    void flush();
};

TWZDebug& operator << (TWZDebug&, TWZDebug&);

#endif // WZDEBUG_H
