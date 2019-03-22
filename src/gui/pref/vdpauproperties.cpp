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

#include "gui/pref/vdpauproperties.h"
#include "config.h"


namespace Gui { namespace Pref {

TVDPAUProperties::TVDPAUProperties(QWidget* parent)
    : QDialog(parent, TConfig::DIALOG_FLAGS)
{
    setupUi(this);

    layout()->setSizeConstraint(QLayout::SetFixedSize);
}

void TVDPAUProperties::setffh264vdpau(bool b) {
    ffh264vdpau_check->setChecked(b);
}

void TVDPAUProperties::setffmpeg12vdpau(bool b) {
    ffmpeg12vdpau_check->setChecked(b);
}

void TVDPAUProperties::setffwmv3vdpau(bool b) {
    ffwmv3vdpau_check->setChecked(b);
}

void TVDPAUProperties::setffvc1vdpau(bool b) {
    ffvc1vdpau_check->setChecked(b);
}

void TVDPAUProperties::setffodivxvdpau(bool b) {
    ffodivxvdpau_check->setChecked(b);
}

void TVDPAUProperties::setDisableFilters(bool b) {
    disable_filters_check->setChecked(b);
}

bool TVDPAUProperties::ffh264vdpau() {
    return ffh264vdpau_check->isChecked();
}

bool TVDPAUProperties::ffmpeg12vdpau() {
    return ffmpeg12vdpau_check->isChecked();
}

bool TVDPAUProperties::ffwmv3vdpau() {
    return ffwmv3vdpau_check->isChecked();
}

bool TVDPAUProperties::ffvc1vdpau() {
    return ffvc1vdpau_check->isChecked();
}

bool TVDPAUProperties::ffodivxvdpau() {
    return ffodivxvdpau_check->isChecked();
}

bool TVDPAUProperties::disableFilters() {
    return disable_filters_check->isChecked();
}

}} // namespace Gui::Pref

#include "moc_vdpauproperties.cpp"

