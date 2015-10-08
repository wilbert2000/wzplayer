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

#ifndef _GUI_MINI_H_
#define _GUI_MINI_H_

#include "gui/guiconfig.h"
#include "gui/baseedit.h"
#include "gui/editabletoolbar.h"
#include "gui/autohidewidget.h"


namespace Gui {

class TMini : public TBaseEdit {
	Q_OBJECT

public:
	TMini();
	virtual ~TMini();

	virtual void loadConfig(const QString& gui_group);
	virtual void saveConfig(const QString& gui_group);

protected slots:
	virtual void togglePlayAction(TCore::State state);

protected:
	virtual void retranslateStrings();
	virtual QMenu* createPopupMenu();

	virtual void aboutToEnterFullscreen();
	virtual void aboutToExitFullscreen();
	virtual void aboutToEnterCompactMode();
	virtual void aboutToExitCompactMode();
};

} // namespace Gui

#endif // _GUI_MINI_H_
