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

#ifndef _SMPLAYER_H_
#define _SMPLAYER_H_

#include <QtGlobal>
#include <QString>
#include <QStringList>
#include <QSettings>
#include <QTranslator>

#include "gui/base.h"

#ifdef Q_OS_WIN
#if QT_VERSION < 0x050000
#define USE_WINEVENTFILTER
#endif
#endif

#ifdef SINGLE_INSTANCE
#include "QtSingleApplication"
typedef QtSingleApplication TBaseApp;
#else
#include <QApplication>
typedef QApplication TBaseApp;
#endif


class TSMPlayer : public TBaseApp {
	Q_OBJECT

public:
	enum ExitCode { ErrorArgument = -3, NoAction = -2, NoRunningInstance = -1, NoError = 0, NoExit = 1 };

	TSMPlayer(int& argc, char** argv);
	virtual ~TSMPlayer();

	// Nothing to do, let the application close
	virtual void commitData(QSessionManager& /*manager*/) {}

	//! Process arguments. If ExitCode != NoExit the application must be exited.
	ExitCode processArgs();
	int execWithRestart();

#ifdef USE_WINEVENTFILTER
	virtual bool winEventFilter(MSG* msg, long* result);
#endif

private slots:
	void loadTranslation();
	void onRequestRestart(bool reset_style);

private:
	QString initial_config_path;
	QTranslator app_trans;
	QTranslator qt_trans;
	Gui::TBase* main_window;

	bool requested_restart;
	bool reset_style;
	int current_file;

	QStringList files_to_play;
	QString subtitle_file;
	QString actions_list; //!< Actions to be run on startup
	QString media_title; //!< Force a title for the first file

	// Change position and size
	bool move_gui;
	QPoint gui_position;
	bool resize_gui;
	QSize gui_size;

	// Options to pass to gui
	int close_at_end; // -1 = not set, 1 = true, 0 false
	int start_in_fullscreen; // -1 = not set, 1 = true, 0 false

	QString default_style;

	bool loadCatalog(QTranslator& translator,
					 const QString& name,
					 const QString& locale,
					 const QString& dir);
	void loadConfig();
	QString loadStyleSheet(const QString& filename);
	void changeStyleSheet(const QString& style);
	void changeStyle();
	void createGUI();
	bool processArgName(const QString& arg, const QStringList& args) const;
	int processArgPos(const QString& name, const QStringList& args) const;
	void start();
	void showInfo();
};

#endif
