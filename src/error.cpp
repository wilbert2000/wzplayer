#include "error.h"
#include <QApplication>

QString TError::message(int id) {

	static const char* c = "TError";
	static const char* msgs[] = {
		QT_TRANSLATE_NOOP(c, "The player failed to start. Check the player path in preferences."),
		QT_TRANSLATE_NOOP(c, "The player unexpectedly quit"),
		QT_TRANSLATE_NOOP(c, "Timeout waiting for player"),
		QT_TRANSLATE_NOOP(c, "Error trying to read from player"),
		QT_TRANSLATE_NOOP(c, "Error trying to write to player"),
		QT_TRANSLATE_NOOP(c, "File not found"),
		QT_TRANSLATE_NOOP(c, "Failed to recognize file format"),
		QT_TRANSLATE_NOOP(c, "No disc in device")
	};

	if (id < ERR_FIRST_ID || id > ERR_LAST_ID) {
		id = ERR_CRASHED;
	}
	return qApp->translate(c, msgs[id - ERR_FIRST_ID]);
}
