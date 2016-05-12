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

#include "app.h"

#include <QFile>
#include <QDir>
#include <QUrl>
#include <QTranslator>
#include <QLocale>
#include <QStyle>

#include "log4qt/logger.h"

#include "config.h"
#include "settings/paths.h"
#include "settings/preferences.h"
#include "settings/cleanconfig.h"
#include "version.h"
#include "clhelp.h"
#include "images.h"
#include "iconprovider.h"
#include "core.h"

#include "gui/playlist/playlist.h"
#include "gui/default.h"

#ifdef Q_OS_WIN
#if USE_ASSOCIATIONS
#include "extensions.h"
#include "winfileassoc.h"	//required for Uninstall
#endif
#endif


using namespace Settings;

TApp::TApp(int& argc, char** argv) :
    QtSingleApplication(TConfig::PROGRAM_ID, argc, argv),
    main_window(0),
    requested_restart(false),
    reset_style(false),
    move_gui(false),
    resize_gui(false),
    close_at_end(-1),
    start_in_fullscreen(-1) {

    setOrganizationName(TConfig::PROGRAM_ORG);
    setApplicationName(TConfig::PROGRAM_ID);
    //setApplicationVersion(TConfig::PROGRAM_VERSION);
    //setOrganizationDomain("www.xs4all.nl");

    // Save default style for resetting style
    default_style = style()->objectName();

    // Enable icons in menus
    setAttribute(Qt::AA_DontShowIconsInMenus, false);

    logger()->debug("TApp: created application");
}

TApp::~TApp() {
    delete Settings::pref;
}

bool TApp::loadCatalog(QTranslator& translator,
                       const QString& name,
                       const QString& locale,
                       const QString& dir) {

    QString loc = name + "_" + locale;
	bool r = translator.load(loc, dir);
	if (r)
        logger()->info("loadCatalog: loaded " + loc + " from " + dir);
	else
        logger()->debug("loadCatalog: failed to load " + loc + " from " + dir);
	return r;
}

void TApp::loadTranslation() {

	QString locale = pref->language;
	if (locale.isEmpty()) {
		locale = QLocale::system().name();
	}
	QString trans_path = TPaths::translationPath();

	// Try to load it first from app path (in case there's an updated
	// translation), if it fails it will try then from the Qt path.
	if (!loadCatalog(qt_trans, "qt", locale, trans_path)) {
		loadCatalog(qt_trans, "qt", locale, TPaths::qtTranslationPath());
	}
	loadCatalog(app_trans, TConfig::PROGRAM_ID, locale, trans_path);
}

void TApp::loadConfig() {

	// Setup config directory
	TPaths::setConfigPath(initial_config_path);
	// Load new settings
	Settings::pref = new Settings::TPreferences();

	// Reconfig log
	// --debug from the command line overrides preferences
    //if (!TLog::log->logDebugMessages()) {
    //	TLog::log->setLogDebugMessages(pref->log_debug_enabled);
    //}
    //TLog::log->setLogFileEnabled(pref->log_file);

	// Load translation
	loadTranslation();
	installTranslator(&app_trans);
	installTranslator(&qt_trans);

	// Fonts
#ifdef Q_OS_WIN
	TPaths::createFontFile();
#endif
}

QString getArgName(const QString& arg) {

	if (arg.left(2) == "--") {
		return arg.mid(2);
	}
	if (arg.left(1) == "-") {
		return arg.mid(1);
	}

#ifdef Q_OS_WIN
	if (arg.left(1) == "/") {
		return arg.mid(1);
	}
#endif

	return "";
}

bool TApp::processArgName(const QString& name, const QStringList& args) const {

	for (int i = 0; i < args.size(); i++) {
		if (getArgName(args.at(i)) == name) {
			return true;
		}
	}

	return false;
}

int TApp::processArgPos(const QString& name, const QStringList& args) const {

	int pos = args.indexOf("--" + name);
	if (pos < 0) {
		pos = args.indexOf("-" + name);

#ifdef Q_OS_WIN
		if (pos < 0) {
			pos = args.indexOf("/" + name);
		}
#endif

	}

	return pos;
}

TApp::ExitCode TApp::processArgs() {

	QStringList args = arguments();

	// Uninstall Windows file associations
#ifdef Q_OS_WIN
    if (processArgName("uninstall", args)) {
#if USE_ASSOCIATIONS
		// Called by uninstaller. Will restore old associations.
		WinFileAssoc RegAssoc; 
        QStringList regExts;
        RegAssoc.GetRegisteredExtensions(extensions.multimedia(), regExts);
		RegAssoc.RestoreFileAssociations(regExts);
		// TODO: send to log
		printf("TApp::processArgs: restored associations\n");
#endif
		return NoError;
	}
#endif

	// Get config path from args
	int pos = processArgPos("config-path", args);
	if (pos >= 0) {
		pos++;
		if (pos < args.count()) {
			initial_config_path = args[pos];
			// Delete from list
			args.removeAt(pos);
			args.removeAt(pos - 1);
		} else {
			printf("TApp::processArgs: error: expected path after --config-path\r\n");
			return TApp::ErrorArgument;
		}
	}

	// Load preferences, setup logging and translation
	loadConfig();

	if (processArgName("delete-config", args)) {
		TCleanConfig::clean(TPaths::configPath());
		return NoError;
	}

	showInfo();

	QString action; // Action to be passed to running instance
	bool show_help = false;
	bool add_to_playlist = false;

	for (int n = 1; n < args.count(); n++) {
		QString argument = args[n];
		QString name = getArgName(argument);

		if (name == "debug") {

		} else if (name == "send-action") {
			if (n+1 < args.count()) {
				n++;
				action = args[n];
			} else {
				printf("Error: expected parameter for --send-action\r\n");
				return ErrorArgument;
			}
		} else if (name == "actions") {
			if (n+1 < args.count()) {
				n++;
				actions_list = args[n];
			} else {
				printf("Error: expected parameter for --actions\r\n");
				return ErrorArgument;
			}
		} else if (name == "sub") {
			if (n+1 < args.count()) {
				n++;
				QString file = args[n];
				if (QFile::exists(file)) {
					subtitle_file = QFileInfo(file).absoluteFilePath();
				} else {
					printf("Error: file '%s' doesn't exists\r\n", file.toUtf8().constData());
				}
			} else {
				printf("Error: expected parameter for --sub\r\n");
				return ErrorArgument;
			}
		} else if (name == "media-title") {
			if (n+1 < args.count()) {
				n++;
				if (media_title.isEmpty()) media_title = args[n];
			}
		} else if (name == "pos") {
			if (n + 2 < args.count()) {
				bool ok_x, ok_y;
				n++;
				gui_position.setX(args[n].toInt(&ok_x));
				n++;
				gui_position.setY(args[n].toInt(&ok_y));
				if (ok_x && ok_y) move_gui = true;
			} else {
				printf("Error: expected parameter for --pos\r\n");
				return ErrorArgument;
			}
		} else if (name == "size") {
			if (n+2 < args.count()) {
				bool ok_width, ok_height;
				n++;
				gui_size.setWidth(args[n].toInt(&ok_width));
				n++;
				gui_size.setHeight(args[n].toInt(&ok_height));
				if (ok_width && ok_height) resize_gui = true;
			} else {
				printf("Error: expected 2 parameters for --size\r\n");
				return ErrorArgument;
			}
		} else if (name == "help" || name == "h" || name == "?") {
			show_help = true;
		} else if (name == "close-at-end") {
			close_at_end = 1;
		} else if (name == "no-close-at-end") {
			close_at_end = 0;
		} else if (name == "fullscreen") {
			start_in_fullscreen = 1;
		} else if (name == "no-fullscreen") {
			start_in_fullscreen = 0;
		} else if (name == "add-to-playlist") {
			add_to_playlist = true;
		} else if (name == "ontop") {
			Settings::pref->stay_on_top = Settings::TPreferences::AlwaysOnTop;
		} else if (name == "no-ontop") {
			Settings::pref->stay_on_top = Settings::TPreferences::NeverOnTop;
		} else {
			// File
			QUrl fUrl = QUrl::fromUserInput(argument);
			if (fUrl.isValid() && fUrl.scheme().toLower() == "file") {
				argument = fUrl.toLocalFile();
			}
			if (QFile::exists(argument)) {
				argument = QFileInfo(argument).absoluteFilePath();
			}
			files_to_play.append(argument);
		}
	}

	if (show_help) {
		printf("%s\n", CLHelp::help().toLocal8Bit().data());
		return NoError;
	}

    if (Settings::pref->use_single_window) {
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
					if (add_to_playlist)
						command = "add_to_playlist";
					sendMessage(command + " " + files_to_play.join(" <<sep>> "));
				}
			}

			return NoError;
		}
	}

	return TApp::NoExit;
}

void TApp::createGUI() {
    logger()->debug("createGUI: creating main window Gui::TDefault");

	main_window = new Gui::TDefault();

    logger()->debug("createGUI: loading window config");
	main_window->loadConfig();

	main_window->setForceCloseOnFinish(close_at_end);
	main_window->setForceStartInFullscreen(start_in_fullscreen);

	connect(main_window, SIGNAL(requestRestart(bool)),
			this, SLOT(onRequestRestart(bool)));
	connect(this, SIGNAL(messageReceived(const QString&)),
			main_window, SLOT(handleMessageFromOtherInstances(const QString&)));

    setActivationWindow(main_window);

	if (move_gui) {
        main_window->move(gui_position);
	}
	if (resize_gui) {
        main_window->resize(gui_size);
	}

    logger()->debug("createGUI: created main window Gui::TDefault");
} // createGUI()

QString TApp::loadStyleSheet(const QString& filename) {
    logger()->debug("loadStyleSheet: '" + filename + "'");

	QFile file(filename);
	file.open(QFile::ReadOnly);
	QString stylesheet = QLatin1String(file.readAll());

	QString path;
	if (Images::has_rcc) {
		path = ":/" + pref->iconset;
	} else {
		QDir current = QDir::current();
		QString td = Images::themesDirectory();
		path = current.relativeFilePath(td);
	}

	stylesheet.replace(QRegExp("url\\s*\\(\\s*([^\\);]+)\\s*\\)",
							   Qt::CaseSensitive, QRegExp::RegExp2),
						QString("url(%1\\1)").arg(path + "/"));
	return stylesheet;
}

void TApp::changeStyleSheet(const QString& style) {
    logger()->debug("changeStyleSheet: '" + style + "'");

	// Load default stylesheet
	QString stylesheet = loadStyleSheet(":/default-theme/style.qss");

	if (!style.isEmpty()) {
		// Check main.css
		QString qss_file = TPaths::configPath() + "/themes/" + pref->iconset + "/main.css";
		if (!QFile::exists(qss_file)) {
			qss_file = TPaths::themesPath() +"/"+ pref->iconset + "/main.css";
		}

		// Check style.qss
		if (!QFile::exists(qss_file)) {
			qss_file = TPaths::configPath() + "/themes/" + pref->iconset + "/style.qss";
			if (!QFile::exists(qss_file)) {
				qss_file = TPaths::themesPath() +"/"+ pref->iconset + "/style.qss";
			}
		}

		// Load style file
		if (QFile::exists(qss_file)) {
			stylesheet += loadStyleSheet(qss_file);
		}
	}

    setStyleSheet(stylesheet);
}

void TApp::changeStyle() {
    logger()->debug("changeStyle");

	// Set application style
	// TODO: from help: Warning: To ensure that the application's style is set
	// correctly, it is best to call this function before the QApplication
	// constructor, if possible.
	if (!pref->style.isEmpty()) {
		// Remove a previous stylesheet to prevent a crash
		setStyleSheet("");
		setStyle(pref->style);
	} else if (reset_style) {
		setStyleSheet("");
		setStyle(default_style);
	}

	// Set theme
	Images::setTheme(pref->iconset);

	// Set stylesheets
	changeStyleSheet(pref->iconset);

    // Set icon style
    iconProvider.setStyle(style());
}

void TApp::start() {
    logger()->debug("start");

	// Setup style
	changeStyle();

	// Create the main window. It will be destoyed when leaving exec().
	createGUI();

	if (!main_window->startHidden() || !files_to_play.isEmpty())
		main_window->show();

    if (files_to_play.isEmpty()) {
        main_window->getCore()->setState(STATE_STOPPED);
    } else {
		if (!subtitle_file.isEmpty())
			main_window->setInitialSubtitle(subtitle_file);
		if (!media_title.isEmpty())
			main_window->getCore()->addForcedTitle(files_to_play[0], media_title);
		main_window->openFiles(files_to_play, current_file);
	}

	if (!actions_list.isEmpty()) {
		if (files_to_play.isEmpty()) {
			main_window->runActions(actions_list);
		} else {
			main_window->runActionsLater(actions_list);
		}
	}

	// Free files_to_play
	files_to_play.clear();
}

void TApp::onRequestRestart(bool reset_style) {
    logger()->debug("onRequestRestart");

	requested_restart = true;
	start_in_fullscreen = pref->fullscreen;
	this->reset_style = reset_style;

    Gui::Playlist::TPlaylist* playlist = main_window->getPlaylist();
	playlist->getFilesAppend(files_to_play);
    current_file = playlist->playingFile();

	// Rebuild playlist from scratch when restarting a disc. Playing the whole
	// disc gives less problems as playing the seperate tracks from the playlist,
	// especially for DVDNAV.
	if (files_to_play.count() > 1) {
        if (current_file.isEmpty()) {
            current_file = files_to_play[0];
		}
        TDiscName disc(current_file);
		if (disc.valid) {
			files_to_play.clear();
			disc.title = 0;
			files_to_play.append(disc.toString());
            current_file = "";
		}
	}
}

int TApp::execWithRestart() {

	int exit_code;
	do {
		requested_restart = false;
		start();
        logger()->debug("execWithRestart: calling exec()");
		exit_code = exec();
        logger()->debug("execWithRestart: exec() returned "
                        + QString::number(exit_code));
		if (requested_restart) {
            logger()->debug("execWithRestart: restarting");
			// Free current settings
			delete Settings::pref;
			// Reload configuration
			loadConfig();
		}
	} while (requested_restart);

	return exit_code;
}

void TApp::showInfo() {
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
    QString s = tr("This is WZPlayer %1 thinking it is running on %2")
                .arg(TVersion::version)
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

    logger()->info(s);
    logger()->info("Compiled with Qt version %1, running on Qt version %2", QT_VERSION_STR,
                 qVersion());
    logger()->debug("application path: '%1'", applicationDirPath());
    logger()->debug("data path: '%1'", TPaths::dataPath());
    logger()->debug("translation path: '%1'", TPaths::translationPath());
    logger()->debug("doc path: '%1'", TPaths::docPath());
    logger()->debug("themes path: '%1'", TPaths::themesPath());
    logger()->debug("shortcuts path: '%1'", TPaths::shortcutsPath());
    logger()->debug("config path: '%1'", TPaths::configPath());
    logger()->debug("file for subtitle styles: '%1'", TPaths::subtitleStyleFile());
    logger()->debug("current path: '%1'", QDir::currentPath());
#ifdef Q_OS_WIN
    logger()->debug("font path: '%1'", TPaths::fontPath());
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

bool TApp::winEventFilter(MSG* msg, long* result) {

	static uint last_appcommand = 0;

	if (msg->message == WM_KEYDOWN) {
        //logger()->debug("TApp::winEventFilter: WM_KEYDOWN: %X", msg->wParam);
		bool eat_key = false;
		if ((last_appcommand == APPCOMMAND_MEDIA_NEXTTRACK) && (msg->wParam == VK_MEDIA_NEXT_TRACK)) eat_key = true;
		else
		if ((last_appcommand == APPCOMMAND_MEDIA_PREVIOUSTRACK) && (msg->wParam == VK_MEDIA_PREV_TRACK)) eat_key = true;
		else
		if ((last_appcommand == APPCOMMAND_MEDIA_PLAY_PAUSE) && (msg->wParam == VK_MEDIA_PLAY_PAUSE)) eat_key = true;
		else
		if ((last_appcommand == APPCOMMAND_MEDIA_STOP) && (msg->wParam == VK_MEDIA_STOP)) eat_key = true;

		if (eat_key) {
            logger()->debug("TApp::winEventFilter: ignoring key %X", msg->wParam);
			last_appcommand = 0;
			*result = true;
			return true;
		}
	}
	else
	if (msg->message == WM_APPCOMMAND) {

		uint cmd  = GET_APPCOMMAND_LPARAM(msg->lParam);
		uint uDevice = GET_DEVICE_LPARAM(msg->lParam);
		uint dwKeys = GET_KEYSTATE_LPARAM(msg->lParam);

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
				QWidget* w = QApplication::focusWidget();
				if (w) QCoreApplication::sendEvent(w, &event);
				*result = true;
				return true;
			}
		//}
	}

	return false;
}
#endif // USE_WINEVENTFILTER

#include "moc_app.cpp"
