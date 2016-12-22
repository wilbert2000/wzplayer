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

#include "gui/pref/network.h"
#include "settings/preferences.h"
#include "images.h"
#include <QNetworkProxy>


namespace Gui { namespace Pref {

TNetwork::TNetwork(QWidget* parent, Qt::WindowFlags f)
    : TWidget(parent, f) {

    setupUi(this);

    proxy_type_combo->addItem(tr("HTTP"), QNetworkProxy::HttpProxy);
    proxy_type_combo->addItem(tr("SOCKS5"), QNetworkProxy::Socks5Proxy);

    retranslateStrings();
}

TNetwork::~TNetwork() {
}

QString TNetwork::sectionName() {
    return tr("Network");
}

QPixmap TNetwork::sectionIcon() {
    return Images::icon("pref_network", icon_size);
}

void TNetwork::retranslateStrings() {

    retranslateUi(this);
    icon_label->setPixmap(Images::icon("pref_network"));
    createHelp();
}

void TNetwork::setData(Settings::TPreferences* pref) {

    if (pref->ipPrefer == Settings::TPreferences::IP_PREFER_4) {
        ip_prefer_4_radiobutton->setChecked(true);
    } else if (pref->ipPrefer == Settings::TPreferences::IP_PREFER_6) {
        ip_prefer_6_radiobutton->setChecked(true);
    } else {
        ip_prefer_auto_radiobutton->setChecked(true);
    }

    proxy_group->setChecked(pref->use_proxy);
    proxy_hostname_edit->setText(pref->proxy_host);
    proxy_port_spin->setValue(pref->proxy_port);
    proxy_username_edit->setText(pref->proxy_username);
    proxy_password_edit->setText(pref->proxy_password);

    setProxyType(pref->proxy_type);

    update_group->setChecked(pref->update_checker_data.enabled);
    days_spin->setValue(pref->update_checker_data.days_to_check);
}

void TNetwork::getData(Settings::TPreferences* pref) {

    requires_restart = false;

    if (ip_prefer_4_radiobutton->isChecked()) {
        pref->ipPrefer = Settings::TPreferences::IP_PREFER_4;
    } else if (ip_prefer_6_radiobutton->isChecked()) {
        pref->ipPrefer = Settings::TPreferences::IP_PREFER_6;
    } else {
        pref->ipPrefer = Settings::TPreferences::IP_PREFER_AUTO;
    }

    pref->use_proxy = proxy_group->isChecked();
    pref->proxy_host = proxy_hostname_edit->text();
    pref->proxy_port = proxy_port_spin->value();
    pref->proxy_username = proxy_username_edit->text();
    pref->proxy_password = proxy_password_edit->text();

    pref->proxy_type = proxyType();

    pref->update_checker_data.enabled = update_group->isChecked();
    pref->update_checker_data.days_to_check = days_spin->value();
}

void TNetwork::setProxyType(int type) {

    int index = proxy_type_combo->findData(type);
    if (index == -1) index = 0;
    proxy_type_combo->setCurrentIndex(index);
}

int TNetwork::proxyType() {

    int index = proxy_type_combo->currentIndex();
    return proxy_type_combo->itemData(index).toInt();
}

void TNetwork::createHelp() {

    clearHelp();

    addSectionTitle(tr("Proxy"));

    setWhatsThis(proxy_group, tr("Enable proxy"),
        tr("Enable/disable the use of the proxy."));

    setWhatsThis(proxy_hostname_edit, tr("Host"),
        tr("The host name of the proxy."));

    setWhatsThis(proxy_port_spin, tr("Port"),
        tr("The port of the proxy."));

    setWhatsThis(proxy_username_edit, tr("Username"),
        tr("If the proxy requires authentication, this sets the username."));

    setWhatsThis(proxy_password_edit, tr("Password"),
        tr("The password for the proxy. <b>Warning:</b> the password will be saved "
           "as plain text in the configuration file."));

    setWhatsThis(proxy_type_combo, tr("Type"),
        tr("Select the proxy type to be used."));

    addSectionTitle(tr("Proxy"));

    setWhatsThis(update_group, tr("Check for updates"),
        tr("If this option is enabled, WZPlayer will check for updates "
           "and display a notification if a new version is available."));

    setWhatsThis(days_spin, tr("Check interval"),
        tr("Enter the number of days between update checks."));
}

}} // namespace Gui::Pref

#include "moc_network.cpp"
