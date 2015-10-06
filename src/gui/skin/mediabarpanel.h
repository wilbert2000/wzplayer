/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>
    umplayer, Copyright (C) 2010 Ori Rejwan

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

#ifndef GUI_SKIN_MEDIABARPANEL_H
#define GUI_SKIN_MEDIABARPANEL_H

#include <QWidget>
#include <QPushButton>
#include <QList>

#include "gui/skin/playcontrol.h"
#include "gui/skin/mediapanel.h"
#include "gui/skin/volumecontrolpanel.h"
#include "core.h"

#include "ui_mediabarpanel.h"

class TAction;

namespace Gui {
namespace Skin {

class TMediaBarPanel : public QWidget, public Ui::TMediaBarPanel {
	Q_OBJECT

public:
	TMediaBarPanel(QWidget* parent, TCore* c);
	virtual ~TMediaBarPanel();
	void setPlayControlActionCollection(QList<QAction*> actions);
	void setMediaPanelActionCollection(QList<QAction*> actions);
	void setVolumeControlActionCollection(QList<QAction*> actions);
	void setToolbarActionCollection(QList<QAction *>actions);
	void setRecordAvailable(bool av);
	void setVolume(int v);

protected:
	void changeEvent(QEvent *e);

private:
	TPlayControl* playControlPanel;
	TMediaPanel* mediaPanel;
	TVolumeControlPanel* volumeControlPanel;
	TCore* core;

	// Play Control
public slots:
	void gotCurrentTime(double time);
	void updateMediaInfo();
	void displayMessage(QString status, int time);
	void displayMessage(QString status);
	void displayPermanentMessage(QString status);
	void setBuffering();
	void setResolutionVisible(bool b);
	void setScrollingEnabled(bool b);
};

} // namesapce Skin
} // namespace Gui

#endif // GUI_SKIN_MEDIABARPANEL_H
