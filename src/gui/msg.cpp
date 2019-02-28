#include "msg.h"
#include "player/player.h"


namespace Gui {


static QStatusBar* statusbar = 0;

void setMessageHandler(QStatusBar* bar) {
    statusbar = bar;
}

void msg(const QString& msg, int timeout) {

    if (statusbar) {
        statusbar->showMessage(msg, timeout);
    }
}

void msgOSD(const QString& message, int timeout) {

    if (player) {
        player->displayTextOnOSD(message, timeout);
    }
    msg(message, timeout);
}

void msg2(const QString& message, int timeout) {

    if (Settings::pref->fullscreen) {
        msgOSD(message, timeout);
    } else {
        msg(message);
    }
}

void msgClear() {

    if (statusbar) {
        statusbar->clearMessage();
    }
}

TMsgSlot* msgSlot = 0;

TMsgSlot::TMsgSlot(QObject *parent) :
    QObject(parent) {
}

TMsgSlot::~TMsgSlot() {
}

void TMsgSlot::msg(const QString& msg, int timeout) {
    Gui::msg(msg, timeout);
}

void TMsgSlot::msgOSD(const QString& msg, int timeout) {
    Gui::msgOSD(msg, timeout);
}

} // namespace Gui
