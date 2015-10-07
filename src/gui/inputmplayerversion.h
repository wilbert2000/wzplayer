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

#ifndef _GUI_INPUTMPLAYERVERSION_H_
#define _GUI_INPUTMPLAYERVERSION_H_

#include "ui_inputmplayerversion.h"

namespace Gui {

class TInputMplayerVersion : public QDialog, public Ui::TInputMplayerVersion
{
	Q_OBJECT

public:
	TInputMplayerVersion(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~TInputMplayerVersion();

	void setVersion(int current_version);
	int version();

	void setVersionFromOutput(QString text);
};

} // namespace Gui

#endif // _GUI_INPUTMPLAYERVERSION_H_
