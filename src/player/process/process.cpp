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

#include "player/process/process.h"
#include <QDebug>
#include "log4qt/logger.h"


namespace Player {
namespace Process {

TProcess::TProcess(QObject* parent) :
    QProcess(parent),
    debug(logger()),
    line_count(0) {

    setProcessChannelMode(QProcess::MergedChannels);

    connect(this, SIGNAL(readyReadStandardOutput()),
            this, SLOT(readStdOut()));
    connect(this, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(procFinished()));

    line_time.start();
}

TProcess::~TProcess() {
}

void TProcess::clearArguments() {

    program = "";
    args.clear();
}

void TProcess::addArgument(const QString& a) {

    if (program.isEmpty()) {
        program = a;
    } else {
        args.append(a);
    }
}

QStringList TProcess::arguments() {

    QStringList l = args;
    l.prepend(program);
    return l;
}

void TProcess::start() {
    debug.level = Log4Qt::Level::INFO_INT;
    debug << "start: program:" << program << "args:" << args;
    debug << debug;
    debug.level = Log4Qt::Level::DEBUG_INT;

    remaining_output.clear();

    QProcess::start(program, args, QIODevice::ReadWrite);
}

void TProcess::handleLine(QString& line) {

    if (!parseLine(line)) {
        logger()->trace("handleLine: ignored");
    }

    line_count++;
    if (line_count % 10000 == 0) {
        logger()->debug("handleLine: parsed %1 lines at %2 lines per second",
                        QString::number(line_count),
                        QString::number((line_count * 1000.0)
                                        / line_time.elapsed()));
    }
}

QString TProcess::bytesToString(const char* bytes, int size) {
// memo: TColorUtils::stripColorsTags(QString::fromLocal8Bit(ba));

#ifdef Q_OS_WIN
    return QString::fromUtf8(bytes, size);
#else
    return QString::fromLocal8Bit(bytes, size);
#endif
}

// Needed because MPlayer uses \r for its status line
const char* TProcess::EOL(const char* start, const char* end) {

    const char* eol = start;
    while (eol < end && *eol != (char) 13 && *eol != (char) 10) {
        eol++;
    }
    return eol;
}

void TProcess::genericRead(QByteArray buffer) {

    remaining_output += buffer;

    const char* start = remaining_output.constData();
    const char* end = start + remaining_output.size();
    const char* pos = EOL(start, end);

    while (pos < end) {
        if (pos > start) {
            QString line = bytesToString(start, pos - start);
            handleLine(line);
        }
        start = pos + 1;
        pos = EOL(start, end);
    }

    remaining_output = remaining_output.mid(start
                                            - remaining_output.constData());
}

void TProcess::readStdOut() {
    genericRead(readAllStandardOutput());
}

/*!
Do some clean up, and be sure that all output has been read.
*/
void TProcess::procFinished() {
    logger()->debug("procFinished: bytes available: %1",
                    QString::number(bytesAvailable()));
    if (bytesAvailable() > 0) {
        readStdOut();
    }
}

QStringList TProcess::splitArguments(const QString& args) {

    QStringList l;

    bool opened_quote = false;
    int init_pos = 0;
    for (int n = 0; n < args.length(); n++) {
        if (args[n] == QChar(' ') && !opened_quote) {
            l.append(args.mid(init_pos, n - init_pos));
            init_pos = n + 1;
        } else if (args[n] == QChar('\"'))
            opened_quote = !opened_quote;

        if (n == args.length() - 1) {
            l.append(args.mid(init_pos, n - init_pos + 1));
        }
    }

    return l;
}

} // namespace Process
} // namespace Player

#include "moc_process.cpp"
