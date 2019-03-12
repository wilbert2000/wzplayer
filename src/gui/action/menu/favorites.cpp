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
#include "gui/playlist/playlist.h"
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

TFavorites::TFavorites(QWidget* parent,
                       TMainWindow* mw,
                       const QString& name,
                       const QString& text,
                       const QString& icon,
                       const QString& filename)
    : TMenu(parent, mw, name, text, icon)
    , _filename(filename) {

    // Warning: also used for submenus. See TMenuFile for more.
    addAct = new TAction(this, "", tr("Add current media"));
    connect(addAct, &TAction::triggered, this, &TFavorites::addCurrentPlaying);

    editAct = new TAction(this, "", tr("Edit..."));
    connect(editAct, &TAction::triggered, this, &TFavorites::edit);

    connect(this, &TFavorites::triggered, this, &TFavorites::onTriggered);

    load();
    addSeparator();
    populateMenu();
}

TFavorites::~TFavorites() {

    deleteSubFavorites();
    deleteFavList();
}

void TFavorites::deleteFavList() {

    for (int i = favList.count() - 1; i >= 0; i--) {
        delete favList.takeLast();
    }
}

void TFavorites::deleteSubFavorites() {

    for (int i = subFavorites.count() - 1; i >= 0; i--) {
        delete subFavorites.takeLast();
    }
}


void TFavorites::enableActions() {

    addAct->setEnabled(!player->mdat.filename.isEmpty());
}

void TFavorites::populateMenu() {

    deleteSubFavorites();

    for (int i = 0; i < favList.count(); i++) {
        const TFavorite* fav = favList.at(i);
        if (fav->isSubentry()) {
            if (fav->file() == _filename) {
                WZWARN("infinite recursion detected. Ignoring item.");
                break;
            }

            TFavorites* sub = new TFavorites(this, main_window, "", fav->name(),
                                             fav->icon(), fav->file());
            subFavorites.append(sub);
            addMenu(sub);
        } else {
            QAction* a = addAction(fav->name());
            a->setData(fav->file());
            a->setIcon(QIcon(fav->icon()));
            a->setStatusTip(fav->file());
        }
    }
}

void TFavorites::updateMenu() {

    // Remove all except the first 2 items
    QList<QAction*> acts = actions();
    while (acts.count() > FIRST_MENU_ENTRY) {
        QAction* a = acts.takeLast();
        removeAction(a);
        a->deleteLater();
    }

    populateMenu();
    markCurrent();
}

void TFavorites::onTriggered(QAction* action) {

    if (action->data().isValid()) {
        QString file = action->data().toString();
        if (!file.isEmpty()) {
            current_file = file;;
            markCurrent();
            main_window->getPlaylist()->open(file,
                                             action->text().replace("&", ""));
        }
    }
}

void TFavorites::markCurrent() {

    QList<QAction*> acts = actions();
    for (int n = FIRST_MENU_ENTRY; n < acts.count(); n++) {
        QAction* a = acts.at(n);
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
    if (!file.isEmpty()) {
        TFavorite* fav = new TFavorite(
                    player->mdat.displayName().replace(",", ""), file);
        favList.append(fav);
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
        foreach(const TFavorite* fav, favList) {
            stream << "#EXTINF:0,";
            stream << fav->name() << ",";
            stream << fav->icon() << ",";
            stream << fav->isSubentry() << "\n";
            stream << fav->file() << "\n";
        }
        f.close();
    } else {
        WZERROR(QString("Failed to save '%1'").arg(_filename));
    }
}

void TFavorites::load() {
    WZDEBUG("");

    QRegExp m3u_id("^#EXTM3U|^#M3U");
    QRegExp info("^#EXTINF:(.*),(.*),(.*),(.*)");

    QFile f(_filename);
    if (f.open(QIODevice::ReadOnly)) {

        QTextStream stream(&f);
        stream.setCodec("UTF-8");

        bool isSub = false;
        QString line, name, file, icon;
        while (!stream.atEnd()) {
            line = stream.readLine().trimmed();
            if (line.isEmpty()) {
                continue; // Ignore empty lines
            }
            if (m3u_id.indexIn(line)!=-1) {
                // #EXTM3U
            } else if (info.indexIn(line) != -1) {
                name = info.cap(2);
                icon = info.cap(3);
                isSub = info.cap(4).toInt() == 1;
            } else if (line.startsWith("#")) {
                // Ignore comment
            } else {
                file = line;
                if (name.isEmpty())
                    name = file;

                TFavorite* fav = new TFavorite(name, file, icon, isSub);
                favList.append(fav);

                // Clear data
                name = "";
                file = "";
                icon = "";
                isSub = false;
            }
        }
        f.close();
    }
}

void TFavorites::edit() {
    WZDEBUG("");

    TFavoriteEditor e(main_window);

    e.setData(favList);
    e.setStorePath(QFileInfo(_filename).absolutePath());

    if (e.exec() == QDialog::Accepted) {
        deleteFavList();
        favList = e.data();
        save();
        updateMenu();
    }
}

} // namespace Menu
} // namespace Action
} // namespace Gui

#include "moc_favorites.cpp"

