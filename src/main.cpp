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

#include "QDateTime"
#include "log4qt/logger.h"
#include "log4qt/logmanager.h"
#include "log4qt/consoleappender.h"
#include "log4qt/ttcclayout.h"
#include "gui/logwindow.h"

using namespace Log4Qt;

class main;
LOG4QT_DECLARE_STATIC_LOGGER(logger, main)

void initLog4Qt() {

    Appender* appender = Logger::rootLogger()->appender("A1");
    if (appender) {
        logger()->debug("initLogQt: appender A1 already up and running");
    } else {
        // Create layout
        TTCCLayout* layout = new TTCCLayout();
        layout->setName("Layout");
        layout->setDateFormat(TTCCLayout::ABSOLUTE);
        layout->setThreadPrinting(false);
        layout->activateOptions();

        // Create an appender
        ConsoleAppender* a = new ConsoleAppender(layout,
            ConsoleAppender::STDERR_TARGET);
        a->setName("A1");
        a->activateOptions();

        // Set appender on root logger
        Logger::rootLogger()->addAppender(a);
        Logger::rootLogger()->setLevel(Level(Level::DEBUG_INT));
        appender = a;
    }

    // Let Log4Qt handle qDebug(), qWarning(), qCritical() and qFatal()
    LogManager::setHandleQtMessages(true);
    LogManager::qtLogger()->setLevel(Logger::rootLogger()->level());

    // Create appender for log window
    Gui::TLogWindow::appender = new Gui::TLogWindowAppender(appender->layout());
    Gui::TLogWindow::appender->setName("A2");
    Gui::TLogWindow::appender->activateOptions();

    // Set log window appender on root logger
    Logger::rootLogger()->addAppender(Gui::TLogWindow::appender);

    logger()->info("initLog4Qt: log initialized on "
                   + QDateTime::currentDateTime().toString());
}

int main(int argc, char** argv) {

    initLog4Qt();

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

    logger()->info("Exiting on %1", QDateTime::currentDateTime().toString());
    return exitCode;
}

