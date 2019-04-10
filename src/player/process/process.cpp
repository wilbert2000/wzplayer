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
#include "log4qt/logger.h"


namespace Player {
namespace Process {

TProcess::TProcess(QObject* parent) :
    QProcess(parent),
    wzdebug(logger()),
    line_count(0) {

    setProcessChannelMode(QProcess::MergedChannels);

    connect(this, &TProcess::readyReadStandardOutput,
            this, &TProcess::readStdOut);
    connect(this, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(procFinished()));

    line_time.start();
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
    wzdebug.level = Log4Qt::Level::INFO_INT;
    wzdebug << "Start program:" << program << "args:" << args;
    wzdebug << wzdebug;
    wzdebug.level = Log4Qt::Level::DEBUG_INT;

    remaining_output.clear();

    QProcess::start(program, args, QIODevice::ReadWrite);
}

void TProcess::handleLine(QString& line) {

    if (!parseLine(line)) {
        WZTRACEOBJ("ignored");
    }

    line_count++;
    if (line_count % 10000 == 0) {
        WZDEBUGOBJ(QString("Parsed %1 lines at %2 lines per second")
                   .arg(line_count)
                   .arg(double(line_count * 1000) / line_time.elapsed()));
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

    remaining_output = remaining_output.mid(
        start - remaining_output.constData());
}

void TProcess::readStdOut() {
    genericRead(readAllStandardOutput());
}

void TProcess::procFinished() {

    qint64 ba = bytesAvailable();
    WZDEBUGOBJ("Available bytes " + QString::number(ba));
    // Read last output
    if (ba > 0) {
        readStdOut();
    }
}

} // namespace Process
} // namespace Player

#include "moc_process.cpp"
