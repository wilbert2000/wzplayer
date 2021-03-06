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

#ifndef PLAYER_PROCESS_PROCESS_H
#define PLAYER_PROCESS_PROCESS_H

#include <QProcess>
#include <QTime>

//! TProcess is a specialized QProcess designed to properly work with mplayer.

/*!
 It can split the mplayer status line into lines.
 It also provides some Qt 3 like functions like addArgument().
*/

namespace Player {
namespace Process {

class TProcess : public QProcess {
    Q_OBJECT
public:
    TProcess(QObject* parent, const QString& name);

    virtual void setExecutable(const QString& p) { program = p; }
    QString executable() { return program; }

    virtual void addArgument(const QString& a); //!< Add an argument

    void clearArguments();   //!< Clear the list of arguments
    QStringList arguments(); //!< Return the list of arguments

    void start();            //!< Start the process

protected slots:
    void readStdOut();       //!< Called for reading from standard output
    void procFinished();     //!< Called when the process has finished

protected:
    //! Called from readStdOut() and readTmpFile() to do all the work
    void genericRead(QByteArray buffer);
    virtual bool parseLine(QString& line) = 0;

protected:
    QString program;
    QStringList args;
    QByteArray remaining_output;

private:
    int line_count;
    QTime line_time;

    QString bytesToString(const char* bytes, int size);
    void handleLine(QString& line);
    const char* EOL(const char* start, const char* end);
};

} // namespace Process
} // namespace Player

#endif // PLAYER_PROCESS_PROCESS_H
