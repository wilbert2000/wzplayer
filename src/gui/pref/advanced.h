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

#ifndef PREF_ADVANCED_H
#define PREF_ADVANCED_H

#include "ui_advanced.h"
#include "gui/pref/widget.h"
#include "settings/preferences.h"

namespace Gui {
namespace Pref {

class TAdvanced : public TWidget, public Ui::TAdvanced {
	Q_OBJECT

public:
	TAdvanced(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~TAdvanced();

	virtual QString sectionName();
	virtual QPixmap sectionIcon();

    // Pass data to the dialog
	void setData(Settings::TPreferences* pref);

    // Apply changes
	void getData(Settings::TPreferences* pref);

	bool colorkeyChanged() { return colorkey_changed; }
	bool lavfDemuxerChanged() { return lavf_demuxer_changed; }

protected:
	virtual void createHelp();

	// Advanced
	void setMplayerAdditionalArguments(QString args);
	QString mplayerAdditionalArguments();

	void setMplayerAdditionalVideoFilters(QString s);
	QString mplayerAdditionalVideoFilters();

	void setMplayerAdditionalAudioFilters(QString s);
	QString mplayerAdditionalAudioFilters();

	void setColorKey(unsigned int c);
	unsigned int colorKey();

	void setUseIdx(bool);
	bool useIdx();

	void setUseLavfDemuxer(bool);
	bool useLavfDemuxer();

	void setUseCorrectPts(Settings::TPreferences::TOptionState value);
	Settings::TPreferences::TOptionState useCorrectPts();

	void setActionsToRun(QString actions);
	QString actionsToRun();

protected:
	virtual void retranslateStrings();

protected slots:
	void on_changeButton_clicked();

private:
	bool colorkey_changed;
	bool lavf_demuxer_changed;
};

} // namespace Pref
} // namespace Gui

#endif // PREF_ADVANCED_H
