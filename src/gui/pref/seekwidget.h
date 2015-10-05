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

#ifndef _GUI_PREF_SEEKWIDGET_H_
#define _GUI_PREF_SEEKWIDGET_H_

#include "ui_seekwidget.h"
#include <QPixmap>
#include <QString>

namespace Gui { namespace Pref {

class TSeekWidget : public QWidget, public Ui::TSeekWidget
{
    Q_OBJECT
	Q_PROPERTY(QPixmap icon READ icon WRITE setIcon)
	Q_PROPERTY(QString label READ label WRITE setLabel)
	Q_PROPERTY(int time READ time WRITE setTime)

public:
	TSeekWidget( QWidget* parent = 0, Qt::WindowFlags f = 0 );
	virtual ~TSeekWidget();

	int time() const;
	const QPixmap * icon() const;
	QString label() const;

public slots:
	void setIcon(QPixmap icon);
	void setLabel(QString text);
	void setTime(int secs);

};

}} // namespace Gui::Pref

#endif // _GUI_PREF_SEEKWIDGET_H_
