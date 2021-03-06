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

    Note: The TShortcutGetter class is taken from the source code of Edyuk
    (http://www.edyuk.org/), from file 3rdparty/qcumber/qshortcutdialog.cpp

    Copyright (C) 2006 FullMetalCoder
    License: GPL

    I modified it to support multiple shortcuts and some other few changes.
*/


#ifndef GUI_SHORTCUTGETTER_H
#define GUI_SHORTCUTGETTER_H

#include <QDialog>
#include <QListWidget>
#include "wzdebug.h"

class QLineEdit;
class QLabel;

namespace Gui {
namespace Action {

class TActionsEditor;

class TShortcutGetter : public QDialog {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    TShortcutGetter(TActionsEditor* parent,
                    const QString& actName,
                    const QString& actOwner);

    QString exec(const QString& s);

protected:
    bool event(QEvent* e);
    bool eventFilter(QObject* o, QEvent* e);

private:
    QString actionName;
    QString actionOwner;
    bool bStop;
    bool capture;
    Qt::KeyboardModifiers modifiers;
    QStringList lKeys;

    QLineEdit* leKey;
    QListWidget* list;
    QLabel* assignedToLabel;
    QPushButton* addItem;
    QPushButton* removeItem;
    QPushButton* okButton;
    QString okButtonText;

    TActionsEditor* editor;

    void captureEvent(QEvent* e);
    void setText();

private slots:
    void setCaptureKeyboard(bool b);
    void rowChanged(int row);
    void onKeyTextChanged(const QString& text);

    void addItemClicked();
    void removeItemClicked();
}; // class TShortcutGetter

} // namespace Action
} // namespace Gui

#endif // GUI_SHORTCUTGETTER_H
