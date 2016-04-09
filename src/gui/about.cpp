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
#include <QDebug>
#include <QFile>
#include <QDesktopServices>

#include "config.h"
#include "settings/paths.h"
#include "settings/preferences.h"
#include "images.h"
#include "version.h"

using namespace Settings;

namespace Gui {

TAbout::TAbout(QWidget* parent, Qt::WindowFlags f)
	: QDialog(parent, f) {

	setupUi(this);
	setWindowIcon(Images::icon("logo", 64));

	logo->setPixmap(QPixmap(":/default-theme/logo.png").scaledToHeight(64, Qt::SmoothTransformation));
	contrib_icon->setPixmap(Images::icon("contributors"));
	translators_icon->setPixmap(Images::icon("translators"));
	license_icon->setPixmap(Images::icon("license"));

	info->setText(
		"<b>" + TConfig::PROGRAM_NAME +"</b> &copy; 2015-2016 Wilbert Hengst.<br>"
		+ tr("Based on %1 by Ricardo Villalba.").arg("<a href=\"" + TConfig::URL_SMPLAYER + "\">SMPlayer</a>")
		+ "<br><br>"
		"<b>" + tr("Version: %1").arg(TVersion::version) + "</b>" +
#if PORTABLE_APP
				" (" + tr("Portable Edition") + ")" +
#endif
        "<br>" +
		tr("Using Qt %1 (compiled with Qt %2)").arg(qVersion()).arg(QT_VERSION_STR)
		+ "<br><br>"
		+ "<b>"+ tr("Links:") +"</b><br>"
		+ tr("Website:") + " " + link(TConfig::URL_HOMEPAGE) + "<br>"
		+ tr("Issues:") +" "+  link(TConfig::URL_ISSUES) + "<br>"
		+ "<br>"
		+ tr("WZPlayer is a graphical interface for %1 and %2.")
			.arg("<a href=\"http://www.mplayerhq.hu/design7/info.html\">MPlayer</a>")
			.arg("<a href=\"http://www.mpv.io\">MPV</a>")
		+ "<br><br>"
		+ tr("Subtitles service powered by %1").arg("<a href=\"http://www.opensubtitles.org\">www.OpenSubtitles.org</a>"));

	QString license_text =
		"<i>"
		"This program is free software; you can redistribute it and/or modify "
		"it under the terms of the GNU General Public License as published by "
		"the Free Software Foundation; either version 2 of the License, or "
		"(at your option) any later version."  "</i><br><br>";
		
	QString license_file = TPaths::doc("gpl.html", "en");
	if (QFile::exists(license_file)) {
		license_file = QUrl::fromLocalFile(license_file).toString();
		license_text += QString("<a href=\"%1\">%2</a>").arg(license_file).arg(tr("Read the entire license"));
	}

	if ((pref->language != "en") && (pref->language != "en_US")) {
		QString license_trans_file = TPaths::doc("gpl.html", pref->language, false);
		if (QFile::exists(license_trans_file)) {
			license_trans_file = QUrl::fromLocalFile(license_trans_file).toString();
			license_text += QString("<br><a href=\"%1\">%2</a>").arg(license_trans_file).arg(tr("Read a translation"));
		}
	}
	license->setText(license_text);
	license->setOpenLinks(false);
	license->setOpenExternalLinks(false);
	connect(license, SIGNAL(anchorClicked(const QUrl &)), this, SLOT(openLink(const QUrl&)));

	translators->setHtml(getTranslators());

	contributions->setText(
		tr("WZPlayer logo by %1").arg("Charles Barcza &lt;kbarcza@blackpanther.hu&gt;") + "<br><br>"
		+ tr("Packages for Windows created by %1").arg("redxii &lt;redxii@users.sourceforge.net&gt;") + "<br><br>"
		+ tr("Some icons by <a href=\"http://tatice.deviantart.com\">Tatice</a>"
		   " under the <a href=\"http://creativecommons.org/licenses/by-nc-nd/4.0/\">"
		   "CC Attribution-Noncommercial-No Derivate 4.0</a> license.") + "<br><br>"
		+ tr("Many other people contributed with patches. See the Changelog for details."));

	// Copy the background color ("window") of the tab widget to the "base" color of the qtextbrowsers
	// Problem, it doesn't work with some styles, so first we change the "window" color of the tab widgets.
	info_tab->setAutoFillBackground(true);
	contributions_tab->setAutoFillBackground(true);
	translations_tab->setAutoFillBackground(true);
	license_tab->setAutoFillBackground(true);

	QPalette pal = info_tab->palette();
	pal.setColor(QPalette::Window, palette().color(QPalette::Window));

	info_tab->setPalette(pal);
	contributions_tab->setPalette(pal);
	translations_tab->setPalette(pal);
	license_tab->setPalette(pal);

	QPalette p = info->palette();
	//p.setBrush(QPalette::Base, info_tab->palette().window());
	p.setColor(QPalette::Base, info_tab->palette().color(QPalette::Window));

	info->setPalette(p);
	contributions->setPalette(p);
	translators->setPalette(p);
	license->setPalette(p);

	tab_widget->removeTab(0);

	adjustSize();
}

TAbout::~TAbout() {
}

QString TAbout::getTranslators() {
	return QString(
		 tr("Many people contributed with translations.") +" "+
		 tr("You can also help to translate WZPlayer into your own language.") +"<p>"+
		 tr("Visit %1 and join a translation team.").arg("<a href=\"http://www.transifex.com/projects/p/wzplayer/\">http://www.transifex.com/projects/p/wzplayer/</a>") +
		"<p><a href=\"" + TConfig::URL_TRANSLATORS + "\">" +
		 tr("Click here to know the translators from the transifex teams") +
		"</a>");
}

QString TAbout::trad(const QString & lang, const QString & author) {
	return trad(lang, QStringList() << author);
}

QString TAbout::trad(const QString & lang, const QStringList & authors) {
	QString s;
	for (int n = 0; n < authors.count(); n++) {
		QString author = authors[n];
		s += author.replace("<", "&lt;").replace(">", "&gt;");
		if (n < (authors.count()-1)) s += ", ";
	}
	//return QString("<h3>%1:</h3><h4>%2</h4><hr>").arg(lang).arg(s);
	return QString("<p><b>%1</b>: %2</p>").arg(lang).arg(s);
}

QString TAbout::link(const QString & url, QString name) {
	if (name.isEmpty()) name = url;
	return QString("<a href=\"" + url + "\">" + name +"</a>");
}

QString TAbout::contr(const QString & author, const QString & thing) {
	return "<li>"+ tr("<b>%1</b> (%2)").arg(author).arg(thing) +"</li>";
}

QSize TAbout::sizeHint () const {
	return QSize(518, 326);
}

void TAbout::openLink(const QUrl & link) {
	qDebug() << "Gui::TAbout::openLink:" << link.toString();
	QDesktopServices::openUrl(link);
}

} // namespace

#include "moc_about.cpp"
