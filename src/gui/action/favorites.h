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

#ifndef _GUI_FAVORITES_H_
#define _GUI_FAVORITES_H_

#include "gui/action/menu.h"
#include <QString>
#include <QList>

class QAction;
class QWidget;

namespace Gui {
namespace Action {


class TAction;

class TFavorite {
public:
	TFavorite();
	TFavorite(const QString& name,
			  const QString& file,
			  const QString& icon = QString::null,
			  bool subentry = false);
	virtual ~TFavorite();

	void setName(const QString& name) { _name = name; }
	void setFile(const QString& file) { _file = file; }
	void setIcon(QString file);
	void setSubentry(bool b) { is_subentry = b; }

	QString name() const { return _name; }
	QString file() const { return _file; }
	QString icon() const { return _icon; }
	bool isSubentry() const { return is_subentry; }

protected:
	QString _name, _file, _icon;
	bool is_subentry; // Not a favorite file, but a new favorite list
};

typedef QList<TFavorite> TFavoriteList;

class TFavorites : public TMenu {
	Q_OBJECT
public:
    TFavorites(QWidget* parent,
               const QString& name,
               const QString& text,
               const QString& icon,
               const QString& filename);
	virtual ~TFavorites();

	TAction* editAct() { return edit_act; }
	TAction* jumpAct() { return jump_act; }
	TAction* nextAct() { return next_act; }
	TAction* previousAct() { return previous_act; }
	TAction* addCurrentAct() { return add_current_act; }

public slots:
	void next();
	void previous();

	void getCurrentMedia(const QString& filename, const QString& title);

signals:
	void activated(const QString& filemane);
	//! Signal to resend the data to child
	void sendCurrentMedia(const QString& filename, const QString& title);

protected:
	virtual void save();
	virtual void load();
	virtual void updateMenu();
	virtual void populateMenu();
	virtual TFavorites* createNewObject(const QString& filename);
	void delete_children();

	int findFile(QString filename);

	// Mark current action in the menu
	void markCurrent();
	bool anyItemAvailable();

protected slots:
	void triggered_slot(QAction* action);
	virtual void edit();
	virtual void jump();
	virtual void addCurrentPlaying(); // Adds to menu current (or last played) file

protected:
	TFavoriteList f_list;
	QString _filename;
	TAction* edit_act;
	TAction* jump_act;
	TAction* next_act;
	TAction* previous_act;
	TAction* add_current_act;

	QWidget* parent_widget;

	// Current (or last) file clicked
	QString current_file;

	// Last item selected in the jump dialog
	int last_item;

	QString received_file_playing;
	QString received_title;

	QList<TFavorites*> child;
}; // class TFavorites

} // namespace Action
} // namespace Gui

#endif // _GUI_FAVORITES_H_

