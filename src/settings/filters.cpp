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

#include "settings/filters.h"
#include <QSettings>

namespace Settings {

TFilters::TFilters() : QObject() {
    init();
}

TFilters::~TFilters() {
}

void TFilters::init() {
    list.clear();

    // Video
    list["noise"] = TFilter("add noise", "noise", "9ah:5ah");
    list["deblock"] = TFilter("deblock", "pp", "vb/hb");
    list["gradfun"] = TFilter("gradfun", "gradfun");
    list["denoise_normal"] = TFilter("normal denoise", "hqdn3d");
    list["denoise_soft"] = TFilter("soft denoise", "hqdn3d", "2:1:2");
    list["blur"] = TFilter("blur", "unsharp", "lc:-1.5");
    list["sharpen"] = TFilter("sharpen", "unsharp", "lc:1.5");

    // Audio
    list["volnorm"] = TFilter("volume normalization", "volnorm", "1");
}

TFilter TFilters::item(const QString & key) {
    return list[key];
}

void TFilters::save(QSettings *set) {
    set->beginGroup("filter_options");

    QMap<QString, TFilter>::iterator i;
    for (i = list.begin(); i != list.end(); ++i) {
        set->setValue(i.key(), i.value().options());
    }

    set->endGroup();
}

void TFilters::load(QSettings *set) {
    set->beginGroup("filter_options");

    QMap<QString, TFilter>::iterator i;
    for (i = list.begin(); i != list.end(); ++i) {
        i.value().setOptions(set->value(i.key(), i.value().options()).toString());
    }

    set->endGroup();
}

} // namespace Settings

#include "moc_filters.cpp"
