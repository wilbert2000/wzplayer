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

#ifndef GUI_PREF_DIALOG_H
#define GUI_PREF_DIALOG_H

#include "ui_dialog.h"
#include "log4qt/logger.h"
#include "settings/preferences.h"


class QTextBrowser;
class QPushButton;


namespace Gui {
namespace Pref {

class TSection;
class TPlayerSection;
class TDemuxer;
class TVideo;
class TAudio;
class TSubtitles;
class TInterface;
class TPlaylistSection;
class TInput;
class TDrives;
class TCapture;
class TUpdates;
class TNetwork;
class TPerformance;

#if USE_ASSOCIATIONS
class TAssociations;
#endif


class TDialog : public QDialog, public Ui::TDialog {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    enum TSectionNumber {
        SECTION_PLAYER = 0,
        SECTION_DEMUXER,
        SECTION_VIDEO,
        SECTION_AUDIO,
        SECTION_SUBTITLES,
        SECTION_GUI,
        SECTION_PLAYLIST,
        SECTION_INPUT,
        SECTION_DRIVES,
        SECTION_CAPTURE,
        SECTION_PERFORMANCE,
        SECTION_NETWORK
#if USE_ASSOCIATIONS
        ,SECTION_ASSOCIATIONS
#endif
    };

    TDialog(QWidget* parent);

    TInput* mod_input() const { return page_input; }

    // Pass data to the standard dialogs
    void setData(Settings::TPreferences* pref);
    // Apply changes
    void getData(Settings::TPreferences* pref);

    // Return true if the application should be restarted.
    bool requiresRestartApp();
    // Return true if the player process should be restarted.
    bool requiresRestartPlayer();

    void showSection(TSectionNumber section);

    void saveSettings();

public slots:
    virtual void accept(); // Reimplemented to send a signal
    virtual void reject();

signals:
    void applied();

protected:
    virtual void retranslateStrings();
    virtual void changeEvent(QEvent* event);

private:
    TPlayerSection* page_player;
    TDemuxer* page_demuxer;
    TVideo* page_video;
    TAudio* page_audio;
    TSubtitles* page_subtitles;
    TInterface* page_interface;
    TPlaylistSection* page_playlist;
    TInput* page_input;
    TDrives* page_drives;
    TCapture* page_capture;
    TPerformance* page_performance;
    TNetwork* page_network;

#if USE_ASSOCIATIONS
    TAssociations* page_associations;
#endif


    QTextBrowser* help_window;
    QPushButton* helpButton;

    void addSection(TSection* s);

private slots:
    void showHelp();
    void onBinChanged(Settings::TPreferences::TPlayerID player_id,
                      bool keep_current_drivers, const QString &path);
};

} // namespace Pref
} // namespace Gui

#endif // GUI_PREF_DIALOG_H
