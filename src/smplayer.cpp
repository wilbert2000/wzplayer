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

#include <QDebug>
#include <QFile>
#include <QDir>
#include <QUrl>
#include <QTranslator>
#include <QLocale>

#include "paths.h"
#include "settings/preferences.h"
#include "log.h"
#include "version.h"
#include "clhelp.h"
#include "cleanconfig.h"

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


TSMPlayer::TSMPlayer(int& argc, char** argv) :
	TBaseApp(
#ifdef SINGLE_INSTANCE
		"smplayer", // AppID
#endif
		argc, argv),
	log(true, false, ".*"),
	main_window(0),
	requested_restart(false),
	gui_to_use("DefaultGUI"),
	move_gui(false),
	resize_gui(false),
	close_at_end(-1),
	start_in_fullscreen(-1) {

	// Change working directory to application path
	QDir::setCurrent(applicationDirPath());
	Paths::setAppPath(applicationDirPath());

#if QT_VERSION >= 0x040400
	// Enable icons in menus
	setAttribute(Qt::AA_DontShowIconsInMenus, false);
#endif
}

TSMPlayer::~TSMPlayer() {
	delete Settings::pref;
}

bool TSMPlayer::loadCatalog(QTranslator& translator,
							const QString& name,
							const QString& locale,
							const QString& dir) {

	QString loc = name + "_" + locale; //.toLower();
	bool r = translator.load(loc, dir);
	if (r)
		qDebug("TSMPlayer::loadCatalog: successfully loaded %s from %s",
			   loc.toUtf8().data(), dir.toUtf8().data());
	else
		qDebug("TSMPlayer::loadCatalog: can't load %s from %s",
			   loc.toUtf8().data(), dir.toUtf8().data());
	return r;
}

void TSMPlayer::loadTranslation() {

	QString locale = pref->language;
	if (locale.isEmpty()) {
		locale = QLocale::system().name();
	}
	QString trans_path = Paths::translationPath();

	// Try to load it first from app path (in case there's an updated
	// translation), if it fails it will try then from the Qt path.
	if (!loadCatalog(qt_trans, "qt", locale, trans_path)) {
		loadCatalog(qt_trans, "qt", locale, Paths::qtTranslationPath());
	}
	loadCatalog(app_trans, "smplayer", locale, trans_path);
}

void TSMPlayer::loadConfig(const QString& config_path) {

	// Load preferences
	Paths::setConfigPath(config_path);
	Settings::pref = new Settings::TPreferences();

	// Reconfig log
	log.setEnabled(pref->log_enabled);
	log.setLogFileEnabled(pref->log_file);
	log.setFilter(pref->log_filter);

	// Load translation
	loadTranslation();
	installTranslator(&app_trans);
	installTranslator(&qt_trans);

	// Fonts
#ifdef Q_OS_WIN
	Paths::createFontFile();
#endif
}

TSMPlayer::ExitCode TSMPlayer::processArgs() {

	QStringList args = arguments();

#ifdef Q_OS_WIN
	if (args.contains("-uninstall")) {
		#if USE_ASSOCIATIONS
		//Called by uninstaller. Will restore old associations.
		WinFileAssoc RegAssoc; 
		TExtensions exts;
		QStringList regExts; 
		RegAssoc.GetRegisteredExtensions(exts.multimedia(), regExts); 
		RegAssoc.RestoreFileAssociations(regExts); 
		printf("TSMPlayer::processArgs: restored associations\n");
		#endif
		return NoError;
	}
#endif

	// Get config path from args
	QString config_path;
	int pos = args.indexOf("-config-path");
	if ( pos >= 0) {
		if (pos + 1 < args.count()) {
			pos++;
			config_path = args[pos];
			// Delete from list
			args.removeAt(pos);
			args.removeAt(pos - 1);
		} else {
			printf("TSMPlayer::processArgs: error: expected parameter for -config-path\r\n");
			return TSMPlayer::ErrorArgument;
		}
	}

	// Load preferences, setup logging and translation
	loadConfig(config_path);

	if (args.contains("-delete-config")) {
		CleanConfig::clean(Paths::configPath());
		return NoError;
	}

	showInfo();

	QString action; // Action to be passed to running instance
	bool show_help = false;

	if (!Settings::pref->gui.isEmpty()) gui_to_use = Settings::pref->gui;
	bool add_to_playlist = false;

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
			Settings::pref->stay_on_top = Settings::TPreferences::AlwaysOnTop;
		}
		else
		if (argument == "-no-ontop") {
			Settings::pref->stay_on_top = Settings::TPreferences::NeverOnTop;
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

	qDebug("TSMPlayer::processArgs: files_to_play: count: %d", files_to_play.count() );
	for (int n=0; n < files_to_play.count(); n++) {
		qDebug("TSMPlayer::processArgs: files_to_play[%d]: '%s'", n, files_to_play[n].toUtf8().data());
	}

#ifdef SINGLE_INSTANCE
	if (Settings::pref->use_single_instance) {
		// Single instance
		if (isRunning()) {
			sendMessage("Hello");

			if (!action.isEmpty()) {
				sendMessage("action " + action);
			} else {
				if (!subtitle_file.isEmpty()) {
					sendMessage("load_sub " + subtitle_file);
				}

				if (!media_title.isEmpty()) {
					sendMessage("media_title " + files_to_play[0] + " <<sep>> "
						+ media_title);
				}

				if (!files_to_play.isEmpty()) {
					QString command = "open_files";
					if (add_to_playlist) command = "add_to_playlist";
					sendMessage(command + " " + files_to_play.join(" <<sep>> "));
				}
			}

			return NoError;
		}
	}
#endif

	if (!Settings::pref->default_font.isEmpty()) {
		QFont f;
		f.fromString(Settings::pref->default_font);
		setFont(f);
	}

	return TSMPlayer::NoExit;
}

void TSMPlayer::createGUI() {

#ifdef SKINS
	if (gui_to_use == "SkinGUI") {
		QString theme = Settings::pref->iconset;
		if (theme.isEmpty()) theme = "Gonzo";
		QString user_theme_dir = Paths::configPath() + "/themes/" + theme;
		QString theme_dir = Paths::themesPath() + "/" + theme;
		qDebug("TSMPlayer::createGUI: user_theme_dir: %s", user_theme_dir.toUtf8().constData());
		qDebug("TSMPlayer::createGUI: theme_dir: %s", theme_dir.toUtf8().constData());
		if ((QDir(theme_dir).exists()) || (QDir(user_theme_dir).exists())) {
			if (Settings::pref->iconset.isEmpty()) Settings::pref->iconset = theme;
		} else {
			qWarning("TSMPlayer::createGUI: skin folder doesn't exist. Falling back to default gui.");
			gui_to_use = "DefaultGUI";
			Settings::pref->iconset = "";
			Settings::pref->gui = gui_to_use;
		}
	}
#endif

	qDebug() << "TSMPlayer::createGUI: gui to create" << gui_to_use;

#ifdef SKINS
	if (gui_to_use.toLower() == "skingui")
		main_window = new Gui::TSkin();
	else
#endif

#ifdef MPCGUI
	if (gui_to_use.toLower() == "mpcgui")
		main_window = new Gui::TMpc();
	else
#endif

	if (gui_to_use.toLower() == "minigui")
		main_window = new Gui::TMini();
	else
		main_window = new Gui::TDefault();

	main_window->loadConfig("");
	qDebug("TSMPlayer::createGUI: loadConfig done. Translating...");
	main_window->retranslate();

	main_window->setForceCloseOnFinish(close_at_end);
	main_window->setForceStartInFullscreen(start_in_fullscreen);

	connect(main_window, SIGNAL(loadTranslation()),
			this, SLOT(loadTranslation()));
	connect(main_window, SIGNAL(requestRestart()),
			this, SLOT(setRequestedRestart()));

#if SINGLE_INSTANCE
	connect(this, SIGNAL(messageReceived(const QString&)),
			main_window, SLOT(handleMessageFromOtherInstances(const QString&)));
	setActivationWindow(main_window);
#endif

	if (move_gui) {
		qDebug("TSMPlayer::createGUI: moving main window to %d %d", gui_position.x(), gui_position.y());
		main_window->move(gui_position);
	}
	if (resize_gui) {
		qDebug("TSMPlayer::createGUI: resizing main window to %d x %d", gui_size.width(), gui_size.height());
		main_window->resize(gui_size);
	}

	qDebug() << "TSMPlayer::createGUI: created" << gui_to_use;
}

void TSMPlayer::setRequestedRestart() {
	qDebug("TSMPlayer::setRequestedRestart");

	requested_restart = true;
}

int TSMPlayer::execWithRestart() {

	int exit_code;
	do {
		requested_restart = false;
		start();
		qDebug("TSMPlayer::execWithRestart: calling exec()");
		exit_code = exec();
		qDebug("TSMPlayer::execWithRestart: exec() returned %d", exit_code);
	} while (requested_restart);

	return exit_code;
}

void TSMPlayer::start() {
	qDebug("TSMPlayer::start");

	// Create the main window. It will be destoyed when leaving exec().
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

void TSMPlayer::showInfo() {
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
				.arg("Linux");
#else
#ifdef Q_OS_WIN
				.arg("Windows ("+win_ver+")");
#else
#ifdef Q_OS_OS2
				.arg("eCS (OS/2)");
#else
				.arg("Other OS");
#endif
#endif
#endif

	qDebug("%s", s.toUtf8().data() );
	qDebug("Compiled with Qt v. %s, using %s", QT_VERSION_STR, qVersion());

	qDebug(" * application path: '%s'", Paths::appPath().toUtf8().data());
	qDebug(" * data path: '%s'", Paths::dataPath().toUtf8().data());
	qDebug(" * translation path: '%s'", Paths::translationPath().toUtf8().data());
	qDebug(" * doc path: '%s'", Paths::docPath().toUtf8().data());
	qDebug(" * themes path: '%s'", Paths::themesPath().toUtf8().data());
	qDebug(" * shortcuts path: '%s'", Paths::shortcutsPath().toUtf8().data());
	qDebug(" * config path: '%s'", Paths::configPath().toUtf8().data());
	qDebug(" * file for subtitles' styles: '%s'", Paths::subtitleStyleFile().toUtf8().data());
	qDebug(" * current path: '%s'", QDir::currentPath().toUtf8().data());
#ifdef Q_OS_WIN
	qDebug(" * font path: '%s'", Paths::fontPath().toUtf8().data());
#endif
}

#ifdef USE_WINEVENTFILTER
#include <QKeyEvent>
#include <QEvent>
#include <QWidget>
#include <windows.h>

#ifndef WM_APPCOMMAND
#define WM_APPCOMMAND 0x0319
#endif

#ifndef FAPPCOMMAND_MOUSE
#define FAPPCOMMAND_MOUSE 0x8000
#define FAPPCOMMAND_KEY   0
#define FAPPCOMMAND_OEM   0x1000
#define FAPPCOMMAND_MASK  0xF000
#define GET_APPCOMMAND_LPARAM(lParam) ((short)(HIWORD(lParam) & ~FAPPCOMMAND_MASK))
#define GET_DEVICE_LPARAM(lParam)     ((WORD)(HIWORD(lParam) & FAPPCOMMAND_MASK))
#define GET_MOUSEORKEY_LPARAM         GET_DEVICE_LPARAM
#define GET_FLAGS_LPARAM(lParam)      (LOWORD(lParam))
#define GET_KEYSTATE_LPARAM(lParam)   GET_FLAGS_LPARAM(lParam)

#define APPCOMMAND_BROWSER_BACKWARD       1
#define APPCOMMAND_BROWSER_FORWARD        2
#define APPCOMMAND_BROWSER_REFRESH        3
#define APPCOMMAND_BROWSER_STOP           4
#define APPCOMMAND_BROWSER_SEARCH         5
#define APPCOMMAND_BROWSER_FAVORITES      6
#define APPCOMMAND_BROWSER_HOME           7
#define APPCOMMAND_VOLUME_MUTE            8
#define APPCOMMAND_VOLUME_DOWN            9
#define APPCOMMAND_VOLUME_UP              10
#define APPCOMMAND_MEDIA_NEXTTRACK        11
#define APPCOMMAND_MEDIA_PREVIOUSTRACK    12
#define APPCOMMAND_MEDIA_STOP             13
#define APPCOMMAND_MEDIA_PLAY_PAUSE       14
#define APPCOMMAND_LAUNCH_MAIL            15
#define APPCOMMAND_LAUNCH_MEDIA_SELECT    16
#define APPCOMMAND_LAUNCH_APP1            17
#define APPCOMMAND_LAUNCH_APP2            18
#define APPCOMMAND_BASS_DOWN              19
#define APPCOMMAND_BASS_BOOST             20
#define APPCOMMAND_BASS_UP                21
#define APPCOMMAND_TREBLE_DOWN            22
#define APPCOMMAND_TREBLE_UP              23
#endif // FAPPCOMMAND_MOUSE

// New commands from Windows XP (some even Sp1)
#ifndef APPCOMMAND_MICROPHONE_VOLUME_MUTE
#define APPCOMMAND_MICROPHONE_VOLUME_MUTE 24
#define APPCOMMAND_MICROPHONE_VOLUME_DOWN 25
#define APPCOMMAND_MICROPHONE_VOLUME_UP   26
#define APPCOMMAND_HELP                   27
#define APPCOMMAND_FIND                   28
#define APPCOMMAND_NEW                    29
#define APPCOMMAND_OPEN                   30
#define APPCOMMAND_CLOSE                  31
#define APPCOMMAND_SAVE                   32
#define APPCOMMAND_PRINT                  33
#define APPCOMMAND_UNDO                   34
#define APPCOMMAND_REDO                   35
#define APPCOMMAND_COPY                   36
#define APPCOMMAND_CUT                    37
#define APPCOMMAND_PASTE                  38
#define APPCOMMAND_REPLY_TO_MAIL          39
#define APPCOMMAND_FORWARD_MAIL           40
#define APPCOMMAND_SEND_MAIL              41
#define APPCOMMAND_SPELL_CHECK            42
#define APPCOMMAND_DICTATE_OR_COMMAND_CONTROL_TOGGLE    43
#define APPCOMMAND_MIC_ON_OFF_TOGGLE      44
#define APPCOMMAND_CORRECTION_LIST        45
#define APPCOMMAND_MEDIA_PLAY             46
#define APPCOMMAND_MEDIA_PAUSE            47
#define APPCOMMAND_MEDIA_RECORD           48
#define APPCOMMAND_MEDIA_FAST_FORWARD     49
#define APPCOMMAND_MEDIA_REWIND           50
#define APPCOMMAND_MEDIA_CHANNEL_UP       51
#define APPCOMMAND_MEDIA_CHANNEL_DOWN     52
#endif // APPCOMMAND_MICROPHONE_VOLUME_MUTE

#define VK_MEDIA_NEXT_TRACK 0xB0
#define VK_MEDIA_PREV_TRACK 0xB1
#define VK_MEDIA_PLAY_PAUSE 0xB3
#define VK_MEDIA_STOP 0xB2

bool TSMPlayer::winEventFilter(MSG * msg, long * result) {
	//qDebug() << "TSMPlayer::winEventFilter" << msg->message << "lParam:" << msg->lParam;

	static uint last_appcommand = 0;

	if (msg->message == WM_KEYDOWN) {
		//qDebug("TSMPlayer::winEventFilter: WM_KEYDOWN: %X", msg->wParam);
		bool eat_key = false;
		if ((last_appcommand == APPCOMMAND_MEDIA_NEXTTRACK) && (msg->wParam == VK_MEDIA_NEXT_TRACK)) eat_key = true;
		else
		if ((last_appcommand == APPCOMMAND_MEDIA_PREVIOUSTRACK) && (msg->wParam == VK_MEDIA_PREV_TRACK)) eat_key = true;
		else
		if ((last_appcommand == APPCOMMAND_MEDIA_PLAY_PAUSE) && (msg->wParam == VK_MEDIA_PLAY_PAUSE)) eat_key = true;
		else
		if ((last_appcommand == APPCOMMAND_MEDIA_STOP) && (msg->wParam == VK_MEDIA_STOP)) eat_key = true;

		if (eat_key) {
			qDebug("TSMPlayer::winEventFilter: ignoring key %X", msg->wParam);
			last_appcommand = 0;
			*result = true;
			return true;
		}
	}
	else
	if (msg->message == WM_APPCOMMAND) {
		/*
		QKeySequence k(Qt::Key_MediaTogglePlayPause);
		qDebug() << "TSMPlayer::winEventFilter" << k.toString();
		*/

		//qDebug() << "TSMPlayer::winEventFilter" << msg->message << "lParam:" << msg->lParam;
		uint cmd  = GET_APPCOMMAND_LPARAM(msg->lParam);
		uint uDevice = GET_DEVICE_LPARAM(msg->lParam);
		uint dwKeys = GET_KEYSTATE_LPARAM(msg->lParam);
		qDebug() << "TSMPlayer::winEventFilter: cmd:" << cmd <<"uDevice:" << uDevice << "dwKeys:" << dwKeys;

		//if (uDevice == FAPPCOMMAND_KEY) {
			int key = 0;
			Qt::KeyboardModifiers modifier = Qt::NoModifier;
			QString name;

			switch (cmd) {
				case APPCOMMAND_MEDIA_PAUSE: key = Qt::Key_MediaPause; name = "Media Pause"; break;
				case APPCOMMAND_MEDIA_PLAY: key = Qt::Key_MediaPlay; name = "Media Play"; break;
				case APPCOMMAND_MEDIA_STOP: key = Qt::Key_MediaStop; name = "Media Stop"; break;
				case APPCOMMAND_MEDIA_PLAY_PAUSE: key = Qt::Key_MediaTogglePlayPause; name = "Toggle Media Play/Pause"; break;

				case APPCOMMAND_MEDIA_NEXTTRACK: key = Qt::Key_MediaNext; name = "Media Next"; break;
				case APPCOMMAND_MEDIA_PREVIOUSTRACK: key = Qt::Key_MediaPrevious; name = "Media Previous"; break;

				case APPCOMMAND_MEDIA_FAST_FORWARD: key = Qt::Key_F; modifier = Qt::ShiftModifier | Qt::ControlModifier; break;
				case APPCOMMAND_MEDIA_REWIND: key = Qt::Key_B; modifier = Qt::ShiftModifier | Qt::ControlModifier; break;
			}

			if (key != 0) {
				last_appcommand = cmd;

				QKeyEvent event(QEvent::KeyPress, key, modifier, name);
				QWidget * w = QApplication::focusWidget();
				if (w) QCoreApplication::sendEvent(w, &event);
				*result = true;
				return true;
			}
		//}
	}

	return false;
}
#endif // USE_WINEVENTFILTER

#include "moc_smplayer.cpp"
