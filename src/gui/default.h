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

#ifndef _GUI_DEFAULT_H_
#define _GUI_DEFAULT_H_

#include <QPoint>

#include "gui/guiconfig.h"
#include "gui/baseplus.h"
#include "gui/editabletoolbar.h"
#include "gui/autohidewidget.h"

class QToolBar;
class QPushButton;
class QResizeEvent;
class QMenu;

class TSeekingButton;

namespace Gui {
	
class TDefault : public TBasePlus {
	Q_OBJECT

public:
	TDefault(QWidget* parent = 0, Qt::WindowFlags flags = 0);
	virtual ~TDefault();

	virtual void loadConfig(const QString& group);
	virtual void saveConfig(const QString& group);

protected:
	virtual void retranslateStrings();
	virtual QMenu * createPopupMenu();

	void createStatusBar();
	void createMainToolBars();
	void createControlWidget();
	void createControlWidgetMini();
	void createFloatingControl();
	void createActions();
	void createMenus();

    virtual void aboutToEnterFullscreen();
    virtual void aboutToExitFullscreen();
    virtual void aboutToEnterCompactMode();
    virtual void aboutToExitCompactMode();

	virtual void resizeEvent(QResizeEvent*);

protected slots:
	virtual void applyNewPreferences();
	virtual void displayTime(QString text);
	virtual void displayFrame(int frame);
	virtual void displayABSection(int secs_a, int secs_b);
	virtual void displayVideoInfo(int width, int height, double fps);

	virtual void enableActionsOnPlaying();
	virtual void disableActionsOnStop();
	virtual void togglePlayAction(TCore::State state);

	void adjustFloatingControlSize();

protected:
	void reconfigureFloatingControl();

protected:
	QLabel * time_display;
	QLabel * frame_display;
	QLabel * ab_section_display;
	QLabel * video_info_display;

	TEditableToolbar * controlwidget;
	TEditableToolbar * controlwidget_mini;

	TEditableToolbar * toolbar1;
	QToolBar * toolbar2;

	QPushButton * select_audio;
	QPushButton * select_subtitle;

	TTimeSliderAction * timeslider_action;
	TVolumeSliderAction * volumeslider_action;

	TSeekingButton * rewindbutton_action;
	TSeekingButton * forwardbutton_action;

	TAutohideWidget * floating_control;
	TTimeLabelAction * time_label_action;

	TAction * viewFrameCounterAct;
	TAction * viewVideoInfoAct;

	TAction * editToolbar1Act;
	TAction * editControl1Act;
	TAction * editControl2Act;
	TAction * editFloatingControlAct;

	QMenu * toolbar_menu;
	QMenu * statusbar_menu;

	int last_second;

	bool fullscreen_toolbar1_was_visible;
	bool fullscreen_toolbar2_was_visible;
	bool compact_toolbar1_was_visible;
	bool compact_toolbar2_was_visible;
};

} // namespace GUI

#endif // _GUI_DEFAULT_H_
