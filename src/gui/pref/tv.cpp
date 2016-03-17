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

#include "gui/pref/tv.h"
#include "settings/preferences.h"
#include "images.h"
#include "settings/mediasettings.h"


namespace Gui { namespace Pref {

TTV::TTV(QWidget* parent, Qt::WindowFlags f)
	: TWidget(parent, f)
{
	setupUi(this);

#ifdef Q_OS_WIN
	rescan_check->hide();
#endif

	retranslateStrings();
}

TTV::~TTV()
{
}

QString TTV::sectionName() {
	return tr("TV and radio");
}

QPixmap TTV::sectionIcon() {
    return Images::icon("open_tv", 22);
}

void TTV::retranslateStrings() {

	retranslateUi(this);

	createHelp();
}

void TTV::setData(Settings::TPreferences* pref) {

	setRescan(pref->check_channels_conf_on_startup);
}

void TTV::getData(Settings::TPreferences* pref) {
	requires_restart = false;

	pref->check_channels_conf_on_startup = rescan();
}

void TTV::setRescan(bool b) {
	rescan_check->setChecked(b);
}

bool TTV::rescan() {
	return rescan_check->isChecked();
}

void TTV::createHelp() {
	clearHelp();


#ifndef Q_OS_WIN
	setWhatsThis(rescan_check, tr("Rescan ~/.mplayer/channels.conf on startup"),
		tr("If this option is enabled, SMPlayer will look for new TV and radio "
           "channels on ~/.mplayer/channels.conf.ter or ~/.mplayer/channels.conf."));
#endif
}

}} // namespace Gui::Pref

#include "moc_tv.cpp"
