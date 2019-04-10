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


#include "gui/pref/dialog.h"
#include "gui/mainwindow.h"
#include "settings/preferences.h"
#include "gui/pref/section.h"
#include "gui/pref/playersection.h"
#include "gui/pref/demuxer.h"
#include "gui/pref/video.h"
#include "gui/pref/audio.h"
#include "gui/pref/subtitles.h"
#include "gui/pref/interface.h"
#include "gui/pref/playlistsection.h"
#include "gui/pref/input.h"
#include "gui/pref/drives.h"
#include "gui/pref/capture.h"
#include "gui/pref/performance.h"
#include "gui/pref/network.h"
#include "config.h"
#include "images.h"
#include "desktop.h"


#if USE_ASSOCIATIONS
#include "gui/pref/associations.h"
#endif

#include <QTextBrowser>


namespace Gui {
namespace Pref {

TDialog::TDialog(TMainWindow* mw) :
    QDialog(mw) {

    setupUi(this);

    // Restore pos and size
    Settings::pref->beginGroup("settingsdialog");
    QPoint p = Settings::pref->value("pos", QPoint()).toPoint();
    QSize s = Settings::pref->value("size", QPoint()).toSize();
    Settings::pref->endGroup();

    if (s.width() > 400 && s.height() > 400) {
        move(p);
        resize(s);
        TDesktop::keepInsideDesktop(this);
    }

    helpButton = buttonBox->button(QDialogButtonBox::Help);
    connect(helpButton, &QPushButton::clicked, this, &TDialog::showHelp);

    setWindowIcon(Images::icon("logo"));

    help_window = new QTextBrowser(this);
    help_window->setWindowFlags(Qt::Window);
    help_window->resize(300, 450);
    help_window->setWindowTitle(tr("%1 - Help").arg(TConfig::PROGRAM_NAME));
    help_window->setWindowIcon(Images::icon("logo"));
    help_window->setOpenExternalLinks(true);

    // Get VO and AO driver lists from TPlayerInfo:
    Player::Info::TPlayerInfo* i = Player::Info::TPlayerInfo::obj();
    i->getInfo();

    page_player = new TPlayerSection(this);
    addSection(page_player);
    connect(page_player, &TPlayerSection::binChanged,
            this, &TDialog::onBinChanged);

    page_demuxer = new TDemuxer(this);
    addSection(page_demuxer);

    page_video = new TVideo(this, i->voList());
    addSection(page_video);

    page_audio = new TAudio(this, i->aoList());
    addSection(page_audio);

    page_subtitles = new TSubtitles(this);
    addSection(page_subtitles);

    page_interface = new TInterface(this);
    addSection(page_interface);

    page_playlist = new TPlaylistSection(this);
    addSection(page_playlist);

    page_input = new TInput(this, mw);
    addSection(page_input);

    page_drives = new TDrives(this);
    addSection(page_drives);

    page_capture = new TCapture(this);
    addSection(page_capture);

    page_performance = new TPerformance(this);
    addSection(page_performance);

    page_network = new TNetwork(this);
    addSection(page_network);

#if USE_ASSOCIATIONS
    page_associations = new TAssociations(this);
    addSection(page_associations);
#endif

    sections->setCurrentRow(SECTION_PLAYER);

    retranslateStrings();
}

void TDialog::showSection(TSectionNumber section) {
    sections->setCurrentRow(section);
}

void TDialog::retranslateStrings() {

    retranslateUi(this);

    for (int n = 0; n < pages->count(); n++) {
        TSection* w = (TSection*) pages->widget(n);
        sections->item(n)->setText(w->sectionName());
        sections->item(n)->setIcon(w->sectionIcon());
    }

    if (help_window->isVisible()) {
        // Makes the help to retranslate
        showHelp();
    }

    help_window->setWindowTitle(tr("%1 - Help").arg(TConfig::PROGRAM_NAME));
}

void TDialog::saveSettings() {

    Settings::pref->beginGroup("settingsdialog");
    Settings::pref->setValue("pos", pos());
    Settings::pref->setValue("size", size());
    Settings::pref->endGroup();

}

void TDialog::accept() {

    help_window->hide();
    hide();
    setResult(QDialog::Accepted);
    emit applied();
}

void TDialog::reject() {

    help_window->hide();
    hide();
    setResult(QDialog::Rejected);
}

void TDialog::onBinChanged(Settings::TPreferences::TPlayerID player_id,
                           bool keep_current_drivers,
                           const QString& path) {

    Player::Info::TPlayerInfo* i = Player::Info::TPlayerInfo::obj();
    i->getInfo(path);
    page_video->vo_list = i->voList();
    page_video->updateDriverCombo(player_id, keep_current_drivers);
    page_audio->ao_list = i->aoList();
    page_audio->updateDriverCombo(player_id, keep_current_drivers);
}

void TDialog::addSection(TSection* s) {

    QListWidgetItem *i = new QListWidgetItem(s->sectionIcon(), s->sectionName());
    sections->addItem(i);
    pages->addWidget(s);
}

void TDialog::setData(Settings::TPreferences* pref) {

    page_player->setData(pref);
    page_demuxer->setData(pref);
    page_video->setData(pref);
    page_audio->setData(pref);
    page_subtitles->setData(pref);
    page_interface->setData(pref);
    page_playlist->setData(pref);
    page_input->setData(pref);
    page_drives->setData(pref);
    page_capture->setData(pref);
    page_performance->setData(pref);
    page_network->setData(pref);

#if USE_ASSOCIATIONS
    page_associations->setData(pref);
#endif

}

void TDialog::getData(Settings::TPreferences* pref) {

    page_player->getData(pref);
    page_demuxer->getData(pref);
    page_video->getData(pref);
    page_audio->getData(pref);
    page_subtitles->getData(pref);
    page_interface->getData(pref);
    page_playlist->getData(pref);
    page_input->getData(pref);
    page_drives->getData(pref);
    page_capture->getData(pref);
    page_performance->getData(pref);
    page_network->getData(pref);

#if USE_ASSOCIATIONS
    page_associations->getData(pref);
#endif

}

bool TDialog::requiresRestartApp() {

    return page_player->requiresRestartApp()
            || page_demuxer->requiresRestartApp()
            || page_video->requiresRestartApp()
            || page_audio->requiresRestartApp()
            || page_subtitles->requiresRestartApp()
            || page_interface->requiresRestartApp()
            || page_playlist->requiresRestartApp()
            || page_input->requiresRestartApp()
            || page_drives->requiresRestartApp()
            || page_capture->requiresRestartApp()
            || page_performance->requiresRestartApp()
            || page_network->requiresRestartApp();
}

bool TDialog::requiresRestartPlayer() {

    return page_player->requiresRestartPlayer()
            || page_demuxer->requiresRestartPlayer()
            || page_video->requiresRestartPlayer()
            || page_audio->requiresRestartPlayer()
            || page_subtitles->requiresRestartPlayer()
            || page_interface->requiresRestartPlayer()
            || page_playlist->requiresRestartPlayer()
            || page_input->requiresRestartPlayer()
            || page_drives->requiresRestartPlayer()
            || page_capture->requiresRestartPlayer()
            || page_performance->requiresRestartPlayer()
            || page_network->requiresRestartPlayer();
}

void TDialog::showHelp() {

    TSection* w = (TSection*) pages->currentWidget();
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
