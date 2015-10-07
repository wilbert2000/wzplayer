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

#ifndef _PREF_VDPAUPROPERTIES_H_
#define _PREF_VDPAUPROPERTIES_H_

#include <QDialog>
#include "ui_vdpauproperties.h"

namespace Gui { namespace Pref {

class TVDPAUProperties : public QDialog, public Ui::TVDPAUProperties {
	Q_OBJECT

public:
	TVDPAUProperties(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~TVDPAUProperties();

	void setffh264vdpau(bool b);
	void setffmpeg12vdpau(bool b);
	void setffwmv3vdpau(bool b);
	void setffvc1vdpau(bool b);
	void setffodivxvdpau(bool b);

	void setDisableFilters(bool b);

	bool ffh264vdpau();
	bool ffmpeg12vdpau();
	bool ffwmv3vdpau();
	bool ffvc1vdpau();
	bool ffodivxvdpau();

	bool disableFilters();
};

}} // namespace Gui::Pref

#endif // _PREF_VDPAUPROPERTIES_H_
