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

#include "gui/mainwindow.h"
#include "gui/action/menu/favorites.h"
#include "gui/action/menu/favoriteeditor.h"
#include "gui/action/action.h"
#include "player/player.h"

#include <QAction>
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QInputDialog>
#include <QFileInfo>

#define FIRST_MENU_ENTRY 3

namespace Gui {
namespace Action {
namespace Menu {

TFavorites::TFavorites(TMainWindow* mw,
                       const QString& name,
                       const QString& text,
                       const QString& icon,
                       const QString& filename)
    : TMenu(mw, mw, name, text, icon)
    , _filename(filename)
    , last_item(1) {

    editAct = new TAction(this, "", tr("&Edit..."), "noicon");
    connect(editAct, SIGNAL(triggered()), this, SLOT(edit()));

    addAct = new TAction(this, "", tr("&Add current media"), "noicon");
    connect(addAct, SIGNAL(triggered()), this, SLOT(addCurrentPlaying()));

    jumpAct = new TAction(this, "", tr("&Jump..."), "noicon", 0, false);
    connect(jumpAct, SIGNAL(triggered()), this, SLOT(jump()));

    connect(this, SIGNAL(triggered(QAction *)),
            this, SLOT(onTriggered(QAction *)));

    load();
    addSeparator();
    populateMenu();
}

TFavorites::~TFavorites() {
    save();
}

void TFavorites::enableActions() {

    addAct->setEnabled(player && !player->mdat.filename.isEmpty());
    jumpAct->setEnabled(f_list.count() > 0);
}

void TFavorites::delete_children() {

    for (int n = 0; n < child.count(); n++) {
        if (child[n]) {
            delete child[n];
            child[n] = 0;
        }
    }
    child.clear();
}

void TFavorites::populateMenu() {

    for (int n = 0; n < f_list.count(); n++) {
        const TFavorite& fav = f_list.at(n);
        QString i = QString::number(n + 1);
        QString name = QString("%1 - " + fav.name()).arg(
                               i.insert(i.size() - 1, '&'), 3, ' ');
        if (fav.isSubentry()) {

            if (fav.file() == _filename) {
                WZWARN("infinite recursion detected. Ignoring item.");
                break;
            }

            TFavorites* sub = new TFavorites(main_window, "", "", "noicon",
                                             fav.file());
            child.push_back(sub);

            QAction* a = addMenu(sub);
            a->setText(name);
            a->setIcon(QIcon(fav.icon()));
        } else {
            QAction* a = addAction(name);
            a->setData(fav.file());
            a->setIcon(QIcon(fav.icon()));
            a->setStatusTip(fav.file());
        }
    }
}

void TFavorites::updateMenu() {

    // Remove all except the first 2 items
    while (actions().count() > FIRST_MENU_ENTRY) {
        QAction* a = actions()[FIRST_MENU_ENTRY];
        removeAction(a);
        a->deleteLater();
    }

    delete_children();

    populateMenu();
    markCurrent();
}

void TFavorites::onTriggered(QAction* action) {

    if (action->data().isValid()) {
        QString file = action->data().toString();
        current_file = file;;
        markCurrent();
        main_window->open(file);
    }
}

void TFavorites::markCurrent() {

    for (int n = FIRST_MENU_ENTRY; n < actions().count(); n++) {
        QAction* a = actions()[n];
        QString file = a->data().toString();
        QFont f = a->font();

        if ((!file.isEmpty()) && (file == current_file)) {
            f.setBold(true);
            a->setFont(f);
        } else {
            f.setBold(false);
            a->setFont(f);
        }
    }
}

void TFavorites::addCurrentPlaying() {

    QString file = player->mdat.filename;
    if (file.isEmpty()) {
        WZWARN("No filename available to add to favorites");
    } else {
        TFavorite fav;
        fav.setName(player->mdat.displayName().replace(",", ""));
        fav.setFile(file);
        f_list.append(fav);
        save();
        updateMenu();
    }
}

void TFavorites::save() {
    WZDEBUG(_filename);

    QFile f(_filename);
    if (f.open(QIODevice::WriteOnly)) {
        QTextStream stream(&f);
        stream.setCodec("UTF-8");

        stream << "#EXTM3U" << "\n";
        foreach(const TFavorite& fav, f_list) {
            stream << "#EXTINF:0,";
            stream << fav.name() << ",";
            stream << fav.icon() << ",";
            stream << fav.isSubentry() << "\n";
            stream << fav.file() << "\n";
        }
        f.close();
    } else {
        WZERROR(QString("failed to save '%1'").arg(_filename));
    }
}

void TFavorites::load() {
    WZDEBUG("");

    QRegExp m3u_id("^#EXTM3U|^#M3U");
    QRegExp info1("^#EXTINF:(.*),(.*),(.*)");
    QRegExp info2("^#EXTINF:(.*),(.*),(.*),(.*)");

    QFile f(_filename);
    if (f.open(QIODevice::ReadOnly)) {

        f_list.clear();

        TFavorite fav;

        QTextStream stream(&f);
        stream.setCodec("UTF-8");

        QString line;
        while (!stream.atEnd()) {
            line = stream.readLine().trimmed();
            if (line.isEmpty()) {
                continue; // Ignore empty lines
            }
            if (m3u_id.indexIn(line)!=-1) {
                //#EXTM3U
                // Ignore line
            } else if (info2.indexIn(line) != -1) {
                fav.setName(info2.cap(2));
                fav.setIcon(info2.cap(3));
                fav.setSubentry(info2.cap(4).toInt() == 1);
            } else if (info1.indexIn(line) != -1) {
                // Compatibility with old files
                fav.setName(info1.cap(2));
                fav.setIcon(info1.cap(3));
                fav.setSubentry(false);
            } else if (line.startsWith("#")) {
                // Ignore comment
            } else {
                fav.setFile(line);
                if (fav.name().isEmpty())
                    fav.setName(line);
                f_list.append(fav);

                // Clear data
                fav.setName("");
                fav.setFile("");
                fav.setIcon("");
                fav.setSubentry(false);
            }
        }
        f.close();
    }
}

void TFavorites::edit() {
    WZDEBUG("");

    TFavoriteEditor e(main_window);

    e.setData(f_list);
    e.setStorePath(QFileInfo(_filename).absolutePath());

    if (e.exec() == QDialog::Accepted) {
        f_list = e.data();
        save();
        updateMenu();
    }
}

void TFavorites::jump() {

    bool ok;
    int item = QInputDialog::getInt(main_window, tr("Jump to item"),
        tr("Enter the number of the item in the list to jump to:"),
        last_item, 1, f_list.count(), 1, &ok);
    if (ok) {
        last_item = item;
        item--;
        actions()[item + FIRST_MENU_ENTRY]->trigger();
    }
}

} // namespace Menu
} // namespace Action
} // namespace Gui

#include "moc_favorites.cpp"

