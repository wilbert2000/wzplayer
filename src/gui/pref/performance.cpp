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


#include "gui/pref/performance.h"
#include "images.h"
#include "settings/preferences.h"


using namespace Settings;

namespace Gui { namespace Pref {

TPerformance::TPerformance(QWidget* parent, Qt::WindowFlags f)
    : TWidget(parent, f) {

    setupUi(this);
    retranslateStrings();
}

TPerformance::~TPerformance() {
}

QString TPerformance::sectionName() {
    return tr("Performance");
}

QPixmap TPerformance::sectionIcon() {
    //return Images::icon("pref_performance", icon_size);
    return Images::icon("pref_cache", icon_size);
}

void TPerformance::retranslateStrings() {

    retranslateUi(this);
    icon_label->setPixmap(Images::icon("pref_cache"));
    createHelp();
}

void TPerformance::setData(TPreferences* pref) {

    cache_group->setChecked(pref->cache_enabled);
    cache_file_spin->setValue(pref->cache_for_files);
    cache_stream_spin->setValue(pref->cache_for_streams);
    cache_tv_spin->setValue(pref->cache_for_tv);
    cache_bluray_spin->setValue(pref->cache_for_brs);
    cache_dvd_spin->setValue(pref->cache_for_dvds);
    cache_cd_spin->setValue(pref->cache_for_audiocds);
    cache_vcd_spin->setValue(pref->cache_for_vcds);
}

void TPerformance::getData(TPreferences* pref) {

    requires_restart = false;

    restartIfBoolChanged(pref->cache_enabled, cache_group->isChecked(),
                         "cache_enabled");
    restartIfIntChanged(pref->cache_for_files, cache_file_spin->value(),
                        "cache_file");
    restartIfIntChanged(pref->cache_for_streams, cache_stream_spin->value(),
                        "cache_stream");
    restartIfIntChanged(pref->cache_for_tv, cache_tv_spin->value(),
                        "cache_tv");
    restartIfIntChanged(pref->cache_for_brs, cache_bluray_spin->value(),
                        "cache_bluray");
    restartIfIntChanged(pref->cache_for_dvds, cache_dvd_spin->value(),
                        "cache_dvd");
    restartIfIntChanged(pref->cache_for_vcds, cache_vcd_spin->value(),
                        "cache_vcd");
    restartIfIntChanged(pref->cache_for_audiocds, cache_cd_spin->value(),
                        "cache_cd");
}

void TPerformance::createHelp() {

    clearHelp();

    addSectionTitle(tr("Cache"));

    setWhatsThis(cache_file_spin, tr("Cache for files"),
        tr("Specifies how many kilo bytes to use to cache a file."));

    setWhatsThis(cache_stream_spin, tr("Cache for streams"),
        tr("Specifie how many kilo bytes to use to cache an URL."));

    setWhatsThis(cache_bluray_spin, tr("Cache for Blu-ray"),
        tr("Specifies how many kilo bytes to use to cache a Blu-ray.<br>"
           "<b>Note:</b> Seeking might not work properly, including chapter"
           " switching, when using a cache for Blu -rays."));

    setWhatsThis(cache_dvd_spin, tr("Cache for DVDs"),
        tr("Specifies how much memory in kilo bytes to use when caching a DVD."
           "<br><b>Note:</b> Seeking might not work properly, including chapter"
           " switching, when using a cache for DVDs."));

    setWhatsThis(cache_vcd_spin, tr("Cache for VCDs"),
        tr("Specifies how much memory to use when caching a VCD in kilo bytes"));

    setWhatsThis(cache_cd_spin, tr("Cache for audio CDs"),
        tr("Specifies how much memory to use when caching an audio CD in kilo"
           " bytes."));
}

}} // namespace Gui::Pref

#include "moc_performance.cpp"
