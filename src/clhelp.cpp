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

#include "clhelp.h"
#include "config.h"
#include <QObject>


const int TEXT_WIDTH = 80;

QString CLHelp::formatText(QString s, int col) {

    QString res = "";

    int last = 0;
    int pos;

    pos = s.indexOf(' ');
    while (pos != -1) {

        if (s.count() < col) {
            res = res + s;
            s = "";
            break;
        }

        last = s.indexOf('\n');
        if (last == -1 || last >= col) {
            last = 0;
            while (pos < col && pos != -1) {
                last = pos;
                pos = s.indexOf(' ', pos + 1);
            }
        }

        res = res + s.left(last) + "\n";
        s = s.mid(last + 1);

        pos = s.indexOf(' ');
    }

    if (!s.isEmpty()) res = res + s;

    return res;
}

QString CLHelp::formatHelp(QString parameter, QString help, bool html) {

    if (html) {
        help = help.replace("\n", "<br/>");
        return "<tr><td><b>" + parameter + "</b></td><td>" + help + "</td></tr>";
    } else {
        int par_width = 20;
        int help_width = TEXT_WIDTH - par_width;

        QString s;
        s = s.fill(' ', par_width - (parameter.count() + 2));
        s = s + parameter + ": ";

        QString f;
        f = f.fill(' ', par_width);

        QString s2 = formatText(help, help_width);
        int pos = s2.indexOf('\n');
        while (pos != -1) {
            s2 = s2.insert(pos + 1, f);
            pos = s2.indexOf('\n', pos + 1 + par_width);
        }

        return s + s2 + "\n";
    }
}


QString CLHelp::help(bool html) {

    QString options = QString("%1 [--help]"
                              " [--loglevel warn|info|debug|trace]"
#ifdef Q_OS_WIN
                              " [--uninstall]"
#endif
                              " [--send-actions %2]"
                              " [--onload-actions %2]"
                              " [--close-at-end] [--no-close-at-end]"
                              " [--fullscreen] [--no-fullscreen]"
                              " [--ontop] [--no-ontop]"
                              " [--sub %3]"
                              " [--pos x y]"
                              " [--size %4 %5]"
                              " [--portable]"
                              " [--add-to-playlist]"
                              " [%6] [%6]...")
                      .arg(TConfig::PROGRAM_ID)
                      .arg(QObject::tr("actions"))
                      .arg(QObject::tr("filename"))
                      .arg(QObject::tr("width")).arg(QObject::tr("height"))
                      .arg(QObject::tr("media"));

    QString s;

    if (html) {
        s = QObject::tr("Usage:") + " <b>" + options + "</b><br>";
        s += "<table>";
    } else {
        s = formatText(QObject::tr("Usage:") + " " + options, TEXT_WIDTH);
        s += "\n\n";
    }

    s += formatHelp("--help", QObject::tr("Show this message and exit."), html);

    s += formatHelp("--loglevel", QObject::tr(
        "Sets the log level to warn, info, debug or trace and logs messages to"
        " the console. Without --loglevel messages will not be logged to the"
        " console."), html);

#ifdef Q_OS_WIN
    s += formatHelp("--uninstall", QObject::tr(
        "Restores the old associations and cleans up the registry."), html);
#endif

    s += formatHelp("--send-actions", QObject::tr(
        "Send actions to an already running instance of %1 and return 0 on"
        " success or -1 on failure. When sending multiple actions or actions"
        " with an argument, separate them with a space and surround them with"
        " double quotes\n"
        "Examples:\n"
        "%2 --send-actions pause\n"
        "%2 --send-actions \"pause view_log true\"\n"
        "If you specify no argument for a checkable action, the action will"
        " toggle its current state.")
        .arg(TConfig::PROGRAM_NAME).arg(TConfig::PROGRAM_ID), html);

    s += formatHelp("--onload-actions", QObject::tr(
        "Sets the actions to run after loading the first file or right away"
        " when no files are specified. When setting multiple actions or actions"
        " with an argument, separate them with a space and surround them with"
        " double quotes\n"
        "Example:\n"
        "%1 --onload-actions \"pause repeat_in_out true\"")
        .arg(TConfig::PROGRAM_ID), html);

    s += formatHelp("--close-at-end", QObject::tr(
        "The main window will be closed when the playlist finishes."), html);

    s += formatHelp("--no-close-at-end", QObject::tr(
        "The main window won't be closed when the playlist finishes."), html);

    s += formatHelp("--fullscreen", QObject::tr(
        "Play the video in fullscreen."), html);

    s += formatHelp("--no-fullscreen", QObject::tr(
        "Play the video in a normal window."), html);

    s += formatHelp("--sub", QObject::tr(
        "Specifies the subtitle file to be loaded for the first video."), html);

    s += formatHelp("--media-title", QObject::tr(
        "Sets the title for the first video."), html);

    s += formatHelp("--pos", QObject::tr(
        "Specifies the coordinates where the main window will be displayed."),
        html);

    s += formatHelp("--size", QObject::tr(
        "Specifies the size of the main window."), html);

    s += formatHelp("--portable", QObject::tr(
        "Run %1 as portable application. Configuration files will be loaded and"
        " stored in the directory the program is running from.")
                    .arg(TConfig::PROGRAM_NAME), html);

    s += formatHelp("--add-to-playlist", QObject::tr(
        "If there's another instance running the media will be added"
        " to the playlist of that instance. If there's no other instance,"
        " the files will be opened in a new instance."), html);

    s += formatHelp(QObject::tr("media"), QObject::tr(
        "'Media' is any kind of file or URL that WZPlayer can open."), html);

    if (html)
        s += "</table>";

    s += "\n" + formatText(QObject::tr("When an option is specified multiple"
         " times, only the last one will be honoured."), TEXT_WIDTH);

    return s;
}
