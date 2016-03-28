#ifndef PROC_ERRORMSG_H
#define PROC_ERRORMSG_H

#include <QString>
#include <QProcess>

namespace Proc {

class TErrorMsg {
public:
	enum TErrorMsgID {
		ERR_FIRST_ID = 4000,
		ERR_FAILED_TO_START = ERR_FIRST_ID,
		ERR_CRASHED,
		ERR_TIMEOUT,
		ERR_READ_ERROR,
		ERR_WRITE_ERROR,
		ERR_FILE_OPEN,
		ERR_OPEN,
		ERR_FILE_FORMAT,
		ERR_NO_DISC,
		ERR_HTTP_403,
		ERR_HTTP_404,
		ERR_NO_STREAM_FOUND,
		ERR_LAST_ID = ERR_NO_STREAM_FOUND
	};

	static QString message(int id);
	static void setExitCodeMsg(const QString& msg);
	static TErrorMsgID processErrorToErrorID(QProcess::ProcessError error) {
		return (TErrorMsgID) (ERR_FAILED_TO_START + error);
	}

private:
	static QString exitCodeMsg;
};

} // namespace Proc

#endif // PROC_ERRORMSG_H
