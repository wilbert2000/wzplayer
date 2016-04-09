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

#ifndef GUI_DEFAULT_H
#define GUI_DEFAULT_H

#include "gui/baseplus.h"


class QLabel;
class QToolBar;
class QPushButton;
class QResizeEvent;
class QMenu;


namespace Gui {

class TDefault : public TBasePlus {
	Q_OBJECT

public:
	TDefault();
	virtual ~TDefault();

	virtual void loadConfig();
	virtual void saveConfig();

protected:
	virtual QString settingsGroupName() { return "default_gui"; }

protected slots:
	// Override
	virtual void onMediaInfoChanged();

	void displayTime(QString text);
	void displayFrame(int frame);
	void displayABSection();
	void displayVideoInfo();

protected:
	QLabel* time_display;
	QLabel* frame_display;
	QLabel* ab_section_display;
	QLabel* video_info_display;

	Action::TAction* viewFrameCounterAct;
	Action::TAction* viewVideoInfoAct;

	int last_second;

private:
	void createStatusBar();
	void createActions();
	void createMenus();
};

} // namespace GUI

#endif // GUI_DEFAULT_H
