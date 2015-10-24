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

#include "settings/preferences.h"

#include <QVBoxLayout>
#include <QTextBrowser>

#include "images.h"

namespace Gui { namespace Pref {

TDialog::TDialog(QWidget* parent, Qt::WindowFlags f)
	: QDialog(parent, f)
{
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
	//help_window->adjustSize();
	help_window->setWindowTitle(tr("SMPlayer - Help"));
	help_window->setWindowIcon(Images::icon("logo"));
	help_window->setOpenExternalLinks(true);

	page_general = new TGeneral;
	addSection(page_general);

	page_drives = new TDrives;
	addSection(page_drives);

	page_performance = new TPerformance;
	addSection(page_performance);

	page_subtitles = new TSubtitles;
	addSection(page_subtitles);

	page_interface = new TInterface;
	addSection(page_interface);

	page_input = new TInput;
	addSection(page_input);

	page_playlist = new TPrefPlaylist;
	addSection(page_playlist);

	page_tv = new TTV;
	addSection(page_tv);

#if USE_ASSOCIATIONS
	page_associations = new TAssociations;
	addSection(page_associations);
#endif

	page_updates = new TUpdates;
	addSection(page_updates);

	page_network = new TNetwork;
	addSection(page_network);

	page_advanced = new TAdvanced;
	addSection(page_advanced);

	//sections->setIconSize(QSize(22,22));
	sections->setCurrentRow(General);

	//adjustSize();
	retranslateStrings();
}

TDialog::~TDialog()
{
}

void TDialog::showSection(Section s) {
	qDebug("Gui::Pref::TDialog::showSection: %d", s);

	sections->setCurrentRow(s);
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

void TDialog::addSection(TWidget *w) {
	QListWidgetItem *i = new QListWidgetItem(w->sectionIcon(), w->sectionName());
	sections->addItem(i);
	pages->addWidget(w);
}

void TDialog::setData(Settings::TPreferences* pref) {
	page_general->setData(pref);
	page_drives->setData(pref);
	page_interface->setData(pref);
	page_performance->setData(pref);
	page_input->setData(pref);
	page_subtitles->setData(pref);
	page_advanced->setData(pref);
	page_playlist->setData(pref);
	page_tv->setData(pref);
	page_updates->setData(pref);
	page_network->setData(pref);

#if USE_ASSOCIATIONS
	page_associations->setData(pref);
#endif
}

void TDialog::getData(Settings::TPreferences* pref) {
	page_general->getData(pref);
	page_drives->getData(pref);
	page_interface->getData(pref);
	page_performance->getData(pref);
	page_input->getData(pref);
	page_subtitles->getData(pref);
	page_advanced->getData(pref);
	page_playlist->getData(pref);
	page_tv->getData(pref);
	page_updates->getData(pref);
	page_network->getData(pref);

#if USE_ASSOCIATIONS
	page_associations->getData(pref);
#endif
}

bool TDialog::requiresRestart() {
	bool need_restart = page_general->requiresRestart();
	if (!need_restart) need_restart = page_drives->requiresRestart();
	if (!need_restart) need_restart = page_interface->requiresRestart();
	if (!need_restart) need_restart = page_performance->requiresRestart();
	if (!need_restart) need_restart = page_input->requiresRestart();
	if (!need_restart) need_restart = page_subtitles->requiresRestart();
	if (!need_restart) need_restart = page_advanced->requiresRestart();
	if (!need_restart) need_restart = page_playlist->requiresRestart();
	if (!need_restart) need_restart = page_tv->requiresRestart();
	if (!need_restart) need_restart = page_updates->requiresRestart();
	if (!need_restart) need_restart = page_network->requiresRestart();

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

}} // namespace Gui::Pref

#include "moc_dialog.cpp"
