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

#include "log4qt/logger.h"
#include "log4qt/logmanager.h"
#include "log4qt/consoleappender.h"
#include "log4qt/ttcclayout.h"
#include "log4qt/level.h"
#include "gui/logwindow.h"

using namespace Log4Qt;

class main;
LOG4QT_DECLARE_STATIC_LOGGER(logger, main)

void initLog4Qt(bool debug) {

    Log4Qt::Layout* layout;
    Appender* appender = LogManager::rootLogger()->appender("A1");
    if (appender) {
        logger()->debug("initLogQt: appender A1 already created");
        if (debug) {
            LogManager::rootLogger()->setLevel(Level(Level::DEBUG_INT));
        }
        layout = appender->layout();
    } else {
        LogManager::rootLogger()->setLevel(Level(debug ? Level::DEBUG_INT
                                                       : Level::INFO_INT));

        // Create layout
        TTCCLayout* tccLayout = new TTCCLayout();
        tccLayout->setName("Layout");
        //tccLayout->setDateFormat(TTCCLayout::ABSOLUTEDATE);
        tccLayout->setThreadPrinting(false);
        tccLayout->activateOptions();

        // Create appender for console
        if (debug) {
            ConsoleAppender* a = new ConsoleAppender(tccLayout,
                ConsoleAppender::STDERR_TARGET);
            a->setName("A1");
            a->activateOptions();

            // Set appender on root logger
            LogManager::rootLogger()->addAppender(a);
        }

        layout = tccLayout;
    }

    // Let Log4Qt handle qDebug(), qWarning(), qCritical() and qFatal()
    LogManager::setHandleQtMessages(true);
    LogManager::qtLogger()->setLevel(Logger::rootLogger()->level());

    // Create an appender for log window
    Gui::TLogWindow::appender = new Gui::TLogWindowAppender(layout);
    Gui::TLogWindow::appender->setName("A2");
    Gui::TLogWindow::appender->activateOptions();

    // Set log window appender on root logger
    LogManager::rootLogger()->addAppender(Gui::TLogWindow::appender);

    logger()->info("initLog4Qt: log initialized on level %1",
                   LogManager::rootLogger()->level().toString());
}

bool isDebug(const char* arg) {

    QString s(arg);
    return s == "--debug" || s == "-debug"

#ifdef Q_OS_WIN
            || s == "/debug"
#endif
    ;
}

int main(int argc, char** argv) {

    bool debug = false;
    for(int i = 0; i < argc; i++) {
        if (isDebug(argv[i])) {
            debug = true;
            break;
        }
    }

    initLog4Qt(debug);

    int exitCode;
    do {
        logger()->debug("Creating application");
        TApp app(argc, argv);
        logger()->debug("Initialising application");
        exitCode = app.processArgs();
        if (exitCode == TApp::NoExit) {
            logger()->debug("Starting application");
            app.start();
            logger()->debug("Calling exec()");
            exitCode = app.exec();
            logger()->debug("exec() returned %1", exitCode);
        }
    } while (exitCode == TApp::NoExit);

    logger()->debug("returning %1", exitCode);
    return exitCode;
}

