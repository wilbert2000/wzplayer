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

#ifndef GUI_PREF_DIALOG_H
#define GUI_PREF_DIALOG_H

#include "ui_dialog.h"


class QTextBrowser;
class QPushButton;

namespace Settings {
class TPreferences;
}

namespace Gui {
namespace Pref {

class TWidget;
class TGeneral;
class TDemuxer;
class TVideo;
class TAudio;
class TInterface;
class TInput;
class TPrefPlaylist;
class TSubtitles;
class TDrives;
class TPerformance;
class TNetwork;
class TUpdates;

#if USE_ASSOCIATIONS
class TAssociations;
#endif

class TAdvanced;


class TDialog : public QDialog, public Ui::TDialog {
	Q_OBJECT

public:
	enum TSection {
		SECTION_GENERAL = 0,
		SECTION_DEMUXER,
		SECTION_VIDEO,
		SECTION_AUDIO,
		SECTION_SUBTITLES,
		SECTION_GUI,
		SECTION_PLAYLIST,
		SECTION_INPUT,
		SECTION_DRIVES,
		SECTION_PERFORMANCE,
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

private:
	TGeneral* page_general;
	TDemuxer* page_demuxer;
	TVideo* page_video;
	TAudio* page_audio;
	TSubtitles* page_subtitles;
	TInterface* page_interface;
	TPrefPlaylist* page_playlist;
	TInput* page_input;
	TDrives* page_drives;
	TPerformance* page_performance;
	TNetwork* page_network;
	TUpdates* page_updates;

#if USE_ASSOCIATIONS
	TAssociations* page_associations;
#endif

	TAdvanced* page_advanced;

	QTextBrowser* help_window;

	QPushButton* okButton;
	QPushButton* cancelButton;
	QPushButton* applyButton;
	QPushButton* helpButton;

	void addSection(TWidget* w);

private slots:
	void apply();
	void showHelp();
	void binChanged(const QString& path);
};

} // namespace Pref
} // namespace Gui

#endif // GUI_PREF_DIALOG_H
