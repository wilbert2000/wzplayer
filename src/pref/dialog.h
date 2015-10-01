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

#ifndef _PREF_DIALOG_H_
#define _PREF_DIALOG_H_

#include "ui_dialog.h"
#include "pref/widget.h"
#include "pref/general.h"
#include "pref/drives.h"
#include "pref/interface.h"
#include "pref/performance.h"
#include "pref/input.h"
#include "pref/subtitles.h"
#include "pref/advanced.h"
#include "pref/prefplaylist.h"
#include "pref/tv.h"
#include "pref/updates.h"
#include "pref/network.h"
#if USE_ASSOCIATIONS
#include "pref/associations.h"
#endif

class QTextBrowser;
class QPushButton;

class Preferences;

namespace Pref {


class TDialog : public QDialog, public Ui::TDialog
{
	Q_OBJECT

public:
	enum Section { General=0, Drives=1, Performance=2,
                   Subtitles=3, Gui=4, Mouse=5, Advanced=6, Associations=7 };

	TDialog( QWidget * parent = 0, Qt::WindowFlags f = 0 );
	~TDialog();

	TGeneral * mod_general() { return page_general; }
	TInterface * mod_interface() { return page_interface; }
	TInput * mod_input() { return page_input; }
	TAdvanced * mod_advanced() { return page_advanced; }
	TPrefPlaylist * mod_playlist() { return page_playlist; }
	TUpdates * mod_updates() { return page_updates; }
	TNetwork * mod_network() { return page_network; }

	void addSection(TWidget *w);

	// Pass data to the standard dialogs
	void setData(Preferences * pref);

	// Apply changes
	void getData(Preferences * pref);

	// Return true if the mplayer process should be restarted.
	bool requiresRestart();

public slots:
	void showSection(Section s);

	virtual void accept(); // Reimplemented to send a signal
	virtual void reject();

signals:
	void applied();

protected:
	virtual void retranslateStrings();
	virtual void changeEvent ( QEvent * event );

protected slots:
	void apply();
	void showHelp();

protected:
	TGeneral * page_general;
	TDrives * page_drives;
	TPerformance * page_performance;
	TSubtitles * page_subtitles;
	TInterface * page_interface;
	TInput * page_input;
	TPrefPlaylist * page_playlist;
	TTV * page_tv;
	TUpdates * page_updates;
	TNetwork * page_network;
	TAdvanced * page_advanced;

#if USE_ASSOCIATIONS
	TAssociations* page_associations;
#endif

	QTextBrowser * help_window;

private:
	QPushButton * okButton;
	QPushButton * cancelButton;
	QPushButton * applyButton;
	QPushButton * helpButton;
};

} // namespace Pref

#endif // _PREF_DIALOG_H_
