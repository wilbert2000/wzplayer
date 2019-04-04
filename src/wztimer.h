#ifndef WZTIMER_H
#define WZTIMER_H

#include "wzdebug.h"
#include <QTimer>

// Class to get rid of name class between slots start() and start(int) in QTimer
class TWZTimer : public QTimer {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER
public:
    TWZTimer(QObject* parent, const QString& name);
public slots:
    void startVoid();
protected:
    void timerEvent(QTimerEvent *e) override;
};

#endif // WZTIMER_H
