#ifndef ERROR_H
#define ERROR_H

#include <QString>
#include <QProcess>

class TError {

public:
	enum TErrorID {
		ERR_FIRST_ID = 4000,
		ERR_FAILED_TO_START = ERR_FIRST_ID,
		ERR_CRASHED,
		ERR_TIMEOUT,
		ERR_READ_ERROR,
		ERR_WRITE_ERROR,
		ERR_FILE_NOT_FOUND,
		ERR_OPEN,
		ERR_FILE_FORMAT,
		ERR_NO_DISC,
		ERR_HTTP_403,
		ERR_HTTP_404,
		ERR_NO_STREAM_FOUND,
		ERR_LAST_ID = ERR_NO_STREAM_FOUND
	};

	static QString message(int id);
	static TErrorID processErrorToErrorID(QProcess::ProcessError error) {
		return (TErrorID) (ERR_FAILED_TO_START + error);
	}
};

#endif // ERROR_H
