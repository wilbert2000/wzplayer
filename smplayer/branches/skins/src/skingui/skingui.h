/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2012 Ricardo Villalba <rvm@users.sourceforge.net>

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

#ifndef _SKINGUI_H_
#define _SKINGUI_H_

#include "guiconfig.h"
#include "baseguiplus.h"
#include <QPoint>

class QToolBar;
class EditableToolbar;
class QPushButton;
class QResizeEvent;
class MyAction;
class QMenu;
class TimeSliderAction;
class VolumeSliderAction;
class FloatingWidget;
class TimeLabelAction;
class MyAction;
class MediaBarPanel;

#if MINI_ARROW_BUTTONS
class SeekingButton;
#endif

class SkinGui : public BaseGuiPlus
{
	Q_OBJECT

public:
	SkinGui( QWidget* parent = 0, Qt::WindowFlags flags = 0 );
	~SkinGui();

public slots:
	//virtual void showPlaylist(bool b);

protected:
	virtual void retranslateStrings();
	virtual QMenu * createPopupMenu();

	void createStatusBar();
	void createMainToolBars();
	void createControlWidget();
	void createFloatingControl();
	void createActions();
	void createMenus();
	void changeIcons();

	void loadConfig();
	void saveConfig();

    virtual void aboutToEnterFullscreen();
    virtual void aboutToExitFullscreen();
    virtual void aboutToEnterCompactMode();
    virtual void aboutToExitCompactMode();

	virtual void resizeEvent( QResizeEvent * );
	/* virtual void closeEvent( QCloseEvent * ); */

protected slots:
	virtual void updateWidgets();
	virtual void displayTime(QString text);
	virtual void displayFrame(int frame);
	virtual void displayABSection(int secs_a, int secs_b);
	virtual void displayVideoInfo(int width, int height, double fps);

	virtual void showFloatingControl(QPoint p);
	virtual void showFloatingMenu(QPoint p);
	virtual void hideFloatingControls();

	// Reimplemented:
#if AUTODISABLE_ACTIONS
	virtual void enableActionsOnPlaying();
	virtual void disableActionsOnStop();
#endif

protected:
	QLabel * time_display;
	QLabel * frame_display;
	QLabel * ab_section_display;
	QLabel * video_info_display;

	MediaBarPanel* mediaBarPanel;

	EditableToolbar * toolbar1;

	TimeSliderAction * timeslider_action;
	VolumeSliderAction * volumeslider_action;

#if MINI_ARROW_BUTTONS
	SeekingButton * rewindbutton_action;
	SeekingButton * forwardbutton_action;
#endif

	FloatingWidget * floating_control;
	TimeLabelAction * time_label_action;

	MyAction * viewFrameCounterAct;
	MyAction * viewVideoInfoAct;

#if USE_CONFIGURABLE_TOOLBARS
	MyAction * editToolbar1Act;
	MyAction * editControl1Act;
	MyAction * editControl2Act;
	MyAction * editFloatingControlAct;
#endif

	QMenu * toolbar_menu;
	QMenu * statusbar_menu;

	int last_second;

	bool fullscreen_toolbar1_was_visible;
	bool compact_toolbar1_was_visible;
};

#endif
