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

#ifndef GUI_SKIN_H
#define GUI_SKIN_H

#include "gui/action/action.h"
#include "gui/baseplus.h"
#include "gui/skin/mediabarpanel.h"

class QMenu;
class QPushButton;
class QToolBar;


namespace Gui {

class TSkin : public TBasePlus
{
	Q_OBJECT

public:
	TSkin();
	virtual ~TSkin();

	virtual void loadConfig();
	virtual void saveConfig();

protected:
	virtual QString settingsGroupName() { return "skin_gui"; }

	void createControlWidget();
	void createActions();

protected slots:
	virtual void onStateChanged(TCore::State state);
	virtual void displayMessage(QString message, int time);
	virtual void displayMessage(QString message);

protected:
	Skin::TMediaBarPanel* mediaBarPanel;
	QAction* mediaBarPanelAction;

	TAction* playOrPauseAct;
	TAction* viewVideoInfoAct;
	TAction* scrollTitleAct;

	int last_second;

private:
	void createMenus();
};

} // namespace Gui

#endif // GUI_SKIN_H
