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
	setCacheForFiles(pref->cache_for_files);
	setCacheForStreams(pref->cache_for_streams);
	setCacheForDVDs(pref->cache_for_dvds);
	setCacheForAudioCDs(pref->cache_for_audiocds);
	setCacheForVCDs(pref->cache_for_vcds);
	setCacheForTV(pref->cache_for_tv);
}

void TPerformance::getData(TPreferences* pref) {

	requires_restart = false;

	restartIfBoolChanged(pref->cache_enabled, cache_group->isChecked());
	restartIfIntChanged(pref->cache_for_files, cacheForFiles());
	restartIfIntChanged(pref->cache_for_streams, cacheForStreams());
	restartIfIntChanged(pref->cache_for_dvds, cacheForDVDs());
	restartIfIntChanged(pref->cache_for_audiocds, cacheForAudioCDs());
	restartIfIntChanged(pref->cache_for_vcds, cacheForVCDs());
	restartIfIntChanged(pref->cache_for_tv, cacheForTV());
}

void TPerformance::setCacheForFiles(int n) {
	cache_files_spin->setValue(n);
}

int TPerformance::cacheForFiles() {
	return cache_files_spin->value();
}

void TPerformance::setCacheForStreams(int n) {
	cache_streams_spin->setValue(n);
}

int TPerformance::cacheForStreams() {
	return cache_streams_spin->value();
}

void TPerformance::setCacheForDVDs(int n) {
	cache_dvds_spin->setValue(n);
}

int TPerformance::cacheForDVDs() {
	return cache_dvds_spin->value();
}

void TPerformance::setCacheForAudioCDs(int n) {
	cache_cds_spin->setValue(n);
}

int TPerformance::cacheForAudioCDs() {
	return cache_cds_spin->value();
}

void TPerformance::setCacheForVCDs(int n) {
	cache_vcds_spin->setValue(n);
}

int TPerformance::cacheForVCDs() {
	return cache_vcds_spin->value();
}

void TPerformance::setCacheForTV(int n) {
	cache_tv_spin->setValue(n);
}

int TPerformance::cacheForTV() {
	return cache_tv_spin->value();
}

void TPerformance::createHelp() {

	clearHelp();

	addSectionTitle(tr("Cache"));

	setWhatsThis(cache_files_spin, tr("Cache for files"), 
		tr("This option specifies how much memory (in kBytes) to use when "
           "precaching a file."));

	setWhatsThis(cache_streams_spin, tr("Cache for streams"), 
		tr("This option specifies how much memory (in kBytes) to use when "
           "precaching a URL."));

	setWhatsThis(cache_dvds_spin, tr("Cache for DVDs"), 
		tr("This option specifies how much memory (in kBytes) to use when "
           "precaching a DVD.<br><b>Warning:</b> Seeking might not work "
           "properly (including chapter switching) when using a cache for DVDs."));

	setWhatsThis(cache_cds_spin, tr("Cache for audio CDs"), 
		tr("This option specifies how much memory (in kBytes) to use when "
           "precaching an audio CD."));

	setWhatsThis(cache_vcds_spin, tr("Cache for VCDs"), 
		tr("This option specifies how much memory (in kBytes) to use when "
           "precaching a VCD."));
}

}} // namespace Gui::Pref

#include "moc_performance.cpp"
