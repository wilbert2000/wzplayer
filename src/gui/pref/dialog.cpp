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


#include "gui/pref/dialog.h"
#include <QDebug>
#include <QTextBrowser>
#include "images.h"
#include "settings/preferences.h"
#include "gui/pref/widget.h"
#include "gui/pref/general.h"
#include "gui/pref/video.h"
#include "gui/pref/audio.h"
#include "gui/pref/drives.h"
#include "gui/pref/interface.h"
#include "gui/pref/performance.h"
#include "gui/pref/input.h"
#include "gui/pref/subtitles.h"
#include "gui/pref/advanced.h"
#include "gui/pref/prefplaylist.h"
#include "gui/pref/updates.h"
#include "gui/pref/network.h"

#if USE_ASSOCIATIONS
#include "gui/pref/associations.h"
#endif


namespace Gui {
namespace Pref {

TDialog::TDialog(QWidget* parent, Qt::WindowFlags f)
	: QDialog(parent, f) {

	setupUi(this);

	// Setup buttons
	okButton = buttonBox->button(QDialogButtonBox::Ok);
	cancelButton = buttonBox->button(QDialogButtonBox::Cancel);
	applyButton = buttonBox->button(QDialogButtonBox::Apply);
	helpButton = buttonBox->button(QDialogButtonBox::Help);
	connect(applyButton, SIGNAL(clicked()), this, SLOT(apply()));
	connect(helpButton, SIGNAL(clicked()), this, SLOT(showHelp()));
	

	setWindowIcon(Images::icon("logo"));

	help_window = new QTextBrowser(this);
	help_window->setWindowFlags(Qt::Window);
	help_window->resize(300, 450);
	help_window->setWindowTitle(tr("SMPlayer - Help"));
	help_window->setWindowIcon(Images::icon("logo"));
	help_window->setOpenExternalLinks(true);

	// Get VO and AO driver lists from InfoReader:
	InfoReader* i = InfoReader::obj();
	i->getInfo();

	// TODO: parent
	page_general = new TGeneral(0);
	addSection(page_general);
	connect(page_general, SIGNAL(binChanged(const QString&)),
			this, SLOT(binChanged(const QString&)));

	page_interface = new TInterface;
	addSection(page_interface);

	page_input = new TInput;
	addSection(page_input);

	page_playlist = new TPrefPlaylist;
	addSection(page_playlist);

	page_video = new TVideo(0, i->voList());
	addSection(page_video);

	page_audio = new TAudio(0, i->aoList());
	addSection(page_audio);

	page_subtitles = new TSubtitles;
	addSection(page_subtitles);

	page_drives = new TDrives;
	addSection(page_drives);

	page_performance = new TPerformance;
	addSection(page_performance);

	page_network = new TNetwork;
	addSection(page_network);

	page_updates = new TUpdates;
	addSection(page_updates);

#if USE_ASSOCIATIONS
	page_associations = new TAssociations;
	addSection(page_associations);
#endif

	page_advanced = new TAdvanced;
	addSection(page_advanced);

	//sections->setIconSize(QSize(22,22));
	sections->setCurrentRow(SECTION_GENERAL);

	retranslateStrings();
}

TDialog::~TDialog() {
}

void TDialog::showSection(TSection section) {
	qDebug("Gui::Pref::TDialog::showSection: %d", section);

	sections->setCurrentRow(section);
}

void TDialog::retranslateStrings() {

	retranslateUi(this);

	for (int n = 0; n < pages->count(); n++) {
		TWidget* w = (TWidget*) pages->widget(n);
		sections->item(n)->setText(w->sectionName());
		sections->item(n)->setIcon(w->sectionIcon());
	}

	if (help_window->isVisible()) {
		// Makes the help to retranslate
		showHelp();
	}

	help_window->setWindowTitle(tr("SMPlayer - Help"));
}

void TDialog::accept() {

	hide();
	help_window->hide();
	setResult(QDialog::Accepted);
	emit applied();
}

void TDialog::apply() {

	setResult(QDialog::Accepted);
	emit applied();
}

void TDialog::reject() {

	hide();
	help_window->hide();
	setResult(QDialog::Rejected);

	setResult(QDialog::Accepted);
}

void TDialog::binChanged(const QString& path) {
	qDebug() << "Gui::Pref::TDialog::binChanged:" << path;

	InfoReader* i = InfoReader::obj();
	i->getInfo(path);
	page_video->vo_list = i->voList();
	page_video->updateDriverCombo(false);
	page_audio->ao_list = i->aoList();
	page_audio->updateDriverCombo(false);
}

void TDialog::addSection(TWidget *w) {

	QListWidgetItem *i = new QListWidgetItem(w->sectionIcon(), w->sectionName());
	sections->addItem(i);
	pages->addWidget(w);
}

void TDialog::setData(Settings::TPreferences* pref) {

	page_general->setData(pref);
	page_interface->setData(pref);
	page_input->setData(pref);
	page_playlist->setData(pref);
	page_video->setData(pref);
	page_audio->setData(pref);
	page_subtitles->setData(pref);
	page_drives->setData(pref);
	page_performance->setData(pref);
	page_network->setData(pref);
	page_updates->setData(pref);

#if USE_ASSOCIATIONS
	page_associations->setData(pref);
#endif

	page_advanced->setData(pref);
}

void TDialog::getData(Settings::TPreferences* pref) {

	page_general->getData(pref);
	page_interface->getData(pref);
	page_input->getData(pref);
	page_playlist->getData(pref);
	page_video->getData(pref);
	page_audio->getData(pref);
	page_subtitles->getData(pref);
	page_drives->getData(pref);
	page_performance->getData(pref);
	page_network->getData(pref);
	page_updates->getData(pref);

#if USE_ASSOCIATIONS
	page_associations->getData(pref);
#endif

	page_advanced->getData(pref);
}

bool TDialog::requiresRestart() {

	bool need_restart = page_general->requiresRestart();
	if (!need_restart) need_restart = page_interface->requiresRestart();
	if (!need_restart) need_restart = page_input->requiresRestart();
	if (!need_restart) need_restart = page_playlist->requiresRestart();
	if (!need_restart) need_restart = page_video->requiresRestart();
	if (!need_restart) need_restart = page_audio->requiresRestart();
	if (!need_restart) need_restart = page_subtitles->requiresRestart();
	if (!need_restart) need_restart = page_drives->requiresRestart();
	if (!need_restart) need_restart = page_performance->requiresRestart();
	if (!need_restart) need_restart = page_network->requiresRestart();
	if (!need_restart) need_restart = page_updates->requiresRestart();
	if (!need_restart) need_restart = page_advanced->requiresRestart();

	return need_restart;
}

void TDialog::showHelp() {

	TWidget* w = (TWidget*) pages->currentWidget();
	help_window->setHtml(w->help());
	help_window->show();
	help_window->raise();
}

// Language change stuff
void TDialog::changeEvent(QEvent *e) {

	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	} else {
		QDialog::changeEvent(e);
	}
}

} // namespace Pref
} // namespace Gui

#include "moc_dialog.cpp"
