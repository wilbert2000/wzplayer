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
#include <QObject>
#include <QApplication>
#include <QFileInfo>

QString CLHelp::formatText(QString s, int col) {
    QString res = "";

    int last = 0;
    int pos;

    pos = s.indexOf(" ");
    while (pos != -1) {

        if (s.count() < col) {
            res = res + s;
            s = "";
            break;
        }

        while ((pos < col) && (pos != -1)) {
            last = pos;
            pos = s.indexOf(" ", pos+1);
        }

        res = res + s.left(last) + "\n";
        s = s.mid(last+1);

        last = 0;
        pos = s.indexOf(" ");

    }

    if (!s.isEmpty()) res = res + s;

    return res;
}

QString CLHelp::formatHelp(QString parameter, QString help, bool html) {
    if (html) {
        return "<tr><td><b>"+parameter+"</b></td><td>"+help+"</td></tr>";
    } else {
        int par_width = 20;
        int help_width = 80 - par_width;

        QString s;
        s = s.fill(' ', par_width - (parameter.count()+2));
        s = s + parameter + ": ";

        QString f;
        f = f.fill(' ', par_width);

        QString s2 = formatText(help, help_width);
        int pos = s2.indexOf('\n');
        while (pos != -1) {
            s2 = s2.insert(pos+1, f);
            pos = s2.indexOf('\n', pos+1);
        }

        return s + s2 + "\n";
    }
}


QString CLHelp::help(bool html) {

    QString app_name = QFileInfo(qApp->applicationFilePath()).baseName();

    QString options = QString("%1 [--debug] [--trace]"
                              " [--send-action %2] [--actions %3]"
                              " [--close-at-end] [--no-close-at-end]"
                              " [--fullscreen] [--no-fullscreen]"
                              " [--ontop] [--no-ontop]"
                              " [--sub %4] [--pos x y] [--size %5 %6]"
                              " [--add-to-playlist] [--help]"
                              " [%7] [%7]...")
                      .arg(app_name)
                      .arg(QObject::tr("action_name"))
                      .arg(QObject::tr("action_list"))
                      .arg(QObject::tr("subtitle_file"))
                      .arg(QObject::tr("width")).arg(QObject::tr("height"))
                      .arg(QObject::tr("media"));

    QString s;

    if (html) {
        s = QObject::tr("Usage:") + " <b>" + options + "</b><br>";
        s += "<table>";
    } else {
        s = formatText(QObject::tr("Usage:") + " " + options, 80);
        s += "\n\n";
    }

    s += formatHelp("--debug", QObject::tr(
        "Logs debug message to the console."), html);

    s += formatHelp("--trace", QObject::tr(
        "Logs trace message to the console."), html);


#ifdef Q_OS_WIN
    s += formatHelp("--uninstall", QObject::tr(
        "Restores the old associations and cleans up the registry."), html);
#endif

    s += formatHelp("--send-action", QObject::tr(
        "Send actions to an already running instance of WZPlayer."
        " Example: --send-action pause"
        " The application will exit and return 0 on success or -1 on failure."),
        html);

    s += formatHelp("--actions", QObject::tr(
        "action_list is a list of actions separated by spaces. "
        "The actions will be executed just after loading the file (if any) "
        "in the same order you entered. For checkable actions you can pass "
        "true or false as parameter. Example: "
        "--actions \"fullscreen repeat_in_out true\". Quotes are necessary in "
        "case you pass more than one action."), html);

    s += formatHelp("--close-at-end", QObject::tr(
        "The main window will be closed when the playlist finishes."), html);

    s += formatHelp("--no-close-at-end", QObject::tr(
        "The main window won't be closed when the playlist finishes."), html);

    s += formatHelp("--fullscreen", QObject::tr(
        "Play the video in fullscreen mode."), html);

    s += formatHelp("--no-fullscreen", QObject::tr(
        "Play the video in window mode."), html);

    s += formatHelp("--sub", QObject::tr(
        "Specifies the subtitle file to be loaded for the first video."), html);

    s += formatHelp("--media-title", QObject::tr(
        "Sets the title for the first video."), html);

    s += formatHelp("--pos", QObject::tr(
        "Specifies the coordinates where the main window will be displayed."),
                    html);

    s += formatHelp("--size", QObject::tr(
        "Specifies the size of the main window."), html);

    s += formatHelp("--help", QObject::tr(
        "Show this message and exit."), html);

    s += formatHelp("--add-to-playlist", QObject::tr(
        "If there's another instance running, the media will be added "
        "to that instance's playlist. If there's no other instance, "
        "the files will be opened in a new instance."), html);

    s += formatHelp(QObject::tr("media"), QObject::tr(
        "'Media' is any kind of file that WZPlayer can open. It can "
        "be a local file, a DVD (e.g. dvd:// or dvdnav://), an Internet stream "
        "(e.g. mms://....) or a local playlist in format m3u or m3u8."), html);

    if (html)
        s += "</table>";

    return s;
}
