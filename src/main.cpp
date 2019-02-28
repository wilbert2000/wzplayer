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
#include "gui/logwindowappender.h"
#include "wzdebug.h"


class main;
LOG4QT_DECLARE_STATIC_LOGGER(logger, ::)

void initLog4Qt(Log4Qt::Level level) {

    Log4Qt::Layout* layout;
    Log4Qt::Appender* appender =
            Log4Qt::LogManager::rootLogger()->appender("A1");
    if (appender) {
        WZDEBUG("using existing appender A1");
        if (level != Log4Qt::Level::INFO_INT) {
            Log4Qt::LogManager::rootLogger()->setLevel(level);
        }
        layout = appender->layout();
    } else {
        Log4Qt::LogManager::rootLogger()->setLevel(level);

        // Create console layout
        Log4Qt::TTCCLayout* tccLayout = new Log4Qt::TTCCLayout();
        tccLayout->setName("Layout");
        tccLayout->setDateFormat(Log4Qt::TTCCLayout::ABSOLUTEDATE);
        tccLayout->setThreadPrinting(false);
        tccLayout->activateOptions();

        // Create appender A1 for console if level set through cmd line option
        if (level != Log4Qt::Level::INFO_INT) {
            Log4Qt::ConsoleAppender* a = new Log4Qt::ConsoleAppender(tccLayout,
                Log4Qt::ConsoleAppender::STDERR_TARGET);
            a->setName("A1");
            a->activateOptions();

            // Set appender on root logger
            Log4Qt::LogManager::rootLogger()->addAppender(a);
        }

        layout = tccLayout;
    }

    // Let Log4Qt handle qDebug(), qWarning(), qCritical() and qFatal()
    Log4Qt::LogManager::setHandleQtMessages(true);
    Log4Qt::LogManager::qtLogger()->setLevel(Log4Qt::Logger::rootLogger()->level());

    // Create appender A2 for log window
    Gui::TLogWindow::appender = new Gui::TLogWindowAppender(layout);
    Gui::TLogWindow::appender->setName("A2");
    Gui::TLogWindow::appender->activateOptions();

    // Set log window appender on root logger
    Log4Qt::LogManager::rootLogger()->addAppender(Gui::TLogWindow::appender);

    WZINFO("root logger initialized on level "
           + Log4Qt::LogManager::rootLogger()->level().toString());
}

bool isOption(const QString& arg, const QString& name) {

    return arg == "--" + name || arg == "-" + name

#ifdef Q_OS_WIN
            || arg  == "/" + name
#endif
    ;
}

void getLevelFromOption(const char* arg, Log4Qt::Level& level) {

    QString s(arg);
    if (isOption(s, "info")) {
        level = Log4Qt::Level(Log4Qt::Level::INFO_INT);
    } else if (isOption(s, "debug")) {
        level = Log4Qt::Level(Log4Qt::Level::DEBUG_INT);
    } else if (isOption(s, "trace")) {
        level = Log4Qt::Level(Log4Qt::Level::TRACE_INT);
    }
}

Log4Qt::Level getLevel(int argc, char** argv) {

    Log4Qt::Level level(Log4Qt::Level::WARN_INT);
    for(int i = 0; i < argc; i++) {
        getLevelFromOption(argv[i], level);
    }

    return level;
}

int main(int argc, char** argv) {

    initLog4Qt(getLevel(argc, argv));

    int exitCode;
    do {
        WZDEBUG("creating application");
        TApp app(argc, argv);
        WZDEBUG("initializing application");
        exitCode = app.processArgs();
        if (exitCode == TApp::START_APP) {
            WZDEBUG("starting application");
            app.start();
            WZDEBUG("executing application");
            exitCode = app.exec();
        }
    } while (exitCode == TApp::START_APP);

    WZDEBUG("returning exit code " + QString::number(exitCode));
    return exitCode;
}

