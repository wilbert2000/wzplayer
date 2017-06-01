#ifndef PLAYER_PROCESS_EXITMSG_H
#define PLAYER_PROCESS_EXITMSG_H

#include <QString>
#include <QProcess>

namespace Player {
namespace Process {

class TExitMsg {
public:
    enum TExitMsgID {
        EXIT_COUNT = 16,
        EXIT_LAST_ID = 255,
        ERR_FIRST_ID = EXIT_LAST_ID - EXIT_COUNT + 1,
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
        ERR_TITLE_NOT_FOUND,
        ERR_QUIT_WITHOUT_PLAY,
        EXIT_OUT_POINT_REACHED
    };

    static QString message(int id);
    static void setExitCodeMsg(const QString& msg);
    static TExitMsgID processErrorToErrorID(QProcess::ProcessError error) {
        return (TExitMsgID) (ERR_FAILED_TO_START + error);
    }

private:
    static QString exitCodeMsg;
};

} // namespace Process
} // namespace Player

#endif // PLAYER_PROCESS_EXITMSG_H
