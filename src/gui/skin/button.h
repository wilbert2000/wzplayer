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

#ifndef GUI_SKIN_MYBUTTON_H
#define GUI_SKIN_MYBUTTON_H

#include <QAbstractButton>
#include "gui/action/action.h"
#include "gui/skin/icon.h"

namespace Gui {
namespace Skin {

class TButton : public QAbstractButton
{
Q_OBJECT
public:
	explicit TButton(QWidget *parent = 0);
    void setState(bool on) {state = on; }
	void setIcon(TIcon p_icon) { icon = p_icon; }
	TIcon getIcon() { return icon;}
	void setAction(TAction* pAction);
    bool eventFilter(QObject *watched, QEvent *event);

private:
	TIcon icon;
    bool mouseHover;
    bool state;
	TAction* action;

protected:
	virtual void paintEvent(QPaintEvent *);
    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);


signals:

public slots:
    void toggleImage();

};

} // namesapce Skin
} // namespace Gui

#endif // GUI_SKIN_MYBUTTON_H
