#ifndef WZDEBUG_H
#define WZDEBUG_H

#include <QDebug>
#include "log4qt/logger.h"
#include "log4qt/level.h"

#define WZLOG(level, s) logger()->log((level), "%1 %2", __FUNCTION__, (s))
#define WZLOGOBJ(level, s) logger()->log((level), "%1 (%2) %3", __FUNCTION__, \
    objectName(), (s))

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


class TWZDebug : public QDebug {
public:
    explicit TWZDebug(Log4Qt::Logger* aLogger, Log4Qt::Level aLevel);
    ~TWZDebug();
private:
    QString msg;
    Log4Qt::Logger* logger;
    Log4Qt::Level level;
};

/*
 * Usage:
 * in header add:
 * LOG4QT_DECLARE_QCLASS_LOGGER
 * after Q_OBJECT like:
 *
 * class Classname {
 *  Q_OBJECT
 *  LOG4QT_DECLARE_QCLASS_LOGGER
 *  public:
 *      ....
 * }
 *
 * for logging use:
 * WZDEBUG(msg);
 * or
 * WZD << msg << "more msg"
 * or
 * WZDOBJ << msg << "more msg"
 *
 */

#define WZT if (logger()->isTraceEnabled()) \
    TWZDebug(logger(), Log4Qt::Level::TRACE_INT) << __FUNCTION__
#define WZTOBJ if (logger()->isTraceEnabled()) \
    TWZDebug(logger(), Log4Qt::Level::TRACE_INT) << __FUNCTION__ \
    << qUtf8Printable("(" + objectName() + ")")

#define WZD if (logger()->isDebugEnabled()) \
    TWZDebug(logger(), Log4Qt::Level::DEBUG_INT) << __FUNCTION__
#define WZDOBJ  if (logger()->isDebugEnabled()) \
    TWZDebug(logger(), Log4Qt::Level::DEBUG_INT) << __FUNCTION__ \
    << qUtf8Printable("(" + objectName() + ")")

#define WZI if (logger()->isInfoEnabled()) \
    TWZDebug(logger(), Log4Qt::Level::INFO_INT) << __FUNCTION__
#define WZIOBJ if (logger()->isInfoEnabled()) \
    TWZDebug(logger(), Log4Qt::Level::INFO_INT) << __FUNCTION__ \
    << qUtf8Printable("(" + objectName() + ")")

#define WZW if (logger()->isWarnEnabled()) \
    TWZDebug(logger(), Log4Qt::Level::WARN_INT) << __FUNCTION__
#define WZWOBJ if (logger()->isWarnEnabled()) \
    TWZDebug(logger(), Log4Qt::Level::WARN_INT) << __FUNCTION__ \
    << qUtf8Printable("(" + objectName() + ")")

#define WZE if (logger()->isErrorEnabled()) \
    TWZDebug(logger(), Log4Qt::Level::ERROR_INT) << __FUNCTION__
#define WZEOBJ if (logger()->isErrorEnabled()) \
    TWZDebug(logger(), Log4Qt::Level::ERROR_INT) << __FUNCTION__ \
    << qUtf8Printable("(" + objectName() + ")")

#endif // WZDEBUG_H
