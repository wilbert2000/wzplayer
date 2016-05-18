/*  WZPlayer, GUI front-end for mplayer and MPV.
    Parts copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef GUI_LOGWINDOW_H
#define GUI_LOGWINDOW_H

#include "ui_logwindow.h"
#include "log4qt/logger.h"
#include "log4qt/varia/listappender.h"


class QPlainTextEdit;

namespace Log4Qt {
class TTCCLayout;
class LoggingEvent;
}

namespace Gui {

class TLogWindow;

class TLogWindowAppender : public Log4Qt::ListAppender {

public:
    TLogWindowAppender(QObject* pParent,
                       Log4Qt::TTCCLayout* alayout);
    virtual ~TLogWindowAppender();

    void setEdit(QPlainTextEdit* edit);

protected:
    virtual void append(const Log4Qt::LoggingEvent& rEvent);

private:
    QPlainTextEdit* textEdit;
    Log4Qt::TTCCLayout* layout;

    void appendTextToEdit(QString s);
};


class TLogWindow : public QWidget, public Ui::TLogWindow {
	Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    TLogWindow(QWidget* parent);
	virtual ~TLogWindow();

	virtual void loadConfig();
	virtual void saveConfig();

	void retranslateStrings();

    static TLogWindowAppender* appender;


signals:
	void visibilityChanged(bool visible);

protected:
	virtual void showEvent(QShowEvent*);
	virtual void hideEvent(QShowEvent*);
	virtual void closeEvent(QCloseEvent* event);

protected slots:
    void onCopyButtonClicked();
    void onSaveButtonClicked();
};

} // namespace Gui

#endif // GUI_LOGWINDOW_H
