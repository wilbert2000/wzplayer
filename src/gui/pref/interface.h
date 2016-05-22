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

#ifndef PREF_INTERFACE_H
#define PREF_INTERFACE_H

#include "ui_interface.h"
#include "gui/pref/widget.h"
#include "settings/preferences.h"
#include "wzdebug.h"


namespace Settings{
class TPreferences;
}

namespace Gui { namespace Pref {

class TInterface : public TWidget, public Ui::TInterface {
	Q_OBJECT
    DECLARE_QCLASS_LOGGER

public:
	TInterface(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~TInterface();

	virtual QString sectionName();
	virtual QPixmap sectionIcon();

    // Pass data to the dialog
	void setData(Settings::TPreferences* pref);

    // Apply changes
	void getData(Settings::TPreferences* pref);

	bool languageChanged() const { return language_changed; }
	bool iconsetChanged() const { return iconset_changed; }
	bool styleChanged() const { return style_changed; }
	bool recentsChanged() const { return recents_changed; }
	bool urlMaxChanged() const { return url_max_changed; }

	void setDirectoryRecursion(bool b);
	bool directoryRecursion();

    void setMediaToAddToPlaylist(Settings::TPreferences::TAddToPlaylist type);
    Settings::TPreferences::TAddToPlaylist mediaToAddToPlaylist();

protected:
	void createLanguageCombo();

	void setLanguage(const QString& lang);
	QString language();

	void setIconSet(const QString& set);
	QString iconSet();

	void setSaveSize(bool b);
	bool saveSize();

	void setStyle(const QString& style);
	QString style();

	void setUseSingleInstance(bool b);
	bool useSingleInstance();

	void setHideVideoOnAudioFiles(bool b);
	bool hideVideoOnAudioFiles();

	// History tab
	void setRecentsMaxItems(int n);
	int recentsMaxItems();

	void setURLMaxItems(int n);
	int urlMaxItems();

	void setRememberDirs(bool b);
	bool rememberDirs();

	virtual void retranslateStrings();

protected slots:
    void changeInstanceImages();

private:
	bool language_changed;
	bool iconset_changed;
	bool style_changed;
	bool recents_changed;
	bool url_max_changed;

	void createHelp();

	void setPauseWhenHidden(bool b);
	bool pauseWhenHidden();

	void setCloseOnFinish(bool b);
	bool closeOnFinish();

	void setStartInFullscreen(bool b);
	bool startInFullscreen();

	// Log options
    void setLogLevel(Log4Qt::Level level);
    Log4Qt::Level logLevel();

	void setLogVerbose(bool b);
	bool logVerbose();
};

}} // namespace Gui::Pref

#endif // PREF_INTERFACE_H
