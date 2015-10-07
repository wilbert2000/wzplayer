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

#ifndef _PREF_DRIVES_H_
#define _PREF_DRIVES_H_

#include "ui_drives.h"
#include "settings/preferences.h"
#include "gui/pref/widget.h"
#include "config.h"


namespace Gui { namespace Pref {

class TDrives : public TWidget, public Ui::TDrives
{
	Q_OBJECT

public:
	TDrives(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~TDrives();

	virtual QString sectionName();
	virtual QPixmap sectionIcon();

    // Pass data to the dialog
	void setData(Settings::TPreferences* pref);

    // Apply changes
	void getData(Settings::TPreferences* pref);

protected:
	virtual void createHelp();

	void setDVDDevice(QString dir);
	QString dvdDevice();

	void setBlurayDevice(QString dir);
	QString blurayDevice();

	void setCDRomDevice(QString dir);
	QString cdromDevice();

	void setUseDVDNav(bool b);
	bool useDVDNav();

	void updateDriveCombos(bool detect_cd_devices = false);

protected slots:
	void on_check_drives_button_clicked();

protected:
	virtual void retranslateStrings();
};

}} // namespace Gui::Pref

#endif // _PREF_DRIVES_H_
