#include "log.h"
#include <stdio.h>
#include <QTime>
#include "paths.h"
#include "global.h"
#include "logwindow.h"


const int LOG_BUF_LENGTH = 32768;

TLog::TLog(bool log_enabled, bool log_file_enabled, const QString& debug_filter) :
	enabled(log_enabled),
	filter(debug_filter),
	log_window(0) {

	// Open log file
	if (log_file_enabled) {
		file.setFileName(Paths::configPath() + "/smplayer_log.txt");
		file.open(QIODevice::WriteOnly);
	}

	// Reserve a buf for logLine()
	lines.reserve(LOG_BUF_LENGTH);

	// Install message handler
#if QT_VERSION >= 0x050000
	qInstallMessageHandler(messageHandler);
#else
	qInstallMsgHandler(msgHandler);
#endif
}

TLog::~TLog() {

	log_window = 0;
	if (file.isOpen()) {
		qDebug("Closing log file");
		file.close();
	}
}

void TLog::setLogWindow(LogWindow *window) {

	log_window = window;
	if (log_window) {
		log_window->setText(lines_back + lines);
	}
}

QString msgTypeToString(QtMsgType type) {

	switch (type) {
		case QtDebugMsg: return "Debug ";
		case QtWarningMsg: return "Warning ";
		case QtFatalMsg: return "Fatal ";
		default: return "Critical ";
	}
}

void TLog::logLine(QtMsgType type, QString line) {

	// Add timestamp, message type and line feed
	line = "["+ QTime::currentTime().toString("hh:mm:ss.zzz") +"] "
		 + msgTypeToString(type) + line + "\n";

	// Store line for log window
	if (lines.length() + line.length() > LOG_BUF_LENGTH) {
		lines_back = lines;
		lines = "";
		lines.reserve(LOG_BUF_LENGTH);
	}
	lines.append(line);

	// Output to STDERR
#ifdef OUTPUT_ON_CONSOLE
	QByteArray bytes = line.toUtf8();
	fwrite(bytes.constData(), 1, bytes.size(), stderr);
#endif

	// Output to log file
	if (file.isOpen()) {
#ifndef OUTPUT_ON_CONSOLE
		QByteArray bytes = line.toUtf8();
#endif
		file.write(bytes.constData(), bytes.size());
		file.flush();
	}

	// Pass line to log window
	if (log_window) {
		log_window->appendText(line);
	}
} // TLog::logLine()


#if QT_VERSION >= 0x050000
void TLog::messageHandler(QtMsgType type, const QMessageLogContext&,
						  const QString& msg) {
#else
void TLog::msgHandler(QtMsgType type, const char* p_msg) {

	QString msg = QString::fromUtf8(p_msg);
#endif

	if (Global::log
		&& Global::log->isEnabled()
		&& (type != QtDebugMsg || Global::log->passesFilter(msg))) {
		Global::log->logLine(type, msg);
	}
}

