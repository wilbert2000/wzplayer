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

#include "config.h"
#include "smplayer.h"

#include <QDir>
#include <QUrl>

#include "paths.h"
#include "global.h"
#include "version.h"
#include "clhelp.h"
#include "cleanconfig.h"
#include "myapplication.h"

#include "gui/default.h"
#include "gui/mini.h"

#ifdef MPCGUI
#include "gui/mpc/mpc.h"
#endif

#ifdef SKINS
#include "gui/skin/skin.h"
#endif

#ifdef Q_OS_WIN
#if USE_ASSOCIATIONS
#include "extensions.h"
#include "winfileassoc.h"	//required for Uninstall
#endif
#endif

#ifdef FONTCACHE_DIALOG
#include "fontcache.h"
#endif

using namespace Global;

SMPlayer::SMPlayer() :
	QObject(0),
	requested_restart(false),
	main_window(0),
	gui_to_use("DefaultGUI"),
	move_gui(false),
	resize_gui(false),
	close_at_end(-1),
	start_in_fullscreen(-1) {

	showInfo();
}

SMPlayer::~SMPlayer() {
}

void SMPlayer::createGUI() {

#ifdef SKINS
	if (gui_to_use == "SkinGUI") {
		QString theme = pref->iconset;
		if (theme.isEmpty()) theme = "Gonzo";
		QString user_theme_dir = Paths::configPath() + "/themes/" + theme;
		QString theme_dir = Paths::themesPath() + "/" + theme;
		qDebug("SMPlayer::createGUI: user_theme_dir: %s", user_theme_dir.toUtf8().constData());
		qDebug("SMPlayer::createGUI: theme_dir: %s", theme_dir.toUtf8().constData());
		if ((QDir(theme_dir).exists()) || (QDir(user_theme_dir).exists())) {
			if (pref->iconset.isEmpty()) pref->iconset = theme;
		} else {
			qWarning("SMPlayer::createGUI: skin folder doesn't exist. Falling back to default gui.");
			gui_to_use = "DefaultGUI";
			pref->iconset = "";
			pref->gui = gui_to_use;
		}
	}
#endif

	qDebug() << "SMPlayer::createGUI:" << gui_to_use;

#ifdef SKINS
	if (gui_to_use.toLower() == "skingui")
		main_window = new Gui::TSkin(0);
	else
#endif

#ifdef MPCGUI
	if (gui_to_use.toLower() == "mpcgui")
		main_window = new Gui::TMpc(0);
	else
#endif

	if (gui_to_use.toLower() == "minigui")
		main_window = new Gui::TMini(0);
	else
		main_window = new Gui::TDefault(0);

	main_window->loadConfig("");
	qDebug("SMPlayer::createGUI: loadConfig done. Translating...");
	main_window->retranslate();

	main_window->setForceCloseOnFinish(close_at_end);
	main_window->setForceStartInFullscreen(start_in_fullscreen);

	connect(main_window, SIGNAL(requestRestart()), this, SLOT(restart()));

#if SINGLE_INSTANCE
	MyApplication* app = MyApplication::instance();
	connect(app, SIGNAL(messageReceived(const QString&)),
			main_window, SLOT(handleMessageFromOtherInstances(const QString&)));
	app->setActivationWindow(main_window);
#endif

	if (move_gui) {
		qDebug("SMPlayer::createGUI: moving main window to %d %d", gui_position.x(), gui_position.y());
		main_window->move(gui_position);
	}
	if (resize_gui) {
		qDebug("SMPlayer::createGUI: resizing main window to %d x %d", gui_size.width(), gui_size.height());
		main_window->resize(gui_size);
	}

	qDebug() << "SMPlayer::createGUI: created" << gui_to_use;
}

void SMPlayer::restart() {
	qDebug("SMPlayer::restart");

	requested_restart = true;
}

SMPlayer::ExitCode SMPlayer::processArgs(QStringList args) {
	qDebug("SMPlayer::processArgs: arguments: %d", args.count());
	for (int n = 0; n < args.count(); n++) {
		qDebug("SMPlayer::processArgs: %d = %s", n, args[n].toUtf8().data());
	}


	QString action; // Action to be passed to running instance
	bool show_help = false;

	if (!pref->gui.isEmpty()) gui_to_use = pref->gui;
	bool add_to_playlist = false;

#ifdef Q_OS_WIN
	if (args.contains("-uninstall")) {
		#if USE_ASSOCIATIONS
		//Called by uninstaller. Will restore old associations.
		WinFileAssoc RegAssoc; 
		Extensions exts; 
		QStringList regExts; 
		RegAssoc.GetRegisteredExtensions(exts.multimedia(), regExts); 
		RegAssoc.RestoreFileAssociations(regExts); 
		printf("Restored associations\n");
		#endif
		return NoError;
	}
#endif

	if (args.contains("-delete-config")) {
		CleanConfig::clean(Paths::configPath());
		return NoError;
	}

	for (int n = 1; n < args.count(); n++) {
		QString argument = args[n];

		if (argument == "-send-action") {
			if (n+1 < args.count()) {
				n++;
				action = args[n];
			} else {
				printf("Error: expected parameter for -send-action\r\n");
				return ErrorArgument;
			}
		}
		else
		if (argument == "-actions") {
			if (n+1 < args.count()) {
				n++;
				actions_list = args[n];
			} else {
				printf("Error: expected parameter for -actions\r\n");
				return ErrorArgument;
			}
		}
		else
		if (argument == "-sub") {
			if (n+1 < args.count()) {
				n++;
				QString file = args[n];
				if (QFile::exists(file)) {
					subtitle_file = QFileInfo(file).absoluteFilePath();
				} else {
					printf("Error: file '%s' doesn't exists\r\n", file.toUtf8().constData());
				}
			} else {
				printf("Error: expected parameter for -sub\r\n");
				return ErrorArgument;
			}
		}
		else
		if (argument == "-media-title") {
			if (n+1 < args.count()) {
				n++;
				if (media_title.isEmpty()) media_title = args[n];
			}
		}
		else
		if (argument == "-pos") {
			if (n+2 < args.count()) {
				bool ok_x, ok_y;
				n++;
				gui_position.setX( args[n].toInt(&ok_x) );
				n++;
				gui_position.setY( args[n].toInt(&ok_y) );
				if (ok_x && ok_y) move_gui = true;
			} else {
				printf("Error: expected parameter for -pos\r\n");
				return ErrorArgument;
			}
		}
		else
		if (argument == "-size") {
			if (n+2 < args.count()) {
				bool ok_width, ok_height;
				n++;
				gui_size.setWidth( args[n].toInt(&ok_width) );
				n++;
				gui_size.setHeight( args[n].toInt(&ok_height) );
				if (ok_width && ok_height) resize_gui = true;
			} else {
				printf("Error: expected parameter for -resize\r\n");
				return ErrorArgument;
			}
		}
		else
		if ((argument == "--help") || (argument == "-help") ||
            (argument == "-h") || (argument == "-?") ) 
		{
			show_help = true;
		}
		else
		if (argument == "-close-at-end") {
			close_at_end = 1;
		}
		else
		if (argument == "-no-close-at-end") {
			close_at_end = 0;
		}
		else
		if (argument == "-fullscreen") {
			start_in_fullscreen = 1;
		}
		else
		if (argument == "-no-fullscreen") {
			start_in_fullscreen = 0;
		}
		else
		if (argument == "-add-to-playlist") {
			add_to_playlist = true;
		}
		else
		if (argument == "-mini" || argument == "-minigui") {
			gui_to_use = "MiniGUI";
		}
		else
		if (argument == "-mpcgui") {
			gui_to_use = "MpcGUI";
		}
		else
		if (argument == "-defaultgui") {
			gui_to_use = "DefaultGUI";
		}
		else
		if (argument == "-ontop") {
			pref->stay_on_top = Preferences::AlwaysOnTop;
		}
		else
		if (argument == "-no-ontop") {
			pref->stay_on_top = Preferences::NeverOnTop;
		}
#ifdef SKINS
		else
		if (argument == "-skingui") {
			gui_to_use = "SkinGUI";
		}
#endif
		else {
			// File
			#if QT_VERSION >= 0x040600
			QUrl fUrl = QUrl::fromUserInput(argument);
			if (fUrl.isValid() && fUrl.scheme().toLower() == "file") {
			    argument = fUrl.toLocalFile();
			}
			#endif
			if (QFile::exists( argument )) {
				argument = QFileInfo(argument).absoluteFilePath();
			}
			files_to_play.append( argument );
		}
	}

	if (show_help) {
		printf("%s\n", CLHelp::help().toLocal8Bit().data());
		return NoError;
	}

	qDebug("SMPlayer::processArgs: files_to_play: count: %d", files_to_play.count() );
	for (int n=0; n < files_to_play.count(); n++) {
		qDebug("SMPlayer::processArgs: files_to_play[%d]: '%s'", n, files_to_play[n].toUtf8().data());
	}

#ifdef SINGLE_INSTANCE
	if (pref->use_single_instance) {
		// Single instance
		MyApplication * a = MyApplication::instance();
		if (a->isRunning()) {
			a->sendMessage("Hello");

			if (!action.isEmpty()) {
				a->sendMessage("action " + action);
			}
			else {
				if (!subtitle_file.isEmpty()) {
					a->sendMessage("load_sub " + subtitle_file);
				}

				if (!media_title.isEmpty()) {
					a->sendMessage("media_title " + files_to_play[0] + " <<sep>> " + media_title);
				}

				if (!files_to_play.isEmpty()) {
					/* a->sendMessage("open_file " + files_to_play[0]); */
					QString command = "open_files";
					if (add_to_playlist) command = "add_to_playlist";
					a->sendMessage(command +" "+ files_to_play.join(" <<sep>> "));
				}
			}

			return NoError;
		}
	}
#endif

	if (!pref->default_font.isEmpty()) {
		QFont f;
		f.fromString(pref->default_font);
		qApp->setFont(f);
	}

	return SMPlayer::NoExit;
}

void SMPlayer::start() {
	qDebug("SMPlayer::start");

	requested_restart = false;

	// TODO: move to global.cpp?
#ifdef FONTCACHE_DIALOG
#ifndef PORTABLE_APP
	if (Version::with_revision() != pref->smplayer_version) {
		FontCacheDialog d(0);
		d.run(pref->mplayer_bin, "sample.avi");
		pref->smplayer_version = Version::with_revision();
	}
#endif
#endif

	// Create the main window. It will be destoyed when leaving exec()
	createGUI();

	if (!main_window->startHidden() || !files_to_play.isEmpty())
		main_window->show();

	if (!files_to_play.isEmpty()) {
		if (!subtitle_file.isEmpty())
			main_window->setInitialSubtitle(subtitle_file);
		if (!media_title.isEmpty())
			main_window->getCore()->addForcedTitle(files_to_play[0], media_title);
		main_window->openFiles(files_to_play);
	}

	if (!actions_list.isEmpty()) {
		if (files_to_play.isEmpty()) {
			main_window->runActions(actions_list);
		} else {
			main_window->runActionsLater(actions_list);
		}
	}
}

void SMPlayer::showInfo() {
#ifdef Q_OS_WIN
	QString win_ver;
	switch (QSysInfo::WindowsVersion) {
		case QSysInfo::WV_2000: win_ver = "Windows 2000"; break;
		case QSysInfo::WV_XP: win_ver = "Windows XP"; break;
		case QSysInfo::WV_2003: win_ver = "Windows XP Professional x64/Server 2003"; break;
		case QSysInfo::WV_VISTA: win_ver = "Windows Vista/Server 2008"; break;
		#if QT_VERSION >= 0x040501
		case QSysInfo::WV_WINDOWS7: win_ver = "Windows 7/Server 2008 R2"; break;
		#endif
		#if QT_VERSION >= 0x040803
		case QSysInfo::WV_WINDOWS8: win_ver = "Windows 8/Server 2012"; break;
		#endif
		#if ((QT_VERSION >= 0x040806 && QT_VERSION < 0x050000) || (QT_VERSION >= 0x050200))
		case QSysInfo::WV_WINDOWS8_1: win_ver = "Windows 8.1/Server 2012 R2"; break;
		#endif
		#if ((QT_VERSION >= 0x040807 && QT_VERSION < 0x050000) || (QT_VERSION >= 0x050500))
		case QSysInfo::WV_WINDOWS10: win_ver = "Windows 10"; break;
		#endif
		case QSysInfo::WV_NT_based: win_ver = "NT-based Windows"; break;
		default: win_ver = QString("Unknown/Unsupported Windows OS"); break;
	}
#endif
	QString s = QObject::tr("This is SMPlayer v. %1 running on %2")
            .arg(Version::printable())
#ifdef Q_OS_LINUX
           .arg("Linux")
#else
#ifdef Q_OS_WIN
           .arg("Windows ("+win_ver+")")
#else
#ifdef Q_OS_OS2
           .arg("eCS (OS/2)")
#else
		   .arg("Other OS")
#endif
#endif
#endif
           ;

	printf("%s\n", s.toLocal8Bit().data() );
	qDebug("%s", s.toUtf8().data() );
	qDebug("Compiled with Qt v. %s, using %s", QT_VERSION_STR, qVersion());

	qDebug(" * application path: '%s'", Paths::appPath().toUtf8().data());
	qDebug(" * data path: '%s'", Paths::dataPath().toUtf8().data());
	qDebug(" * translation path: '%s'", Paths::translationPath().toUtf8().data());
	qDebug(" * doc path: '%s'", Paths::docPath().toUtf8().data());
	qDebug(" * themes path: '%s'", Paths::themesPath().toUtf8().data());
	qDebug(" * shortcuts path: '%s'", Paths::shortcutsPath().toUtf8().data());
	qDebug(" * config path: '%s'", Paths::configPath().toUtf8().data());
	qDebug(" * ini path: '%s'", Paths::iniPath().toUtf8().data());
	qDebug(" * file for subtitles' styles: '%s'", Paths::subtitleStyleFile().toUtf8().data());
	qDebug(" * current path: '%s'", QDir::currentPath().toUtf8().data());
#ifdef Q_OS_WIN
	qDebug(" * font path: '%s'", Paths::fontPath().toUtf8().data());
#endif
}

#include "moc_smplayer.cpp"
