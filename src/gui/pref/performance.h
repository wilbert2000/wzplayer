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

#ifndef PREF_PERFORMANCE_H
#define PREF_PERFORMANCE_H

#include "ui_performance.h"
#include "gui/pref/widget.h"


namespace Settings {
class TPreferences;
}

namespace Gui {
namespace Pref {

class TPerformance : public TWidget, public Ui::TPerformance {
	Q_OBJECT

public:
	TPerformance(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~TPerformance();

	virtual QString sectionName();
	virtual QPixmap sectionIcon();

    // Pass data to the dialog
	void setData(Settings::TPreferences* pref);

    // Apply changes
	void getData(Settings::TPreferences* pref);

protected:
	virtual void createHelp();

	void setCacheForFiles(int n);
	int cacheForFiles();

	void setCacheForStreams(int n);
	int cacheForStreams();

	void setCacheForDVDs(int n);
	int cacheForDVDs();

	void setCacheForAudioCDs(int n);
	int cacheForAudioCDs();

	void setCacheForVCDs(int n);
	int cacheForVCDs();

	void setCacheForTV(int n);
	int cacheForTV();

	void setPriority(int n);
	int priority();

	void setFrameDrop(bool b);
	bool frameDrop();

	void setHardFrameDrop(bool b);
	bool hardFrameDrop();

	void setHwdec(const QString & v);
	QString hwdec();

protected:
	virtual void retranslateStrings();
};

} // namespace Pref
} // namespace Gui

#endif // PREF_PERFORMANCE_H
