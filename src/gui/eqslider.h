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


#ifndef GUI_EQSLIDER_H
#define GUI_EQSLIDER_H

#include "ui_eqslider.h"
#include <QPixmap>

namespace Gui {

class TEqSlider : public QWidget, public Ui::TEqSlider
{
	Q_OBJECT
	Q_PROPERTY(QPixmap icon READ icon WRITE setIcon)
	Q_PROPERTY(QString label READ label WRITE setLabel)
	Q_PROPERTY(int value READ value WRITE setValue)

public:
	TEqSlider(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~TEqSlider();

	int value() const;
	const QPixmap* icon() const;
	QString label() const;

	QSlider* sliderWidget() { return _slider; }
	TVerticalText* labelWidget() { return _label; }
	QLabel* iconWidget() { return _icon; }

public slots:
	void setIcon(QPixmap i);
	void setLabel(const QString& s);
	void setValue(int value);

signals:
	void valueChanged(int);

protected slots:
	void onValueChanged(int);
};

} // namespace Gui

#endif // GUI_EQSLIDER_H
