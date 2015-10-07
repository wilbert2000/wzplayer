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

#ifndef _PREF_SUBTITLES_H_
#define _PREF_SUBTITLES_H_

#include "ui_subtitles.h"
#include "settings/preferences.h"
#include "gui/pref/widget.h"

class Encodings;

namespace Gui { namespace Pref {

class TSubtitles : public TWidget, public Ui::TSubtitles
{
	Q_OBJECT

public:
	TSubtitles(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~TSubtitles();

	virtual QString sectionName();
	virtual QPixmap sectionIcon();

    // Pass data to the dialog
	void setData(Settings::TPreferences* pref);

    // Apply changes
	void getData(Settings::TPreferences* pref);

protected:
	virtual void createHelp();

	void setAssFontScale(double n);
	double assFontScale();

	void setAutoloadSub(bool v);
	bool autoloadSub();

	void setFontEncoding(QString s);
	QString fontEncoding();

	void setUseEnca(bool v);
	bool useEnca();

	void setEncaLang(QString s);
	QString encaLang();

	void setAssLineSpacing(int spacing);
	int assLineSpacing();

	void setForceAssStyles(bool b);
	bool forceAssStyles();

	void setCustomizedAssStyle(QString style) { forced_ass_style = style; }
	QString customizedAssStyle() { return forced_ass_style; }

	void setFontFuzziness(int n);
	int fontFuzziness();

	void setSubtitlesOnScreenshots(bool b);
	bool subtitlesOnScreenshots();

	void setFreetypeSupport(bool b);
	bool freetypeSupport();

protected slots:
	/* void on_ass_subs_button_toggled(bool b); */
	void on_ass_customize_button_clicked();
	void on_freetype_check_toggled(bool b);
	void on_windowsfontdir_check_toggled(bool b);
	void checkBorderStyleCombo(int index);

protected:
	virtual void retranslateStrings();

private:
	Encodings* encodings;
	QString forced_ass_style;
};

}} // namespace Gui::Pref

#endif // _PREF_SUBTITLES_H_
