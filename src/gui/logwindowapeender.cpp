#include "gui/logwindowapeender.h"
#include "settings/preferences.h"
#include "log4qt/layout.h"
#include <QPlainTextEdit>

namespace Gui {

TLogWindowAppender::TLogWindowAppender(Log4Qt::Layout* aLayout) :
    Log4Qt::ListAppender(),
    textEdit(0),
    layout(aLayout) {
}

TLogWindowAppender::~TLogWindowAppender() {
}

void TLogWindowAppender::removeNewLine(QString& s) {
    s.chop(1);
}

void TLogWindowAppender::appendTextToEdit(QString s) {

    //QTextCursor prevCursor = textEdit->textCursor();
    //textEdit->moveCursor(QTextCursor::End);
    //textEdit->insertPlainText(s);
    //textEdit->setTextCursor(prevCursor);

    removeNewLine(s);
    QMetaObject::invokeMethod(textEdit, "appendPlainText", Qt::AutoConnection,
                              Q_ARG(QString, s));
}

void TLogWindowAppender::append(const Log4Qt::LoggingEvent& rEvent) {

    QMutexLocker locker(&mObjectGuard);

    Log4Qt::ListAppender::append(rEvent);

    // Shrink the list to log_window_max_events
    if (Settings::pref) {
        setMaxCount(Settings::pref->log_window_max_events);
        setMaxCount(0);
    }

    // Append text to edit
    if (textEdit) {
        appendTextToEdit(layout->format(rEvent));
    }
}

void TLogWindowAppender::setEdit(QPlainTextEdit* edit) {

    if (edit) {
        QMutexLocker locker(&mObjectGuard);

        QString s;
        foreach(const Log4Qt::LoggingEvent& rEvent, list()) {
            s += layout->format(rEvent);
        }
        removeNewLine(s);
        edit->setPlainText(s);
        edit->moveCursor(QTextCursor::End);
        textEdit = edit;
    } else if (textEdit) {
        edit = textEdit;
        {
            QMutexLocker locker(&mObjectGuard);
            textEdit = 0;
        }
        edit->clear();
        Log4Qt::Logger::logger("Gui::TLogWindowAppender")->debug(
                    "disconnected from log window");
    }
}

} // namespace Gui

