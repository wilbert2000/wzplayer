#include "gui/action/menu/menusubtitle.h"
#include "settings/mediasettings.h"
#include "gui/action/actiongroup.h"
#include "player/player.h"
#include "gui/mainwindow.h"

using namespace Settings;

namespace Gui {
namespace Action {
namespace Menu {


TClosedCaptionsGroup::TClosedCaptionsGroup(TMainWindow* mw)
    : TActionGroup(mw, "closedcaptionsgroup") {

    setEnabled(false);
    new TActionGroupItem(mw, this, "cc_none", tr("Off"), 0);
    new TActionGroupItem(mw, this, "cc_ch_1", tr("1"), 1);
    new TActionGroupItem(mw, this, "cc_ch_2", tr("2"), 2);
    new TActionGroupItem(mw, this, "cc_ch_3", tr("3"), 3);
    new TActionGroupItem(mw, this, "cc_ch_4", tr("4"), 4);
    setChecked(player->mset.closed_caption_channel);
    connect(this, &TActionGroup::activated,
            player, &Player::TPlayer::setClosedCaptionChannel);
}

class TMenuCC : public TMenu {
public:
    explicit TMenuCC(QWidget* parent, TMainWindow* mw);
};

TMenuCC::TMenuCC(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, mw, "closed_captions_menu", tr("Closed captions")) {

    addActions(mw->findChild<TClosedCaptionsGroup*>()->actions());
}


TSubFPSGroup::TSubFPSGroup(TMainWindow* mw)
    : TActionGroup(mw, "subfpsgroup") {

    setEnabled(false);
    new TActionGroupItem(mw, this, "sub_fps_none", tr("Default"),
                         TMediaSettings::SFPS_None);
    new TActionGroupItem(mw, this, "sub_fps_23976", tr("23.976"),
                         TMediaSettings::SFPS_23976);
    new TActionGroupItem(mw, this, "sub_fps_24", tr("24"),
                         TMediaSettings::SFPS_24);
    new TActionGroupItem(mw, this, "sub_fps_25", tr("25"),
                         TMediaSettings::SFPS_25);
    new TActionGroupItem(mw, this, "sub_fps_29970", tr("29.970"),
                         TMediaSettings::SFPS_29970);
    new TActionGroupItem(mw, this, "sub_fps_30", tr("30"),
                         TMediaSettings::SFPS_30);
    setChecked(player->mset.external_subtitles_fps);
    connect(this, &TSubFPSGroup::activated,
            player, &Player::TPlayer::changeExternalSubFPS);
}

class TMenuSubFPS : public TMenu {
public:
    explicit TMenuSubFPS(QWidget* parent, TMainWindow* mw);
};

TMenuSubFPS::TMenuSubFPS(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, mw, "subfps_menu", tr("FPS external subs")) {

    addActions(mw->findChild<TSubFPSGroup*>()->actions());
}


TMenuSubtitle::TMenuSubtitle(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, mw, "subtitle_menu", tr("Subtitles"), "noicon") {

    addAction(mw->findAction("dec_sub_pos"));
    addAction(mw->findAction("inc_sub_pos"));

    addSeparator();
    addAction(mw->findAction("inc_sub_scale"));
    addAction(mw->findAction("dec_sub_scale"));

    addSeparator();
    addAction(mw->findAction("inc_sub_delay"));
    addAction(mw->findAction("dec_sub_delay"));
    addAction(mw->findAction("sub_delay"));

    addSeparator();
    addAction(mw->findAction("inc_sub_step"));
    addAction(mw->findAction("dec_sub_step"));
    addAction(mw->findAction("seek_next_sub"));
    addAction(mw->findAction("seek_prev_sub"));

    // Subtitle tracks
    addSeparator();
    subtitleTrackMenu = new TMenu(this, mw, "subtitle_track_menu",
                                  tr("Subtitle track"), "sub");
    subtitleTrackMenu->addAction(mw->findAction("next_subtitle"));
    addMenu(subtitleTrackMenu);

    // Secondary subtitle track
    secondarySubtitleTrackMenu = new TMenu(this, mw, "subtitle_track2_menu",
        tr("Secondary track"), "secondary_sub");
    if (pref->isMPV()) {
        addMenu(secondarySubtitleTrackMenu);
    }

    connect(mw, &TMainWindow::subtitleTrackGroupsChanged,
            this, &TMenuSubtitle::subtitleTrackGroupsChanged);

    // Closed caption
    addMenu(new TMenuCC(this, mw));
    addAction(mw->findAction("use_forced_subs_only"));

    addSeparator();
    addAction(mw->findAction("load_subs"));
    addAction(mw->findAction("unload_subs"));
    addMenu(new TMenuSubFPS(this, mw));

    addSeparator();
    addAction(mw->findAction("use_custom_sub_style"));
}

void TMenuSubtitle::subtitleTrackGroupsChanged(
        Action::TAction* next,
        Action::TActionGroup* subGroup,
        Action::TActionGroup* secSubGroup) {
    WZDEBUG("");

    subtitleTrackMenu->clear();
    subtitleTrackMenu->addAction(next);
    subtitleTrackMenu->addSeparator();
    subtitleTrackMenu->addActions(subGroup->actions());

    secondarySubtitleTrackMenu->clear();
    secondarySubtitleTrackMenu->addActions(secSubGroup->actions());
}


} // namespace Menu
} // namespace Action
} // namespace Gui
