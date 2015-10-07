/*  Mpcgui for SMPlayer.
    Copyright (C) 2008 matt_ <matt@endboss.org>

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

#ifndef _GUI_MPC_H_
#define _GUI_MPC_H_

#include "gui/guiconfig.h"
#include "gui/baseplus.h"
#include "gui/autohidewidget.h"

class QSpacerItem;
class QToolBar;

namespace Gui {

class TMpc : public TBasePlus {
	Q_OBJECT

public:
	TMpc();
	virtual ~TMpc();

	virtual void loadConfig(const QString &group);
	virtual void saveConfig(const QString &group);

#if USE_MPCMUMSIZE
	virtual QSize mpcmumSizeHint () const;
#endif

protected slots:
	void muteIconChange(bool b);
	void iconChange(TCore::State state);
	void updateAudioChannels();

	void displayTime(QString text);
	void displayFrame(int frame);
	void showFullscreenControls();
	void hideFullscreenControls();
	void setJumpTexts();

protected:
	virtual void retranslateStrings();

	void createActions();
	void createControlWidget();
	void createFloatingControl();
	void createStatusBar();

	void setupIcons();

	// Reimplemented
	virtual void aboutToEnterFullscreen();
	virtual void aboutToExitFullscreen();
	virtual void aboutToEnterCompactMode();
	virtual void aboutToExitCompactMode();

protected:
	QToolBar* controlwidget;
	QToolBar* timeslidewidget;

	QLabel* audiochannel_display;
	QLabel* time_display;
	QLabel* frame_display;

	QLabel* floating_control_time;

	QSpacerItem* spacer;
};

} // namespace Gui

#endif // _GUI_MPC_H_
