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

#ifndef GUI_PREF_WIDGET_H
#define GUI_PREF_WIDGET_H

#include <QWidget>
#include <QString>
#include <QPixmap>


class QEvent;

namespace Gui {
namespace Pref {

class TWidget : public QWidget {

public:
	TWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~TWidget();

	// Return the name of the section
	virtual QString sectionName();

	virtual QPixmap sectionIcon();

	// Return true if the changes made require to restart the mplayer
	// process. Should be call just after the changes have been applied.
	virtual bool requiresRestart() { return requires_restart; }

	virtual QString help() { return help_message; }

protected:
	bool requires_restart;
	int icon_size;

	virtual void retranslateStrings();
	virtual void changeEvent (QEvent* event) ;

	// Request restart if changed
	void restartIfBoolChanged(bool& old_value, bool new_value);
	void restartIfIntChanged(int& old_value, int new_value);
	void restartIfUIntChanged(unsigned int& old_value, unsigned int new_value);
	void restartIfDoubleChanged(double& old_value, const double& new_value);
	void restartIfStringChanged(QString& old_value, const QString& new_value);

	// Help
	void addSectionTitle(const QString& title);
	void addSectionGroup(const QString& title);
	void setWhatsThis(QWidget* w, const QString& title, const QString& text);
	void clearHelp();
	
private:
	QString help_message;
};

} // namespace Pref
} // namespace Gui

#endif // GUI_PREF_WIDGET_H
