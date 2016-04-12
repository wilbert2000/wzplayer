#include "log.h"
#include <QDebug>
#include <stdio.h>
#include <QTime>
#include <QDateTime>
#include "settings/paths.h"
#include "gui/logwindow.h"

// Size one log buffer, allocated twice
const int LOG_BUF_LENGTH = 2 * 65536 - 512;

// Static instance log
TLog* TLog::log = 0;


TLog::TLog(bool debug_enabled, bool log_file_enabled) :
	log_debug_messages(debug_enabled),
	log_debug_messages_to_console(debug_enabled),
	log_window(0) {

	// Reserve a buf for logLine()
	lines.reserve(LOG_BUF_LENGTH);

	// Install message handler
#if QT_VERSION >= 0x050000
	qInstallMessageHandler(messageHandler);
#else
	qInstallMsgHandler(msgHandler);
#endif

	// Start handling messages
	log = this;

	// Open log file
	if (log_file_enabled) {
		setLogFileEnabled(log_file_enabled);
	}

	qDebug("TLog::Tlog: started log at %s",
			QDateTime::currentDateTime().toString().toUtf8().data());
}

TLog::~TLog() {

	// Don't pass messages to window
	log_window = 0;

	qDebug("TLog::~Tlog: closing log at %s",
			QDateTime::currentDateTime().toString().toUtf8().data());

	// Stop handling messages
	log = 0;

	// Close log file
	file.close();
}

void TLog::setLogFileEnabled(bool log_file_enabled) {

	if (log_file_enabled) {
		// Open log file
		if (!file.isOpen()) {
			QString filename = Settings::TPaths::configPath() + "/log.txt";
			file.setFileName(filename);
			file.open(QIODevice::WriteOnly);
			if (file.isOpen()) {
				qDebug("TLog::setLogFileEnabled: opened log file '%s'",
					   filename.toUtf8().data());
			} else {
				qWarning("TLog::setLogFileEnabled: failed to open '%s'",
						 filename.toUtf8().data());
			}
		}
	} else {
		file.close();
	}
}

void TLog::setLogWindow(Gui::TLogWindow *window) {

	bool did_log = log_window;
	log_window = window;
	if (log_window) {
		log_window->setText(lines_back + lines);
		qDebug("TLog::setLogWindow: forwarding log to log window");
	} else if (did_log) {
		qDebug("TLog::setLogWindow: stopped logging to log window");
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

	// Output to console on stderr
#ifdef Q_OS_LINUX
	if (type != QtDebugMsg || log_debug_messages_to_console) {
		QByteArray bytes = line.toUtf8();
		fwrite(bytes.constData(), 1, bytes.size(), stderr);
	}
#endif

	// Output to log file
	if (file.isOpen()) {
		QByteArray bytes = line.toUtf8();
		file.write(bytes.constData(), bytes.size());
		file.flush();
	}

	// Pass line to log window
	if (log_window) {
		log_window->appendText(line);
	}
} // TLog::logLine()

// Message handler
#if QT_VERSION >= 0x050000
void TLog::messageHandler(QtMsgType type, const QMessageLogContext&,
						  const QString& msg) {
#else
void TLog::msgHandler(QtMsgType type, const char* p_msg) {

	QString msg = QString::fromUtf8(p_msg);
#endif

	if (log && (type != QtDebugMsg || log->logDebugMessages())) {
		log->logLine(type, msg);
	}
}

