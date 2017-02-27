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

#include "gui/action/menu/favorites.h"
#include <QAction>
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QInputDialog>
#include <QFileInfo>
#include "gui/action/action.h"
#include "gui/action/menu/favoriteeditor.h"
#include "gui/mainwindow.h"

#define FIRST_MENU_ENTRY 3

namespace Gui {
namespace Action {
namespace Menu {

TFavorite::TFavorite() : is_subentry(false) {
}

TFavorite::TFavorite(const QString& name,
                     const QString& file,
                     const QString& icon,
                     bool subentry)
    : _name(name)
    , _file(file)
    , _icon(icon)
    , is_subentry(subentry) {
}

TFavorite::~TFavorite() {
}

void TFavorite::setIcon(QString file) {
    _icon = file;
}


TFavorites::TFavorites(TMainWindow* mw,
                       const QString& name,
                       const QString& text,
                       const QString& icon,
                       const QString& filename)
    : TMenu(mw, mw, name, text, icon)
    , _filename(filename)
    , last_item(1) {

    edit_act = new TAction(this, "", tr("&Edit..."), "noicon");
    connect(edit_act, SIGNAL(triggered()), this, SLOT(edit()));

    jump_act = new TAction(this, "", tr("&Jump..."), "noicon", 0, false);
    connect(jump_act, SIGNAL(triggered()), this, SLOT(jump()));

    next_act = new TAction(this, "", tr("&Next"), "noicon", 0, false);
    connect(next_act, SIGNAL(triggered()), this, SLOT(next()));

    previous_act = new TAction(this, "", tr("&Previous"), "noicon", 0, false);
    connect(previous_act, SIGNAL(triggered()), this, SLOT(previous()));

    add_current_act = new TAction(this, "", tr("&Add current media"), "noicon");
    add_current_act->setEnabled(false);
    connect(add_current_act, SIGNAL(triggered()), this, SLOT(addCurrentPlaying()));

    connect(this, SIGNAL(triggered(QAction *)),
            this, SLOT(triggered_slot(QAction *)));

    load();

    addSeparator();
    populateMenu();
}

TFavorites::~TFavorites() {
    save();
}

void TFavorites::delete_children() {

    for (int n = 0; n < child.count(); n++) {
        if (child[n])
            delete child[n];
        child[n] = 0;
    }
    child.clear();
}

TFavorites* TFavorites::createNewObject(const QString& filename) {
    return new TFavorites(main_window, "", "", "noicon", filename);
}

void TFavorites::populateMenu() {

    for (int n = 0; n < f_list.count(); n++) {
        QString i = QString::number(n+1);
        QString name = QString("%1 - " + f_list[n].name()).arg(i.insert(i.size()-1, '&'), 3, ' ');
        if (f_list[n].isSubentry()) {

            if (f_list[n].file() == _filename) {
                WZWARN("infinite recursion detected. Ignoring item.");
                break;
            }

            TFavorites* new_fav = createNewObject(f_list[n].file());
            new_fav->getCurrentMedia(received_file_playing, received_title);
            connect(this, SIGNAL(sendCurrentMedia(const QString&, const QString&)),
                    new_fav, SLOT(getCurrentMedia(const QString&, const QString&)));
            child.push_back(new_fav);

            QAction* a = addMenu(new_fav);
            a->setText(name);
            a->setIcon(QIcon(f_list[n].icon()));
        } else {
            QAction* a = addAction(name);
            a->setData(f_list[n].file());
            a->setIcon(QIcon(f_list[n].icon()));
            a->setStatusTip(f_list[n].file());
        }
    }

    // Enable actions
    jump_act->setEnabled(f_list.count() > 0);
    bool e = anyItemAvailable();
    next_act->setEnabled(e);
    previous_act->setEnabled(e);
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

void TFavorites::triggered_slot(QAction* action) {

    if (action->data().isValid()) {
        QString file = action->data().toString();
        emit activated(file);
        current_file = file;;
        markCurrent();
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

int TFavorites::findFile(const QString& filename) const {

    for (int n = 0; n < f_list.count(); n++) {
        if (f_list[n].file() == filename) {
            return n;
        }
    }
    return -1;
}

bool TFavorites::anyItemAvailable() {

    if (f_list.isEmpty())
        return false;

    bool item_available = false;
    for (int n = 0; n < f_list.count(); n++) {
        if (!f_list[n].isSubentry()) {
            item_available = true;
            break;
        }
    }

    return item_available;
}

void TFavorites::next() {

    if (!anyItemAvailable())
        return;

    int current = findFile(current_file);

    int i = current;
    if (current < 0)
        current = 0;

    do {
        i++;
        if (i == current)
            break;
        if (i >= f_list.count())
            i = 0;
    } while (f_list[i].isSubentry());

    QAction* a = actions()[i + FIRST_MENU_ENTRY]; // Skip "edit" and separator
    if (a) {
        a->trigger();
    }
}

void TFavorites::previous() {

    if (!anyItemAvailable())
        return;

    int current = findFile(current_file);

    int i = current;
    if (current < 0)
        current = 0;

    do {
        i--;
        if (i == current)
            break;
        if (i < 0)
            i = f_list.count() - 1;
    } while (f_list[i].isSubentry());

    QAction* a = actions()[i + FIRST_MENU_ENTRY]; // Skip "edit" and separator
    if (a) {
        a->trigger();
    }
}

void TFavorites::getCurrentMedia(const QString& filename, const QString& title) {

    if (!filename.isEmpty()) {
        received_file_playing = filename;
        received_title = title;

        emit sendCurrentMedia(filename, title);

        add_current_act->setEnabled(true);
    }
}

void TFavorites::addCurrentPlaying() {

    if (received_file_playing.isEmpty()) {
        WZWARN("received file is empty, doing nothing");
    } else {
        TFavorite fav;
        fav.setName(received_title.replace(",", ""));
        fav.setFile(received_file_playing);
        f_list.append(fav);
        save();
        updateMenu();
    }
}

void TFavorites::save() {
    WZDEBUG("");

    QFile f(_filename);
    if (f.open(QIODevice::WriteOnly)) {
        QTextStream stream(&f);
        stream.setCodec("UTF-8");

        stream << "#EXTM3U" << "\n";
        for (int n = 0; n < f_list.count(); n++) {
            stream << "#EXTINF:0,";
            stream << f_list[n].name() << ",";
            stream << f_list[n].icon() << ",";
            stream << f_list[n].isSubentry() << "\n";
            stream << f_list[n].file() << "\n";
        }
        f.close();
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
            if (line.isEmpty()) continue; // Ignore empty lines
            if (m3u_id.indexIn(line)!=-1) {
                //#EXTM3U
                // Ignore line
            }
            else
            if (info2.indexIn(line) != -1) {
                fav.setName(info2.cap(2));
                fav.setIcon(info2.cap(3));
                fav.setSubentry(info2.cap(4).toInt() == 1);
            }
            else
            // Compatibility with old files
            if (info1.indexIn(line) != -1) {
                fav.setName(info1.cap(2));
                fav.setIcon(info1.cap(3));
                fav.setSubentry(false);
            }
            else
            if (line.startsWith("#")) {
                // Comment
                // Ignore
            } else {
                fav.setFile(line);
                if (fav.name().isEmpty()) fav.setName(line);
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
        tr("Enter the number of the item in the list to jump:"),
        last_item, 1, f_list.count(), 1, &ok);
    if (ok) {
        last_item = item;
        item--;
        actions()[item + FIRST_MENU_ENTRY]->trigger();
    }
} // class TFavorites::jump()

} // namespace Menu
} // namespace Action
} // namespace Gui

#include "moc_favorites.cpp"

