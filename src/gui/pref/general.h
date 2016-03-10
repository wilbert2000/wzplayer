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

#ifndef GUI_PREF_GENERAL_H
#define GUI_PREF_GENERAL_H

#include "ui_general.h"
#include "gui/pref/widget.h"


namespace Settings {
class TPreferences;
}

namespace Gui {
namespace Pref {

class TGeneral : public TWidget, public Ui::TGeneral {
	Q_OBJECT

public:
	TGeneral(QWidget* parent);
	virtual ~TGeneral();

	// Return the name of the section
	virtual QString sectionName();
	// Return the icon of the section
	virtual QPixmap sectionIcon();

	// Pass data to the dialog
	void setData(Settings::TPreferences* pref);

	// Apply changes
	void getData(Settings::TPreferences* pref);

signals:
	void binChanged(const QString& path);

protected:
	virtual void retranslateStrings();

private:
	virtual void createHelp();

	// Tab General
	void setPlayerPath(const QString& path);
	QString playerPath();

	// Media settings
	void setRememberSettings(bool b);
	bool rememberSettings();

	void setRememberTimePos(bool b);
	bool rememberTimePos();

	void setFileSettingsMethod(const QString& method);
	QString fileSettingsMethod();

	// Screenshots
	void setUseScreenshots(bool b);
	bool useScreenshots();

	void setScreenshotDir(const QString& path);
	QString screenshotDir();

#ifdef MPV_SUPPORT
	void setScreenshotFormat(const QString& format);
	QString screenshotFormat();
#endif

	void setPauseWhenHidden(bool b);
	bool pauseWhenHidden();

	void setCloseOnFinish(bool b);
	bool closeOnFinish();


	// Preferred tab
	void setAudioLang(const QString& lang);
	QString audioLang();

	void setSubtitleLang(const QString& lang);
	QString subtitleLang();

private slots:
	void fileChanged(QString file);
};

} // namespace Pref
} // namespace Gui

#endif // GUI_PREF_GENERAL_H
