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

#include "gui/pref/playlistsection.h"
#include "settings/preferences.h"
#include "images.h"


namespace Gui {
namespace Pref {

TPlaylistSection::TPlaylistSection(QWidget* parent, Qt::WindowFlags f)
    : TSection(parent, f) {

    setupUi(this);

    retranslateStrings();
}

QString TPlaylistSection::sectionName() {
    return tr("Playlist");
}

QPixmap TPlaylistSection::sectionIcon() {
    return Images::icon("playlist", iconSize);
}

void TPlaylistSection::retranslateStrings() {

    retranslateUi(this);
    createHelp();
}

void TPlaylistSection::setData(Settings::TPreferences* pref) {

    setDirectoryRecursion(pref->addDirectories);
    video_check->setChecked(pref->addVideo);
    audio_check->setChecked(pref->addAudio);
    playlists_check->setChecked(pref->addPlaylists);
    images_check->setChecked(pref->addImages);

    image_duration_spinbox->setValue(pref->imageDuration);

    directory_playlist_check->setChecked(pref->useDirectoriePlaylists);

    name_blacklist_edit->setPlainText(pref->nameBlacklist.join("\n"));
    title_blacklist_edit->setPlainText(pref->titleBlacklist.join("\n"));
}

void TPlaylistSection::getData(Settings::TPreferences* pref) {

    TSection::getData(pref);

    pref->addDirectories = directoryRecursion();
    pref->addVideo = video_check->isChecked();
    pref->addAudio = audio_check->isChecked();
    pref->addPlaylists = playlists_check->isChecked();
    if (pref->addImages != images_check->isChecked()) {
        pref->addImages = images_check->isChecked();
        _requiresRestartApp = true;
        WZDEBUG("playlist addImages changed, restarting app");
    }
    pref->imageDuration = image_duration_spinbox->value();
    pref->useDirectoriePlaylists = directory_playlist_check->isChecked();

    pref->nameBlacklist = name_blacklist_edit->toPlainText().split("\n",
        QString::SkipEmptyParts);
    pref->titleBlacklist = title_blacklist_edit->toPlainText().split("\n",
        QString::SkipEmptyParts);
    pref->compileTitleBlackList();
}

void TPlaylistSection::setDirectoryRecursion(bool b) {
    recursive_check->setChecked(b);
}

bool TPlaylistSection::directoryRecursion() {
    return recursive_check->isChecked();
}

void TPlaylistSection::createHelp() {

    clearHelp();
    addSectionTitle(tr("Playlist"));
    setWhatsThis(recursive_check, tr("Add files in directories recursively"),
        tr("Check this option to add files in subdirectories recursively"
           " to the playlist when opening a directory. Otherwise only the files"
           " directly inside the directory will be added to the playlist."));
}

}} // namespace Gui::Pref

#include "moc_playlistsection.cpp"
