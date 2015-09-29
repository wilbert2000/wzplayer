/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

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

#include "process.h"
#include <QDebug>

namespace Proc {

TProcess::TProcess(QObject * parent) : QProcess(parent)
{
	clearArguments();
	setProcessChannelMode(QProcess::MergedChannels);
	
	connect(this, SIGNAL(readyReadStandardOutput()), this, SLOT(readStdOut()) );
	connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), 
			this, SLOT(procFinished()) );
}

void TProcess::clearArguments() {
	program = "";
	arg.clear();
}

bool TProcess::isRunning() {
	return (state() == QProcess::Running);
}

void TProcess::addArgument(const QString & a) {
	if (program.isEmpty()) {
		program = a;
	} else {
		arg.append(a);
	}
}

QStringList TProcess::arguments() {
	QStringList l = arg;
	l.prepend(program);
	return l;
}

void TProcess::start() {

	remaining_output.clear();
	QProcess::start(program, arg);
}

void TProcess::readStdOut() {
	genericRead( readAllStandardOutput() );
}

void TProcess::genericRead(QByteArray buffer) {
	QByteArray ba = remaining_output + buffer;
	int start = 0;
	int from_pos = 0;
	int pos = canReadLine(ba, from_pos);

	while ( pos > -1 ) {
		QByteArray line = ba.mid(start, pos-start);
		from_pos = pos + 1;
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
		if ((from_pos < ba.size()) && (ba.at(from_pos)=='\n')) from_pos++;
#endif
		start = from_pos;

		emit lineAvailable(line);

		pos = canReadLine(ba, from_pos);
	}

	remaining_output = ba.mid(from_pos);
}

int TProcess::canReadLine(const QByteArray & ba, int from) {
	int pos1 = ba.indexOf('\n', from);
	int pos2 = ba.indexOf('\r', from);

	if ( (pos1 == -1) && (pos2 == -1) ) return -1;

	int pos = pos1;
	if ( (pos1 != -1) && (pos2 != -1) ) {
		/*
		if (pos2 == (pos1+1)) pos = pos2; // \r\n
		else
		*/
		if (pos1 < pos2) pos = pos1; else pos = pos2;
	} else {
		if (pos1 == -1) pos = pos2;
		else
		if (pos2 == -1) pos = pos1;
	}

	return pos;
}

/*!
Do some clean up, and be sure that all output has been read.
*/
void TProcess::procFinished() {
	qDebug("Proc::TProcess::procFinished");

	qDebug() << "TProcess::procFinished: Bytes available: " << bytesAvailable();
	if ( bytesAvailable() > 0 ) readStdOut();
}

QStringList TProcess::splitArguments(const QString & args) {
	qDebug("Proc::TProcess::splitArguments: '%s'", args.toUtf8().constData());

	QStringList l;

	bool opened_quote = false;
	int init_pos = 0;
	for (int n = 0; n < args.length(); n++) {
		if ((args[n] == QChar(' ')) && (!opened_quote)) {
			l.append(args.mid(init_pos, n - init_pos));
			init_pos = n+1;
		}
		else
		if (args[n] == QChar('\"')) opened_quote = !opened_quote;

		if (n == args.length()-1) {
			l.append(args.mid(init_pos, (n - init_pos)+1));
		}
	}

	for (int n = 0; n < l.count(); n++) {
		qDebug("Proc::TProcess::splitArguments: arg: %d '%s'", n, l[n].toUtf8().constData());
	}

	return l;
}

} // namespace Proc

#include "moc_process.cpp"
