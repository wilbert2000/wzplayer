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

#ifndef GUI_STEREO3DDIALOG_H
#define GUI_STEREO3DDIALOG_H

#include "ui_stereo3ddialog.h"
#include <QDialog>

namespace Gui {

class TStereo3dDialog : public QDialog, public Ui::TStereo3dDialog
{
	Q_OBJECT

public:
	TStereo3dDialog(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~TStereo3dDialog();

	void setInputFormat(const QString & in);
	void setOutputFormat(const QString & out);

	QString inputFormat();
	QString outputFormat();
};

} // namespace GUi

#endif // GUI_STEREO3DDIALOG_H