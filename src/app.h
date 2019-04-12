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

#ifndef APP_H
#define APP_H

#include "qtsingleapplication/QtSingleApplication"
#include "log4qt/logger.h"

#include <QtGlobal>
#include <QString>
#include <QStringList>
#include <QSettings>
#include <QTranslator>


namespace Gui {
class TMainWindowTray;
}

class TApp : public QtSingleApplication {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    enum TExitCode {
             ERROR_INVALID_ARGUMENT = -1,
             NO_OTHER_INSTANCE = -1,
             NO_ERROR = 0,
             START_APP = 1111
    };

    enum TStartFS {
        FS_NOT_SET = -1,
        FS_FALSE = 0,
        FS_TRUE = 1,
        FS_RESTART = 2
    };

    static TStartFS start_in_fullscreen;
    static bool acceptClipboardAsURL();

    explicit TApp(int& argc, char** argv);
    virtual ~TApp();

    void start();

    // Process command line arguments.
    // Returning anything but TExitCode::START_APP makes ::main() exit.
    TExitCode processArgs();

signals:
    void receivedMessage(QString msg);

private:
    static bool restarting;
    static bool addCommandLineFiles;
    static QString current_file;
    static QStringList files_to_play;

    Gui::TMainWindowTray* main_window;

    QTranslator app_trans;
    QTranslator qt_trans;

    QString subtitle_file;
    QString onLoadActions; //!< Actions to be run on startup
    QString media_title; //!< Force a title for the first file

    // Change position and size
    bool move_gui;
    QPoint gui_position;
    bool resize_gui;
    QSize gui_size;

    // Options to pass to gui
    int close_at_end; // -1 = not set, 1 = true, 0 false

    bool loadCatalog(QTranslator& translator,
                     const QString& name,
                     const QString& locale,
                     const QString& dir);
    void loadTranslation();
    void loadConfig(bool portable);
    QString loadStyleSheet(const QString& filename);
    void changeStyleSheet(const QString& style);
    void setupStyle();
    void createGUI();
    bool processArgName(const QString& arg, QStringList& args) const;
    void logInfo();

private slots:
    void onRequestRestart();
    void onMessageReceived(QString msg);
};

#endif // APP_H
