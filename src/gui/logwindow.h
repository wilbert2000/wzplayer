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


namespace Gui {

class TLogWindowAppender;

class TLogWindow : public QWidget, public Ui::TLogWindow {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER
public:
    static TLogWindowAppender* appender;

    TLogWindow(QWidget* parent);
    virtual ~TLogWindow();

signals:
    void visibilityChanged(bool visible);

protected:
    virtual void showEvent(QShowEvent*);
    virtual void hideEvent(QShowEvent*);
    virtual void closeEvent(QCloseEvent* event);

private:
    void find(const QString& s, QTextDocument::FindFlags options);

private slots:
    void onSaveButtonClicked();
    void onCopyButtonClicked();
    void onFindPreviousButtonClicked();
    void onFindNextButtonClicked();
    void onFindTextChanged();
};

} // namespace Gui

#endif // GUI_LOGWINDOW_H
