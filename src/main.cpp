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
#include "gui/logwindowapeender.h"


using namespace Log4Qt;

class main;
LOG4QT_DECLARE_STATIC_LOGGER(logger, ::)

void initLog4Qt(Level level) {

    Log4Qt::Layout* layout;
    Appender* appender = LogManager::rootLogger()->appender("A1");
    if (appender) {
        logger()->debug("initLogQt: using existing appender A1");
        if (level != Level::INFO_INT) {
            LogManager::rootLogger()->setLevel(level);
        }
        layout = appender->layout();
    } else {
        LogManager::rootLogger()->setLevel(level);

        // Create console layout
        TTCCLayout* tccLayout = new TTCCLayout();
        tccLayout->setName("Layout");
        tccLayout->setDateFormat(TTCCLayout::ABSOLUTEDATE);
        tccLayout->setThreadPrinting(false);
        tccLayout->activateOptions();

        // Create appender A1 for console if level set through cmd line option
        if (level != Level::INFO_INT) {
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

    // Create appender A2 for log window
    Gui::TLogWindow::appender = new Gui::TLogWindowAppender(layout);
    Gui::TLogWindow::appender->setName("A2");
    Gui::TLogWindow::appender->activateOptions();

    // Set log window appender on root logger
    LogManager::rootLogger()->addAppender(Gui::TLogWindow::appender);

    logger()->info("initLog4Qt: root logger initialized on level %1",
                   LogManager::rootLogger()->level().toString());
}

bool isOption(const QString& arg, const QString& name) {

    return arg == "--" + name || arg == "-" + name

#ifdef Q_OS_WIN
            || arg  == "/" + name
#endif
    ;
}

void getLevelFromOption(const char* arg, Level& level) {

    QString s(arg);
    if (isOption(s, "debug")) {
        level = Level(Level::DEBUG_INT);
    } else if (isOption(s, "trace")) {
        level = Level(Level::TRACE_INT);
    }
}

Level getLevel(int argc, char** argv) {

    Level level(Level::INFO_INT);
    for(int i = 0; i < argc; i++) {
        getLevelFromOption(argv[i], level);
    }

    return level;
}

int main(int argc, char** argv) {

    initLog4Qt(getLevel(argc, argv));

    int exitCode;
    do {
        logger()->debug("main: creating application TApp");
        TApp app(argc, argv);
        logger()->debug("main: initialising application");
        exitCode = app.processArgs();
        if (exitCode == TApp::NoExit) {
            logger()->debug("main: starting application");
            app.start();
            logger()->debug("main: calling exec()");
            exitCode = app.exec();
            logger()->debug("main: exec() returned %1", exitCode);
        }
    } while (exitCode == TApp::NoExit);

    logger()->debug("main: returning exit code %1", exitCode);
    return exitCode;
}

