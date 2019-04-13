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
#include "gui/mainwindowtray.h"
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

    // Enable icons in menus
    setAttribute(Qt::AA_DontShowIconsInMenus, false);
}

TApp::~TApp() {

    Settings::TPreferences* p = Settings::pref;
    Settings::pref = 0;
    delete p;
}

QString TApp::loadStyleSheet(const QString& filename) {
    WZD << filename;

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
    WZD << style;

    using namespace Settings;

    // Load default stylesheet
    QString stylesheet = loadStyleSheet(":/default-theme/style.qss");

    if (!style.isEmpty()) {
        // Check main.css
        QString qss = TPaths::configPath() + "/themes/" + pref->iconset
                + "/main.css";
        if (!QFile::exists(qss)) {
            qss = TPaths::themesPath() + "/" + pref->iconset + "/main.css";
        }

        // Check style.qss
        if (!QFile::exists(qss)) {
            qss = TPaths::configPath() + "/themes/" + pref->iconset
                    + "/style.qss";
            if (!QFile::exists(qss)) {
                qss = TPaths::themesPath() +"/" + pref->iconset + "/style.qss";
            }
        }

        // Load style file
        if (QFile::exists(qss)) {
            stylesheet += loadStyleSheet(qss);
        }
    }

    setStyleSheet(stylesheet);
}

void TApp::setupStyle() {
    WZT;

    // Set application style
    // From help: Warning: To ensure that the application's style is set
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
    bool loaded = translator.load(loc, dir);
    if (loaded) {
        WZI << "Loaded" << loc << "from" << dir;
    } else {
        WZD << loc << "not found in" << dir;
    }
    return loaded;
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

void TApp::loadConfig(bool portable) {
    WZD;

    // Setup config dir
    Settings::TPaths::setConfigPath(portable);
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

bool TApp::processArgName(const QString& name, QStringList& args) const {

    for (int i = 0; i < args.size(); i++) {
        if (getArgName(args.at(i)) == name) {
            args.takeAt(i);
            return true;
        }
    }

    return false;
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
#endif
        return NO_ERROR;
    }
#endif

    // Load preferences, set style and load translation
    loadConfig(processArgName("portable", args));

    if (processArgName("delete-config", args)) {
        Settings::TCleanConfig::clean();
        return NO_ERROR;
    }

    logInfo();

    QString send_actions; // Action to be passed to running instance
    bool add_to_playlist = false;

#define MSG(s) printf("%s\n", s)

    for (int n = 1; n < args.count(); n++) {
        QString argument = args.at(n);
        QString name = getArgName(argument);

        if (name == "loglevel") {
            // Already handled by main
            n++;
        } else if (name == "send-actions") {
            if (n + 1 < args.count()) {
                n++;
                send_actions = args.at(n);
            } else {
                MSG("Expected action after option --send-actions");
                return ERROR_INVALID_ARGUMENT;
            }
        } else if (name == "onload-actions") {
            if (n + 1 < args.count()) {
                n++;
                onLoadActions = args.at(n);
            } else {
                MSG("Expected action after option --onload-actions");
                return ERROR_INVALID_ARGUMENT;
            }
        } else if (name == "sub") {
            if (n + 1 < args.count()) {
                n++;
                QString file = args.at(n);
                if (QFile::exists(file)) {
                    subtitle_file = QFileInfo(file).absoluteFilePath();
                } else {
                    MSG(("File '" + file + "' not found").toLocal8Bit().data());
                    return ERROR_INVALID_ARGUMENT;
                }
            } else {
                MSG("Expected file name after option --sub");
                return ERROR_INVALID_ARGUMENT;
            }
        } else if (name == "media-title") {
            if (n + 1 < args.count()) {
                n++;
                if (media_title.isEmpty()) {
                    media_title = args.at(n);
                }
            }
        } else if (name == "close-at-end") {
            close_at_end = 1;
        } else if (name == "no-close-at-end") {
            close_at_end = 0;
        } else if (name == "add-to-playlist") {
            add_to_playlist = true;
        } else if (name == "help" || name == "h" || name == "?") {
            MSG(CLHelp::help().toLocal8Bit().data());
            return NO_ERROR;
        } else if (name == "pos") {
            if (n + 2 < args.count()) {
                bool xOK, yOK;
                n++;
                gui_position.setX(args.at(n).toInt(&xOK));
                n++;
                gui_position.setY(args.at(n).toInt(&yOK));
                if (!restarting && xOK && yOK) {
                    move_gui = true;
                }
            } else {
                MSG("Expected x and y position after option --pos");
                return ERROR_INVALID_ARGUMENT;
            }
        } else if (name == "size") {
            if (n + 2 < args.count()) {
                bool widthOK, heightOK;
                n++;
                gui_size.setWidth(args.at(n).toInt(&widthOK));
                n++;
                gui_size.setHeight(args.at(n).toInt(&heightOK));
                if (!restarting && widthOK && heightOK) {
                    resize_gui = true;
                }
            } else {
                MSG("Expected width and height after option --size");
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
            WZD << "Adding" << argument << "to files to play";
            files_to_play.append(argument);
        }
    }

    if (!send_actions.isEmpty()) {
        if (isRunning()
                && sendMessage("hello")
                && sendMessage("send_actions " + send_actions)) {
            return NO_ERROR;
        }
        return NO_OTHER_INSTANCE;
    }

    // Single instance
    if (Settings::pref->use_single_window
            && isRunning()
            && sendMessage("hello")) {

        if (!subtitle_file.isEmpty()) {
            sendMessage("load_sub " + subtitle_file);
        }
        if (!media_title.isEmpty()) {
            sendMessage("media_title " + files_to_play[0] + " <<sep>> "
                        + media_title);
        }
        if (!files_to_play.isEmpty()) {
            QString cmd = add_to_playlist ? "add_to_playlist" : "open_files";
            sendMessage(cmd + " " + files_to_play.join(" <<sep>> "));
        }

        return NO_ERROR;
    }

    return TApp::START_APP;
}

void TApp::onMessageReceived(QString msg) {
    // The default timeout for sendMessage() is 5 seconds. To improve the chance
    // that we respond within 5 seconds, handle the message here and send it to
    // the main window on a queued connection.
    emit receivedMessage(msg);
}

void TApp::createGUI() {
    WZTRACE("Creating main window");

    main_window = new Gui::TMainWindowTray();

    connect(this, &TApp::messageReceived,
            this, &TApp::onMessageReceived);
    connect(this, &TApp::receivedMessage,
            main_window, &Gui::TMainWindowTray::showMainWindow,
            Qt::QueuedConnection);
    connect(this, &TApp::receivedMessage,
            main_window, &Gui::TMainWindowTray::onReceivedMessage,
            Qt::QueuedConnection);
    connect(main_window, &Gui::TMainWindowTray::requestRestart,
            this, &TApp::onRequestRestart);

    setActivationWindow(main_window);

    WZTRACE("Loading main window settings");
    main_window->loadSettings();

    main_window->setForceCloseOnFinish(close_at_end);

    if (move_gui) {
        main_window->move(gui_position);
    }
    if (resize_gui) {
        main_window->resize(gui_size);
    }

    WZTRACE("Created main window");
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
    WZT;

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
        if (onLoadActions.isEmpty() && acceptClipboardAsURL()) {
            onLoadActions = "open_url";
        }
    } else {
        if (!subtitle_file.isEmpty()) {
            player->setInitialSubtitle(subtitle_file);
        }
        if (!media_title.isEmpty()) {
            player->addForcedTitle(files_to_play[0], media_title);
        }
        main_window->getPlaylist()->openFiles(files_to_play, current_file);
    }

    main_window->runActionsLater(onLoadActions, files_to_play.count() == 0);

    // Free files_to_play
    files_to_play.clear();
    // No longer restarting
    restarting = false;
}

void TApp::onRequestRestart() {
    WZD;

    restarting = true;
    if (Settings::pref->fullscreen && start_in_fullscreen != FS_TRUE) {
        start_in_fullscreen = FS_RESTART;
    }

    Gui::Playlist::TPlaylist* playlist = main_window->getPlaylist();
    playlist->getFilesToPlay(files_to_play);
    current_file = playlist->playingFile();
    player->saveRestartState();
    addCommandLineFiles = files_to_play.count() == 0;
}

void TApp::logInfo() {

    QString s = tr("%1 %2 running on %3")
            .arg(TConfig::PROGRAM_NAME)
            .arg(TVersion::version)

#ifdef Q_OS_LINUX
                .arg("Linux");
#else

#ifdef Q_OS_WIN
                .arg("Windows");
#else
                .arg("an undetermined OS");
#endif

#endif

    WZI << s;
    WZI << "Compiled with Qt version" << QT_VERSION_STR
        << "running on Qt version" << qVersion();
    WZI << "Application dir" << applicationDirPath();
    WZI << "Configuration dir" << Settings::TPaths::configPath();
    WZI << "Data dir" << Settings::TPaths::dataPath();
    WZI << "Favorites dir" << Settings::TPaths::favoritesPath();
    WZI << "Screenshots dir" << Settings::pref->screenshot_directory;
    WZI << "Translation dir" << Settings::TPaths::translationPath();
    WZI << "Themes dir" << Settings::TPaths::themesPath();
    WZI << "Shortcuts dir" << Settings::TPaths::shortcutsPath();
    WZI << "Current dir" << QDir::currentPath();
}

#include "moc_app.cpp"
