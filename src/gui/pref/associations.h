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


	prefassociations.h
	Handles file associations in Windows
	Author: Florin Braghis (florin@libertv.ro)
*/

#ifndef GUI_PREF_ASSOCIATIONS_H
#define GUI_PREF_ASSOCIATIONS_H

#include "ui_associations.h"
#include "gui/pref/widget.h"
#include <QStringList>

class QWidget;
class QString;
class QPixmap;
class QListWidgetItem;

namespace Settings {
class TPreferences;
}

namespace Gui {
namespace Pref {

class TAssociations : public TWidget, public Ui::TAssociations {
	Q_OBJECT

public:
	TAssociations(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~TAssociations();

	virtual QString sectionName();
	virtual QPixmap sectionIcon();

	// Pass data to the dialog
	void setData(Settings::TPreferences* pref);

	// Apply changes
	void getData(Settings::TPreferences* pref);

	void addItem(QString label); 

	int ProcessAssociations(QStringList& current, QStringList& old);
	void refreshList(); 

protected:
	QStringList m_regExtensions; 
protected:
	virtual void createHelp();

protected:
	virtual void retranslateStrings();

public slots:
	void selectAllClicked(bool); 
	void selectNoneClicked(bool); 
	void listItemClicked(QListWidgetItem* item); 
	void listItemPressed(QListWidgetItem* item); 
	
protected:
	bool something_changed;
};

} // namespace Pref
} // namespace Gui

#endif // GUI_PREF_ASSOCIATIONS_H
