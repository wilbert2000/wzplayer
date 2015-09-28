#ifndef LOG_H
#define LOG_H

#include <QString>
#include <QFile>
#include <QRegExp>

#if QT_VERSION >= 0x050000
#include <QMessageLogContext>
#endif

class LogWindow;

class TLog {
public:
	TLog(bool log_enabled, bool log_file_enabled, const QString& debug_filter);
	~TLog();

	bool isEnabled() const { return enabled; }
	bool passesFilter(const QString& msg) { return filter.indexIn(msg) >= 0; }
	void logLine(QtMsgType type, QString line);
	void setLogWindow(LogWindow* window);
	QString getLogLines() { return lines_back + lines; }

private:
	bool enabled;
	QString lines_back;
	QString lines;
	QFile file;
	QRegExp filter;
	LogWindow* log_window;

#if QT_VERSION >= 0x050000
	static void messageHandler(QtMsgType type, const QMessageLogContext&, const QString& msg);
#else
	static void msgHandler(QtMsgType type, const char* msg);
#endif

}; // class TLog


#endif // LOG_H
