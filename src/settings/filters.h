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

/* Default options for the video and audio filters */

#ifndef SETTINGS_FILTERS_H
#define SETTINGS_FILTERS_H

#include <QObject>
#include <QString>
#include <QMap>

class QSettings;

namespace Settings {

class TFilter {
public:
    TFilter() {}
    TFilter(QString tr_name, QString name, QString options = QString::null) {
        _tr_name = tr_name; _name = name; _options = options;
    }
    virtual ~TFilter() {}

    void setTrName(QString tr_name) { _tr_name = tr_name; }
    void setName(QString name) { _name = name; }
    void setOptions(QString options) { _options = options; }

    QString trName() const { return _tr_name; }
    QString name() const { return _name; }
    QString options() const { return _options; }

    QString filter() {
        QString s = name();
        if (!options().isEmpty()) s += "=" + options();
        return s;
    }

protected:
    QString _tr_name, _name, _options;
};

typedef QMap<QString,TFilter> TFilterMap;

class TFilters : public QObject {
    Q_OBJECT
public:
    TFilters();

    void init();

    TFilter item(const QString& key);

    void setTFilters(TFilterMap filters) { list = filters; }
    TFilterMap filters() { return list; }

    void save(QSettings* set);
    void load(QSettings* set);

protected:
    TFilterMap list;
};

} // namespace Settings

#endif // SETTINGS_FILTERS_H

