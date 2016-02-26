#ifndef LOG_H
#define LOG_H

#include <QString>
#include <QFile>

#if QT_VERSION >= 0x050000
#include <QMessageLogContext>
#endif


namespace Gui {
class TLogWindow;
}


class TLog {

public:
	TLog(bool debug_enabled, bool log_file_enabled);
	virtual ~TLog();

	static TLog* log;

	bool logDebugMessages() const { return log_debug_messages; }
	void setLogDebugMessages(bool enable) { log_debug_messages = enable; }
	void setLogFileEnabled(bool log_file_enabled);
	void setLogWindow(Gui::TLogWindow* window);

	void logLine(QtMsgType type, QString line);
	QString getLogLines() { return lines_back + lines; }

private:
	bool log_debug_messages;
	bool log_debug_messages_to_console;
	QString lines_back;
	QString lines;
	QFile file;
	Gui::TLogWindow* log_window;

#if QT_VERSION >= 0x050000
	static void messageHandler(QtMsgType type, const QMessageLogContext&, const QString& msg);
#else
	static void msgHandler(QtMsgType type, const char* msg);
#endif

}; // class TLog

#endif // LOG_H
