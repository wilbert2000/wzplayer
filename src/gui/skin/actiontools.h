/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

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

#ifndef GUI_SKIN_ACTIONTOOLS_H
#define GUI_SKIN_ACTIONTOOLS_H

#include "gui/skin/button.h"
#include <QAction>
#include <QList>

#define SETACTIONTOBUTTON(button, name) { TActionTools::setActionToButton(button, name, actions); }

namespace Gui {
namespace Skin {

class TActionTools {

public:

	static void setActionToButton(TButton * button, const QString & name, QList<QAction*> actions);
	static QAction * findAction(const QString & name, QList<QAction*> actions);
};

} // namespace Skin
} // namespace Gui

#endif // GUI_SKIN_ACTIONTOOLS_H

