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
#include "gui/mainwindowplus.h"
#include "gui/playlist/playlist.h"
#include "player/player.h"

#include "settings/preferences.h"
#include "settings/cleanconfig.h"
#include "settings/paths.h"

#include "config.h"
#include "version.h"
#include "clhelp.h"
#include "images.h"
#include "iconprovider.h"

#include <QFile>
#include <QDir>
#include <QTranslator>
#include <QLocale>
#include <QStyle>
#include <QClipboard>


#ifdef Q_OS_WIN
#include "settings/paths.h"
#if USE_ASSOCIATIONS
#include "extensions.h"
#include "winfileassoc.h" // Required for uninstall
#endif
#endif


// Statics that need to survive a restart
bool TApp::restarting = false;
bool TApp::addCommandLineFiles = true;
TApp::TStartFS TApp::start_in_fullscreen = TApp::FS_NOT_SET;
QString TApp::current_file;
QStringList TApp::files_to_play;


TApp::TApp(int& argc, char** argv) :
    QtSingleApplication(TConfig::PROGRAM_ID, argc, argv),
    main_window(0),
    move_gui(false),
    resize_gui(false),
    close_at_end(-1) {

    setOrganizationName(TConfig::PROGRAM_ORG);
    setApplicationName(TConfig::PROGRAM_ID);
    //setOrganizationDomain();
    //setApplicationVersion(TConfig::PROGRAM_VERSION);

    // Enable icons in menus
    setAttribute(Qt::AA_DontShowIconsInMenus, false);
}

TApp::~TApp() {

    Settings::TPreferences* p = Settings::pref;
    Settings::pref = 0;
    delete p;
}

QString TApp::loadStyleSheet(const QString& filename) {
    WZDEBUG("'" + filename + "'");

    QFile file(filename);
    file.open(QFile::ReadOnly);
    QString stylesheet = QLatin1String(file.readAll());

    QString path;
    if (Images::has_rcc) {
        path = ":/" + Settings::pref->iconset;
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
    WZDEBUG("'" + style + "'");

    // Load default stylesheet
    QString stylesheet = loadStyleSheet(":/default-theme/style.qss");

    if (!style.isEmpty()) {
        // Check main.css
        QString qss_file = Settings::TPaths::configPath() + "/themes/"
                           + Settings::pref->iconset + "/main.css";
        if (!QFile::exists(qss_file)) {
            qss_file = Settings::TPaths::themesPath() + "/"
                       + Settings::pref->iconset + "/main.css";
        }

        // Check style.qss
        if (!QFile::exists(qss_file)) {
            qss_file = Settings::TPaths::configPath() + "/themes/"
                       + Settings::pref->iconset + "/style.qss";
            if (!QFile::exists(qss_file)) {
                qss_file = Settings::TPaths::themesPath() +"/"
                           + Settings::pref->iconset + "/style.qss";
            }
        }

        // Load style file
        if (QFile::exists(qss_file)) {
            stylesheet += loadStyleSheet(qss_file);
        }
    }

    setStyleSheet(stylesheet);
}

void TApp::setupStyle() {
    WZDEBUG("");

    // Set application style
    // TODO: from help: Warning: To ensure that the application's style is set
    // correctly, it is best to call this function before the QApplication
    // constructor, if possible.
    if (!Settings::pref->style.isEmpty()) {
        setStyle(Settings::pref->style);
    }

    // Set theme
    Images::setTheme(Settings::pref->iconset);

    // Set stylesheets
    changeStyleSheet(Settings::pref->iconset);

    // Set icon style
    iconProvider.setStyle(style());
}

bool TApp::loadCatalog(QTranslator& translator,
                       const QString& name,
                       const QString& locale,
                       const QString& dir) {

    QString loc = name + "_" + locale;
    bool r = translator.load(loc, dir);
    if (r) {
        WZINFO("loaded '" + loc + "' from '" + dir + "'");
    } else {
        WZDEBUG("skipped loading of '" + loc + "' from '" + dir + "'");
    }
    return r;
}

void TApp::loadTranslation() {
    WZDEBUG("");

    QString locale = Settings::pref->language;
    if (locale.isEmpty()) {
        locale = QLocale::system().name();
    }
    QString trans_path = Settings::TPaths::translationPath();

    // Try to load it first from app path (in case there's an updated
    // translation), if it fails it will try then from the Qt path.
    if (!loadCatalog(qt_trans, "qt", locale, trans_path)) {
        loadCatalog(qt_trans, "qt", locale,
                    Settings::TPaths::qtTranslationPath());
    }
    loadCatalog(app_trans, TConfig::PROGRAM_ID, locale, trans_path);
}

void TApp::loadConfig() {
    WZDEBUG("");

    // Setup config directory
    Settings::TPaths::setConfigPath(initial_config_path);
    // Create settings
    Settings::pref = new Settings::TPreferences();
    // Load settings
    Settings::pref->load();
    // Setup style
    setupStyle();
    // Load translation
    loadTranslation();
    // Install translations
    installTranslator(&app_trans);
    installTranslator(&qt_trans);
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

TApp::TExitCode TApp::processArgs() {

    QStringList args = arguments();

    // Uninstall Windows file associations
#ifdef Q_OS_WIN
    if (processArgName("uninstall", args)) {
#if USE_ASSOCIATIONS
        // Called by uninstaller. Will restore old associations.
        WinFileAssoc RegAssoc;
        QStringList regExts;
        RegAssoc.GetRegisteredExtensions(extensions.allPlayable(), regExts);
        RegAssoc.RestoreFileAssociations(regExts);
        WZINFO("restored associations");
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
            WZINFO("configuration path set to '" + initial_config_path + "'");
        } else {
            WZERROR("expected path after option --config-path");
            return ERROR_INVALID_ARGUMENT;
        }
    }

    // Load preferences, set style and load translation
    loadConfig();

    if (processArgName("delete-config", args)) {
        Settings::TCleanConfig::clean();
        return NO_ERROR;
    }

    showInfo();

    QString send_action; // Action to be passed to running instance
    bool add_to_playlist = false;

    for (int n = 1; n < args.count(); n++) {
        QString argument = args[n];
        QString name = getArgName(argument);

        if (name == "debug" || name == "trace") {

        } else if (name == "send-action") {
            if (n + 1 < args.count()) {
                n++;
                send_action = args[n];
            } else {
                WZERROR("expected action after option --send-action");
                return ERROR_INVALID_ARGUMENT;
            }
        } else if (name == "actions") {
            if (n+1 < args.count()) {
                n++;
                actions = args[n];
            } else {
                WZERROR("expected actions after option --actions");
                return ERROR_INVALID_ARGUMENT;
            }
        } else if (name == "sub") {
            if (n + 1 < args.count()) {
                n++;
                QString file = args[n];
                if (QFile::exists(file)) {
                    subtitle_file = QFileInfo(file).absoluteFilePath();
                } else {
                    WZERROR("file '" + file + "' doesn\'t exists");
                }
            } else {
                WZERROR("expected parameter after option --sub");
                return ERROR_INVALID_ARGUMENT;
            }
        } else if (name == "media-title") {
            if (n + 1 < args.count()) {
                n++;
                if (media_title.isEmpty()) {
                    media_title = args[n];
                }
            }
        } else if (name == "close-at-end") {
            close_at_end = 1;
        } else if (name == "no-close-at-end") {
            close_at_end = 0;
        } else if (name == "add-to-playlist") {
            add_to_playlist = true;
        } else if (name == "help" || name == "h" || name == "?") {
            if (!restarting) {
                printf("%s\n", CLHelp::help().toLocal8Bit().data());
                return NO_ERROR;
            }
        } else if (name == "pos") {
            if (n + 2 < args.count()) {
                bool ok_x, ok_y;
                n++;
                gui_position.setX(args[n].toInt(&ok_x));
                n++;
                gui_position.setY(args[n].toInt(&ok_y));
                if (!restarting && ok_x && ok_y) {
                    move_gui = true;
                }
            } else {
                WZERROR("expected x and y position after option --pos");
                return ERROR_INVALID_ARGUMENT;
            }
        } else if (name == "size") {
            if (n + 2 < args.count()) {
                bool ok_width, ok_height;
                n++;
                gui_size.setWidth(args[n].toInt(&ok_width));
                n++;
                gui_size.setHeight(args[n].toInt(&ok_height));
                if (!restarting && ok_width && ok_height) {
                    resize_gui = true;
                }
            } else {
                WZERROR("expected width and height after option --size");
                return ERROR_INVALID_ARGUMENT;
            }
        } else if (name == "fullscreen") {
            if (!restarting) {
                start_in_fullscreen = FS_TRUE;
            }
        } else if (name == "no-fullscreen") {
            if (!restarting) {
                start_in_fullscreen = FS_FALSE;
            }
        } else if (addCommandLineFiles) {
            WZDEBUG("adding '" + argument + "' to files to play");
            files_to_play.append(argument);
        }
    }

    if (Settings::pref->use_single_window) {
        // Single instance
        if (isRunning()) {
            sendMessage("Hello");

            if (!send_action.isEmpty()) {
                sendMessage("action " + send_action);
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
                    if (add_to_playlist) {
                        command = "add_to_playlist";
                    }
                    sendMessage(command + " " + files_to_play.join(" <<sep>> "));
                }
            }

            return NO_ERROR;
        }
    }

    return TApp::START_APP;
}

void TApp::createGUI() {
    WZDEBUG("creating main window");

    main_window = new Gui::TMainWindowPlus();

    WZDEBUG("loading window config");
    main_window->loadConfig();

    main_window->setForceCloseOnFinish(close_at_end);

    connect(main_window, SIGNAL(requestRestart()),
            this, SLOT(onRequestRestart()));
    connect(this, SIGNAL(messageReceived(const QString&)),
            main_window, SLOT(handleMessageFromOtherInstances(const QString&)));

    setActivationWindow(main_window);

    if (move_gui) {
        main_window->move(gui_position);
    }
    if (resize_gui) {
        main_window->resize(gui_size);
    }

    WZDEBUG("created main window");
} // createGUI()

bool TApp::acceptClipboardAsURL() {

    const QString txt = QApplication::clipboard()->text();
    if (txt.contains("\x0a") || txt.contains("\x0d")) {
        return false;
    }
    if (!txt.contains("/")
#ifdef Q_OS_WIN
            && !txt.contains("\\")
#endif
        ) {
        return false;
    }

    if (txt == Settings::pref->last_clipboard) {
        return false;
    }

    return true;
}

void TApp::start() {
    WZDEBUG("");

    // Create the main window. It will be destoyed when leaving exec().
    createGUI();

    // Show main window
    if (!main_window->startHidden() || !files_to_play.isEmpty()) {
        main_window->show();
    }

    if (files_to_play.isEmpty()) {
        // Nothing to open
        player->setState(Player::STATE_STOPPED);
        // Check clipboard
        if (actions.isEmpty() && acceptClipboardAsURL()) {
            actions = "open_url";
        }
    } else {
        if (!subtitle_file.isEmpty()) {
            player->setInitialSubtitle(subtitle_file);
        }
        if (!media_title.isEmpty()) {
            player->addForcedTitle(files_to_play[0], media_title);
        }
        main_window->openFiles(files_to_play, current_file);
    }

    main_window->runActionsLater(actions, files_to_play.count() == 0);

    // Free files_to_play
    files_to_play.clear();
    // No longer restarting
    restarting = false;
}

void TApp::onRequestRestart() {
    WZDEBUG("");

    restarting = true;
    if (Settings::pref->fullscreen && start_in_fullscreen != FS_TRUE) {
        start_in_fullscreen = FS_RESTART;
    }

    Gui::Playlist::TPlaylist* playlist = main_window->getPlaylist();
    playlist->getFilesToPlay(files_to_play);
    current_file = playlist->playingFile();
    player->saveRestartTime();
    addCommandLineFiles = files_to_play.count() == 0;
}

void TApp::showInfo() {

#ifdef Q_OS_WIN
    QString win_ver;
    switch (QSysInfo::WindowsVersion) {
        case QSysInfo::WV_2000: win_ver = "Windows 2000"; break;
        case QSysInfo::WV_XP: win_ver = "Windows XP"; break;
        case QSysInfo::WV_2003:
            win_ver = "Windows XP Professional x64/Server 2003";
            break;
        case QSysInfo::WV_VISTA: win_ver = "Windows Vista/Server 2008"; break;
        case QSysInfo::WV_WINDOWS7: win_ver = "Windows 7/Server 2008 R2"; break;
        case QSysInfo::WV_WINDOWS8: win_ver = "Windows 8/Server 2012"; break;
#if (QT_VERSION >= 0x050200)
        case QSysInfo::WV_WINDOWS8_1:
            win_ver = "Windows 8.1/Server 2012 R2";
            break;
#endif
#if (QT_VERSION >= 0x050500)
        case QSysInfo::WV_WINDOWS10: win_ver = "Windows 10"; break;
#endif
        case QSysInfo::WV_NT_based: win_ver = "NT-based Windows"; break;
        default: win_ver = QString("Unknown/Unsupported Windows OS"); break;
    }
#endif // ifdef Q_OS_WIN

    QString s = tr("WZPlayer %1 running on %2")
                .arg(TVersion::version)

#ifdef Q_OS_LINUX
                .arg("Linux");
#else
#ifdef Q_OS_WIN
                .arg("Windows (" + win_ver + ")");
#else
                .arg("a non-Linux, non-Windows OS");
#endif
#endif

    WZINFO(s);
    WZINFO(QString("Compiled with Qt version " QT_VERSION_STR
           ", running on Qt version ") + qVersion());
    WZINFO("application '" + applicationDirPath() + "'");
    WZINFO("data '" + Settings::TPaths::dataPath() + "'");
    WZINFO("translation '" + Settings::TPaths::translationPath() + "'");
    WZINFO("doc '" + Settings::TPaths::docPath() + "'");
    WZINFO("themes '" + Settings::TPaths::themesPath() + "'");
    WZINFO("shortcuts '" + Settings::TPaths::shortcutsPath() + "'");
    WZINFO("config '" + Settings::TPaths::configPath() + "'");
    WZINFO("subtitle styles '" + Settings::TPaths::subtitleStyleFile() + "'");
    WZINFO("current directory '" + QDir::currentPath() + "'");
}

#include "moc_app.cpp"
