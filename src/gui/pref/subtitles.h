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

#ifndef GUI_PREF_SUBTITLES_H
#define GUI_PREF_SUBTITLES_H

#include "ui_subtitles.h"
#include "gui/pref/widget.h"
#include "log4qt/logger.h"

class Encodings;

namespace Settings {
class TPreferences;
}

namespace Gui {
namespace Pref {

class TSubtitles : public TWidget, public Ui::TSubtitles {
	Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

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
	void setForceAssStyles(bool b);
	bool forceAssStyles();

	void setCustomizedAssStyle(const QString& style) { forced_ass_style = style; }
	QString customizedAssStyle() const { return forced_ass_style; }

protected:
	virtual void retranslateStrings();

private:
	Encodings* encodings;
	QString forced_ass_style;
	bool enable_border_spins;

	void createHelp();

	void setFuzziness(int n);
	int fuzziness();

	void setSubtitleLanguage(const QString& lang);
	QString subtitleLanguage();

	void setSelectFirstSubtitle(bool v);
	bool selectFirstSubtitle();

	void setEncaLang(const QString& s);
	QString encaLang();

	void setEncodingFallback(const QString& s);
	QString encodingFallback();

	void setAssFontScale(double n);
	double assFontScale();

	void setAssLineSpacing(int spacing);
	int assLineSpacing();

private slots:
	void onWindowsFontDirCheckToggled(bool b);
	void onUseCustomStyleToggled(bool b);
	void onBorderStyleCurrentIndexChanged(int index);
	void onAssCustomizeButtonClicked();
};

} // namespace Pref
} // namespace Gui

#endif // GUI_PREF_SUBTITLES_H
