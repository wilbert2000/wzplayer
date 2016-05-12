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

#include <QDateTime>
#include "app.h"

#include "log4qt/logmanager.h"
#include "log4qt/consoleappender.h"
#include "log4qt/logger.h"
#include "log4qt/ttcclayout.h"


bool processArgName(const QString& name, char* arg) {

	return (arg == "--" + name)
		|| (arg == "-" + name)
#ifdef Q_OS_WIN
		|| (arg == "/" + name)
#endif
		;
}

void initLog4Qt(bool debug) {

    // Let Log4Qt handle Debug(), qWarning(), qCritical() and qFatal()
    Log4Qt::LogManager::setHandleQtMessages(true);

    // Create a layout
    Log4Qt::TTCCLayout* layout = new Log4Qt::TTCCLayout();
    layout->setName("Layout");
    layout->activateOptions();

    // Create an appender
    Log4Qt::ConsoleAppender* appender = new Log4Qt::ConsoleAppender(
        layout, Log4Qt::ConsoleAppender::STDOUT_TARGET);
    appender->setName("Console appender");
    appender->activateOptions();

    // Set appender on root logger
    Log4Qt::Logger::rootLogger()->addAppender(appender);

    Log4Qt::Logger::logger("main")->info("initLog4Qt: log initialized on "
        + QDateTime::currentDateTime().toString());
}

int main(int argc, char** argv) {

    // Setup initial logging
    bool log_debug = false;
    for(int i = 0; i < argc; i++) {
        if (processArgName("debug", argv[i])) {
            log_debug = true;
            break;
        }
    }
    initLog4Qt(log_debug);

    // Create and exec app
    TApp app(argc, argv);
    int exit_code = app.processArgs();
    if (exit_code == TApp::NoExit) {
        exit_code = app.execWithRestart();
    }

    Log4Qt::Logger::logger("main")->info("main: returning %1 on %2",
        exit_code, QDateTime::currentDateTime().toString());
    return exit_code;
}

