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

#ifndef GUI_TVLIST_H
#define GUI_TVLIST_H

#include "gui/action/favorites.h"

class QWidget;

namespace Gui {
namespace Action {


class TTVList : public TFavorites {
	Q_OBJECT

public:
	enum Service { TV = 1, Radio = 2, Data = 4 };
	Q_DECLARE_FLAGS(Services, Service)

	TTVList(QWidget* parent,
			QObject* aTranslator,
			const QString& name,
			const QString& text,
			const QString& icon,
			const QString& filename,
			bool check_channels_conf,
			Services services);
	virtual ~TTVList();

#ifndef Q_OS_WIN
	static QString findChannelsFile();
#endif

#ifndef Q_OS_WIN
protected:
	void parse_channels_conf(Services services);
#endif

protected:
	virtual TFavorites* createNewObject(const QString& filename);

protected slots:
	virtual void edit();
}; // class TTVList

} // namespace Action
} // namespace Gui

Q_DECLARE_OPERATORS_FOR_FLAGS(Gui::Action::TTVList::Services)

#endif // GUI_TVLIST_H

