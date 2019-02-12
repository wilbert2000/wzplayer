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
#include "images.h"

namespace Gui {
namespace Pref {

TPlaylistSection::TPlaylistSection(QWidget* parent, Qt::WindowFlags f)
    : TSection(parent, f),
    debug(logger()) {

    setupUi(this);

    retranslateStrings();
}

TPlaylistSection::~TPlaylistSection() {
}

QString TPlaylistSection::sectionName() {
    return tr("Playlist");
}

QPixmap TPlaylistSection::sectionIcon() {
    return Images::icon("playlist", iconSize);
}

void TPlaylistSection::retranslateStrings() {

    retranslateUi(this);

    // Playlist
    int index = media_to_add_combo->currentIndex();
    media_to_add_combo->clear();
    media_to_add_combo->addItem(tr("None"), Settings::TPreferences::NoFiles);
    media_to_add_combo->addItem(tr("Video files"), Settings::TPreferences::VideoFiles);
    media_to_add_combo->addItem(tr("Audio files"), Settings::TPreferences::AudioFiles);
    media_to_add_combo->addItem(tr("Video and audio files"), Settings::TPreferences::MultimediaFiles);
    media_to_add_combo->addItem(tr("Consecutive files"), Settings::TPreferences::ConsecutiveFiles);
    media_to_add_combo->setCurrentIndex(index);

    createHelp();
}

void TPlaylistSection::setData(Settings::TPreferences* pref) {

    setMediaToAddToPlaylist(pref->mediaToAddToPlaylist);

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

    pref->mediaToAddToPlaylist = mediaToAddToPlaylist();
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

void TPlaylistSection::setMediaToAddToPlaylist(Settings::TPreferences::TAddToPlaylist type) {

    int i = media_to_add_combo->findData(type);
    if (i < 0)
        i = 0;
    media_to_add_combo->setCurrentIndex(i);
}

Settings::TPreferences::TAddToPlaylist TPlaylistSection::mediaToAddToPlaylist() {
    return (Settings::TPreferences::TAddToPlaylist)
        media_to_add_combo->itemData(media_to_add_combo->currentIndex()).toInt();
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

    setWhatsThis(media_to_add_combo, tr("Add files from folder"),
        tr("This option allows to add files automatically to the playlist:") +"<br>"+
        tr("<b>None</b>: no files will be added") +"<br>"+
        tr("<b>Video files</b>: all video files found in the folder will be added") +"<br>"+
        tr("<b>Audio files</b>: all audio files found in the folder will be added") +"<br>"+
        tr("<b>Video and audio files</b>: all video and audio files found in the folder will be added") +"<br>"+
        tr("<b>Consecutive files</b>: consecutive files (like video_1.avi, video_2.avi) will be added"));

    setWhatsThis(recursive_check, tr("Add files in directories recursively"),
        tr("Check this option if you want that adding a directory will also "
        "add the files in subdirectories recursively. Otherwise only the "
        "files in the selected directory will be added."));
}

}} // namespace Gui::Pref

#include "moc_playlistsection.cpp"
