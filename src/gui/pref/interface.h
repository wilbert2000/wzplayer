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

#ifndef PREF_INTERFACE_H
#define PREF_INTERFACE_H

#include "ui_interface.h"
#include "settings/preferences.h"
#include "gui/pref/widget.h"
#include "config.h"


namespace Gui { namespace Pref {

class TInterface : public TWidget, public Ui::TInterface {
	Q_OBJECT

public:
	TInterface(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~TInterface();

	virtual QString sectionName();
	virtual QPixmap sectionIcon();

    // Pass data to the dialog
	void setData(Settings::TPreferences* pref);

    // Apply changes
	void getData(Settings::TPreferences* pref);

	bool languageChanged() { return language_changed; }
	bool iconsetChanged() { return iconset_changed; }
	bool styleChanged() { return style_changed; }
	bool recentsChanged() { return recents_changed; }
	bool urlMaxChanged() { return url_max_changed; }

protected:
	virtual void createHelp();
	void createLanguageCombo();

	void setLanguage(QString lang);
	QString language();

	void setIconSet(QString set);
	QString iconSet();

	void setResizeMethod(int v);
	int resizeMethod();

	void setSaveSize(bool b);
	bool saveSize();

	void setStyle(QString style);
	QString style();

#ifdef SINGLE_INSTANCE
	void setUseSingleInstance(bool b);
	bool useSingleInstance();
#endif

	void setSeeking1(int n);
	int seeking1();

	void setSeeking2(int n);
	int seeking2();

	void setSeeking3(int n);
	int seeking3();

	void setSeeking4(int n);
	int seeking4();

	void setUpdateWhileDragging(bool);
	bool updateWhileDragging();

	void setRelativeSeeking(bool);
	bool relativeSeeking();

	void setPreciseSeeking(bool);
	bool preciseSeeking();

	void setDefaultFont(QString font_desc);
	QString defaultFont();

	void setHideVideoOnAudioFiles(bool b);
	bool hideVideoOnAudioFiles();

	// Privacy tab
	void setRecentsMaxItems(int n);
	int recentsMaxItems();

	void setURLMaxItems(int n);
	int urlMaxItems();

	void setRememberDirs(bool b);
	bool rememberDirs();

protected slots:
	void on_changeFontButton_clicked();
#ifdef SINGLE_INSTANCE
	void changeInstanceImages();
#endif

protected:
	virtual void retranslateStrings();

private:
	bool language_changed;
	bool iconset_changed;
	bool style_changed;
	bool recents_changed;
	bool url_max_changed;
};

}} // namespace Gui::Pref

#endif // PREF_INTERFACE_H
