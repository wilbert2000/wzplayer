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

#ifndef _GUI_SKIN_H_
#define _GUI_SKIN_H_

#include "gui/guiconfig.h"
#include "gui/action.h"
#include "gui/widgetactions.h"
#include "gui/autohidewidget.h"
#include "gui/baseplus.h"
#include "gui/skin/mediabarpanel.h"
#include "gui/editabletoolbar.h"

class QMenu;
class QPushButton;
class QToolBar;


//#define SKIN_EDITABLE_CONTROL 1

namespace Gui {

class TSkin : public TBasePlus
{
	Q_OBJECT

public:
	TSkin( QWidget* parent = 0, Qt::WindowFlags flags = 0 );
	virtual ~TSkin();

	virtual void loadConfig(const QString &group);
	virtual void saveConfig(const QString &group);

public slots:
	//virtual void showPlaylist(bool b);

protected:
	virtual void retranslateStrings();
	virtual QMenu * createPopupMenu();

	void createMainToolBars();
	void createControlWidget();
	void createFloatingControl();
	void createActions();
	void createMenus();

	virtual void aboutToEnterFullscreen();
	virtual void aboutToExitFullscreen();
	virtual void aboutToEnterCompactMode();
	virtual void aboutToExitCompactMode();

protected slots:
	virtual void displayTime(QString text);
	virtual void displayState(TCore::State state);
	virtual void displayMessage(QString message, int time);
	virtual void displayMessage(QString message);

	virtual void enableActionsOnPlaying();
	virtual void disableActionsOnStop();
	virtual void togglePlayAction(TCore::State);

protected:
	Skin::TMediaBarPanel* mediaBarPanel;
	QAction * mediaBarPanelAction;

	TEditableToolbar * toolbar1;
	QToolBar * controlwidget;

	TTimeSliderAction * timeslider_action;
	TVolumeSliderAction * volumeslider_action;

#if MINI_ARROW_BUTTONS
	TSeekingButton * rewindbutton_action;
	TSeekingButton * forwardbutton_action;
#endif

	TAutohideWidget * floating_control;
	TTimeLabelAction * time_label_action;

	TAction * editToolbar1Act;
#if defined(SKIN_EDITABLE_CONTROL)
	TAction * editFloatingControlAct;
#endif

	TAction * viewVideoInfoAct;
	TAction * scrollTitleAct;

	QMenu * toolbar_menu;
	QMenu * statusbar_menu;

	int last_second;

	bool fullscreen_toolbar1_was_visible;
	bool compact_toolbar1_was_visible;
};

} // namespace Gui

#endif // _GUI_SKIN_H_
