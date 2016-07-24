#ifndef GUI_MSG_H
#define GUI_MSG_H

#include <QStatusBar>
#include "config.h"


class TCore;

namespace Gui {

extern void msg(const QString& msg, int timeout = TConfig::MESSAGE_DURATION);
extern void setMessageHandler(QStatusBar* bar);

extern void msgOSD(const QString& message,
                   int timeout = TConfig::MESSAGE_DURATION);
extern void setOSDMessageHandler(TCore* c);


class TMsgSlot : public QObject {
    Q_OBJECT

public:
    explicit TMsgSlot(QObject* parent);
    virtual ~TMsgSlot();

public slots:
    void msg(const QString &msg, int timeout = TConfig::MESSAGE_DURATION);
    void msgOSD(const QString &msg, int timeout = TConfig::MESSAGE_DURATION);
};

extern TMsgSlot* msgSlot;

} // namespace Gui

#endif // GUI_MSG_H
