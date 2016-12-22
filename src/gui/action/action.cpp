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

#include "gui/action/action.h"
#include <QDebug>
#include <QEvent>
#include <QWidget>
#include "images.h"


namespace Gui {
namespace Action {

TAction::TAction (QObject* parent,
                  const QString& name,
                  const QString& text,
                  const QString& icon,
                  const QKeySequence& shortCut,
                  bool autoAdd)
    : QAction(parent) {

    setObjectName(name);
    // Set before calling setTextAndTip
    setShortcut(shortCut);
    setTextAndTip(text);
    QString iconName = icon.isEmpty() ? name : icon;
    if (!iconName.isEmpty() && iconName != "noicon")
        setIcon(Images::icon(iconName));
    if (autoAdd)
        addActionToParent();
}

TAction::~TAction() {
}

void TAction::addShortcut(QKeySequence key) {
    setShortcuts(shortcuts() << key);
}

void TAction::addActionToParent() {

    QWidget* w = qobject_cast<QWidget*>(parent());
    if (w) {
        w->addAction(this);
    }
}

void TAction::setTextAndTip(const QString& text) {

    setText(text);

    QString accel_text = shortcut().toString();
    if (!accel_text.isEmpty()) {
        QString s = text;
        s.replace("&", "");
        setToolTip(s + " (" + accel_text + ")");
    }
}

} // namespace Action
} // namespace Gui
