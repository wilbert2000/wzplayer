#include "error.h"
#include <QApplication>

QString TError::message(int id) {

	static const char* c = "TError";
	static const char* msgs[] = {
		QT_TRANSLATE_NOOP(c, "The player failed to start. Please check the player path in the preferences dialog."),
		QT_TRANSLATE_NOOP(c, "The player quit unexpectedly"),
		QT_TRANSLATE_NOOP(c, "Timeout waiting for the player"),
		QT_TRANSLATE_NOOP(c, "Error trying to read from the player"),
		QT_TRANSLATE_NOOP(c, "Error trying to write to the player"),
		QT_TRANSLATE_NOOP(c, "File not found"),
		QT_TRANSLATE_NOOP(c, "Failed to open file"),
		QT_TRANSLATE_NOOP(c, "Failed to recognize file format"),
		QT_TRANSLATE_NOOP(c, "No disc in device"),
		QT_TRANSLATE_NOOP(c, "HTTP 403: the server refused access to the file"),
		QT_TRANSLATE_NOOP(c, "HTTP 404: file not found on the server"),
		QT_TRANSLATE_NOOP(c, "No stream found at the given URL")
	};

	if (id < ERR_FIRST_ID || id > ERR_LAST_ID) {
		id = ERR_CRASHED;
	}
	return qApp->translate(c, msgs[id - ERR_FIRST_ID]);
}
