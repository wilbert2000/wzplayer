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

#include "gui/about.h"
#include <QFile>
#include <QDesktopServices>

#include "config.h"
#include "settings/paths.h"
#include "settings/preferences.h"
#include "images.h"
#include "version.h"

using namespace Settings;

namespace Gui {

TAbout::TAbout(QWidget* parent)
    : QDialog(parent, TConfig::DIALOG_FLAGS) {

    setupUi(this);

    setWindowIcon(Images::icon("logo", 64));
    logo->setPixmap(QPixmap(":/default-theme/logo.png")
                    .scaledToHeight(64, Qt::SmoothTransformation));
    contrib_icon->setPixmap(Images::icon("contributors"));
    translators_icon->setPixmap(Images::icon("translators"));
    license_icon->setPixmap(Images::icon("license"));

    info->setText(
        "<b>" + TConfig::PROGRAM_NAME
                +"</b> &copy; 2015-2019 Wilbert Hengst.<br><br>"

        + tr("WZPlayer is a graphical user interface for %1 and %2"
             " based on %3 by Ricardo Villalba.")
            .arg(link(TConfig::URL_MPLAYER, "MPlayer"))
            .arg(link(TConfig::URL_MPV, "MPV"))
            .arg(link(TConfig::URL_SMPLAYER, "SMPlayer")) + "<br><br>"

          "<b>" + tr("Version:") + "</b> " + TVersion::version + "<br>"
        + tr("Using Qt %1 (compiled with Qt %2)")
                .arg(qVersion()).arg(QT_VERSION_STR) + "<br><br>"

          "<b>" + tr("Home:") + "</b> " + link(TConfig::URL_HOMEPAGE));

    contributions->setText(
        tr("WZPlayer logo by %1")
            .arg("Charles Barcza &lt;kbarcza@blackpanther.hu&gt;")
        + "<br><br>"
        + tr("Packages for Windows created by %1")
            .arg("redxii &lt;redxii@users.sourceforge.net&gt;")
        + "<br><br>"
        + tr("Many other people contributed with patches. See the Readme.txt"
             " file for details."));

    translators->setHtml(
        tr("Many people contributed with translations.") + " " +
        tr("You can also help to translate WZPlayer into your own language.")

        + "<p>" + tr("Visit %1 and join a translation team.")
        .arg(link("http://www.transifex.com/projects/p/wzplayer/")) + "</p>"

        + "<p>" + link(TConfig::URL_TRANSLATORS, tr("Click here"))
        + tr(" for the translators from the transifex teams"));

    license->setText("<i>This program is free software; you can redistribute it"
        " and/or modify it under the terms of the GNU General Public License as"
        " published by the Free Software Foundation; either version 2 of the"
        " license or any later version.</i>");

    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &TAbout::accept);


    // TODO: fix palette
    /*
    QPalette pal = info->palette();
    pal.setColor(QPalette::Base, info_tab->palette().color(QPalette::Window));
    pal.setBrush(QPalette::Base, info_tab->palette().window());

    info->setPalette(pal);
    contributions->setPalette(pal);
    translators->setPalette(pal);
    license->setPalette(pal);
    */

    adjustSize();
}

QString TAbout::link(const QString& url, QString name) const {

    if (name.isEmpty())
        name = url;
    return QString("<a href=\"" + url + "\">" + name +"</a>");
}

QSize TAbout::sizeHint () const {
    return QSize(528, 336);
}

} // namespace

#include "moc_about.cpp"
