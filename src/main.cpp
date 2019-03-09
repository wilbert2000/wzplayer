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
#include "settings/preferences.h"
#include "wzdebug.h"


class main;
LOG4QT_DECLARE_STATIC_LOGGER(logger, ::)

void initLog4Qt(Log4Qt::Level level) {

    using namespace Log4Qt;

    Layout* layout;
    Appender* appender = LogManager::rootLogger()->appender("A1");
    if (appender) {
        WZDEBUG("Using existing appender A1");
        if (Settings::TPreferences::log_override) {
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
        if (Settings::TPreferences::log_override) {
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

    WZINFO("Initialized root logger on level "
           + LogManager::rootLogger()->level().toString());
}

bool isOption(const QString& arg, const QString& name) {

    return arg == "--" + name || arg == "-" + name

#ifdef Q_OS_WIN
            || arg  == "/" + name
#endif
    ;
}

Log4Qt::Level getLevel(int argc, char** argv) {

    using namespace Log4Qt;

    Level level(Level::NULL_INT);
    for(int i = 0; i < argc; i++) {
        QString name = argv[i];
        if (isOption(name, "loglevel")) {
            i++;
            if (i < argc) {
                QString v = argv[i];
                if (v == "warn") {
                    level = Level(Log4Qt::Level::WARN_INT);
                } else if (v == "info") {
                    level = Level(Log4Qt::Level::INFO_INT);
                } else if (v == "debug") {
                    level = Level(Log4Qt::Level::DEBUG_INT);
                } else if (v == "trace") {
                    level = Level(Log4Qt::Level::TRACE_INT);
                } else {
                    WZWARN(QString("Invalid loglevel '%1'").arg(v));
                }
            } else {
                WZWARN("Expected log level info, debug or trace after"
                       " --loglevel");
            }
            break;
        }
    }

    if (level == Level::NULL_INT) {
        level = Settings::TPreferences::log_default_level;
    } else {
        Settings::TPreferences::log_override = true;
    }

    return level;
}

int main(int argc, char** argv) {

    initLog4Qt(getLevel(argc, argv));

    int exitCode;
    do {
        WZDEBUG("Creating application");
        TApp app(argc, argv);
        WZDEBUG("Initializing application");
        exitCode = app.processArgs();
        if (exitCode == TApp::START_APP) {
            WZDEBUG("Starting application");
            app.start();
            WZDEBUG("Executing application");
            exitCode = app.exec();
        }
    } while (exitCode == TApp::START_APP);

    WZDEBUG("Returning exit code " + QString::number(exitCode));
    return exitCode;
}
