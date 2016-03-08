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

#ifndef PREF_DIALOG_H
#define PREF_DIALOG_H

#include "ui_dialog.h"
#include "gui/pref/widget.h"
#include "gui/pref/general.h"
#include "gui/pref/drives.h"
#include "gui/pref/interface.h"
#include "gui/pref/performance.h"
#include "gui/pref/input.h"
#include "gui/pref/subtitles.h"
#include "gui/pref/advanced.h"
#include "gui/pref/prefplaylist.h"
#include "gui/pref/tv.h"
#include "gui/pref/updates.h"
#include "gui/pref/network.h"
#if USE_ASSOCIATIONS
#include "gui/pref/associations.h"
#endif


class QTextBrowser;
class QPushButton;

namespace Gui {
namespace Pref {


class TDialog : public QDialog, public Ui::TDialog {
	Q_OBJECT

public:
	enum TSection {
		SECTION_GENERAL = 0,
		SECTION_GUI,
		SECTION_INPUT,
		SECTION_PLAYLIST,
		SECTION_SUBTITLES,
		SECTION_DRIVES,
		SECTION_PERFORMANCE,
		SECTION_TV,
		SECTION_NETWORK,
		SECTION_UPDATES,
#if USE_ASSOCIATIONS
		SECTION_ASSOCIATIONS,
#endif
		SECTION_ADVANCED
	};

	TDialog(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~TDialog();

	TInterface* mod_interface() const { return page_interface; }
	TInput* mod_input() const { return page_input; }
	TPrefPlaylist* mod_playlist() const { return page_playlist; }
	TAdvanced* mod_advanced() const { return page_advanced; }

	// Pass data to the standard dialogs
	void setData(Settings::TPreferences* pref);

	// Apply changes
	void getData(Settings::TPreferences* pref);

	// Return true if the mplayer process should be restarted.
	bool requiresRestart();

	void showSection(TSection section);

public slots:
	virtual void accept(); // Reimplemented to send a signal
	virtual void reject();

signals:
	void applied();

protected:
	virtual void retranslateStrings();
	virtual void changeEvent (QEvent* event);

protected slots:
	void apply();
	void showHelp();

protected:
	TGeneral* page_general;
	TInterface* page_interface;
	TInput* page_input;
	TPrefPlaylist* page_playlist;
	TSubtitles* page_subtitles;
	TDrives* page_drives;
	TPerformance* page_performance;
	TTV* page_tv;
	TNetwork* page_network;
	TUpdates* page_updates;

#if USE_ASSOCIATIONS
	TAssociations* page_associations;
#endif

	TAdvanced* page_advanced;

	QTextBrowser* help_window;

private:
	QPushButton* okButton;
	QPushButton* cancelButton;
	QPushButton* applyButton;
	QPushButton* helpButton;

	void addSection(TWidget* w);
};

} // namespace Pref
} // namespace Gui

#endif // _PREF_DIALOG_H_
