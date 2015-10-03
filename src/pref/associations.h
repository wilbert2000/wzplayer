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


	prefassociations.h
	Handles file associations in Windows
	Author: Florin Braghis (florin@libertv.ro)
*/

#ifndef _PREF_ASSOCIATIONS_H_
#define _PREF_ASSOCIATIONS_H_

#include "ui_associations.h"
#include "pref/widget.h"

class TPreferences;

namespace Pref {

class TAssociations : public TWidget, public Ui::TAssociations
{
	Q_OBJECT

public:
	TAssociations( QWidget * parent = 0, Qt::WindowFlags f = 0 );
	virtual ~TAssociations();

	virtual QString sectionName();
	virtual QPixmap sectionIcon();

	// Pass data to the dialog
	void setData(TPreferences * pref);

	// Apply changes
	void getData(TPreferences * pref);

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

#endif // _PREF_ASSOCIATIONS_H_
