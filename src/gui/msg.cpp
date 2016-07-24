#include "msg.h"
#include "core.h"


namespace Gui {


QStatusBar* statusbar = 0;

void msg(const QString& msg, int timeout) {

    if (statusbar) {
        statusbar->showMessage(msg, timeout);
    }
}

void setMessageHandler(QStatusBar* bar) {
    statusbar = bar;
}


TCore* core = 0;

void msgOSD(const QString& message, int timeout) {

    if (core) {
        core->displayTextOnOSD(message, timeout);
    }
    msg(message, timeout);
}

void setOSDMessageHandler(TCore* c) {
    core = c;
}


TMsgSlot* msgSlot = 0;

TMsgSlot::TMsgSlot(QObject *parent) :
    QObject(parent) {
}

TMsgSlot::~TMsgSlot() {
}

void TMsgSlot::msg(const QString &msg, int timeout) {
    Gui::msg(msg, timeout);
}

void TMsgSlot::msgOSD(const QString &msg, int timeout) {
    Gui::msgOSD(msg, timeout);
}

} // namespace Gui
