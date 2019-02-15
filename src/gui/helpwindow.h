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

#ifndef GUI_HELPWINDOW_H
#define GUI_HELPWINDOW_H

#include "ui_helpwindow.h"


class QTextEdit;
class QSettings;

namespace Gui {

class THelpWindow : public QWidget, public Ui::THelpWindow {
    Q_OBJECT

public:
    THelpWindow(QWidget* parent, const QString& name);
    virtual ~THelpWindow();

    void loadSettings(QSettings* pref);
    void saveSettings(QSettings* pref);

    void setText(const QString& log);
    QString text();

    void setHtml(const QString& text);
    QString html();

    void clear();

    void appendText(const QString& text);
    void appendHtml(const QString& text);

signals:
    void visibilityChanged(bool visible);

protected:
    virtual void showEvent(QShowEvent*);
    virtual void hideEvent(QShowEvent*);
    virtual void closeEvent(QCloseEvent* event);

protected slots:
    void onCopyButtonClicked();
};

} // namespace Gui

#endif // GUI_HELPWINDOW_H
