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

#ifndef GUI_ACTION_EDITABLETOOLBAR_H
#define GUI_ACTION_EDITABLETOOLBAR_H

#include <QToolBar>
#include <QStringList>
#include <gui/action/actionlist.h>


namespace Gui {

class TBase;

namespace Action {

class TSizeGrip;
class TTimeSlider;


class TEditableToolbar : public QToolBar {
	Q_OBJECT

public:
	TEditableToolbar(TBase* mainwindow);
	virtual ~TEditableToolbar();

	void setActionsFromStringList(const QStringList& acts, const TActionList& all_actions);
	QStringList actionsToStringList(bool remove_size_grip = true);

	void setDefaultActions(const QStringList& action_names) { default_actions = action_names; }
	QStringList defaultActions() const { return default_actions; }

	virtual void setVisible(bool visible);

public slots:
	void edit();

protected:
	TBase* main_window;

	virtual void resizeEvent(QResizeEvent* event);
	virtual void moveEvent(QMoveEvent* event);
	virtual void enterEvent(QEvent* event);
	virtual void leaveEvent(QEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* e);

private:
	QStringList actions;
	QStringList default_actions;

    TSizeGrip* size_grip;
	TTimeSlider* space_eater;
	bool fixing_size;
	int fix_size;

    void addMenu(QAction* action);
    void addSizeGrip();
	void removeSizeGrip();

private slots:
	void showContextMenu(const QPoint& pos);
    void onTopLevelChanged(bool);
	void reload();
}; // class TEditableToolbar

} // namespace Action
} // namesapce Gui

#endif // GUI_ACTION_EDITABLETOOLBAR_H
