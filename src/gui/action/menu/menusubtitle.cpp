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
    : TActionGroup(mw, "closed_captions_group") {

    setEnabled(false);
    new TActionGroupItem(mw, this, "cc_none", tr("Off"), 0);
    new TActionGroupItem(mw, this, "cc_ch_1", tr("1"), 1);
    new TActionGroupItem(mw, this, "cc_ch_2", tr("2"), 2);
    new TActionGroupItem(mw, this, "cc_ch_3", tr("3"), 3);
    new TActionGroupItem(mw, this, "cc_ch_4", tr("4"), 4);
    setChecked(player->mset.closed_caption_channel);
    connect(this, &TActionGroup::triggeredID,
            player, &Player::TPlayer::setClosedCaptionChannel);
}

class TMenuCC : public TMenu {
public:
    explicit TMenuCC(QWidget* parent, TMainWindow* mw);
};

TMenuCC::TMenuCC(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, "closed_captions_menu", tr("Closed captions")) {

    addActions(mw->findChild<TClosedCaptionsGroup*>()->actions());
}


TSubFPSGroup::TSubFPSGroup(TMainWindow* mw)
    : TActionGroup(mw, "sub_fps_group") {

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
    connect(this, &TSubFPSGroup::triggeredID,
            player, &Player::TPlayer::changeExternalSubFPS);
}

class TMenuSubFPS : public TMenu {
public:
    explicit TMenuSubFPS(QWidget* parent, TMainWindow* mw);
};

TMenuSubFPS::TMenuSubFPS(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, "subfps_menu", tr("FPS external subs")) {

    addActions(mw->findChild<TSubFPSGroup*>()->actions());
}


TMenuSubtitle::TMenuSubtitle(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, "subtitle_menu", tr("Subtitles"), "noicon") {

    addAction(mw->requireAction("dec_sub_pos"));
    addAction(mw->requireAction("inc_sub_pos"));

    addSeparator();
    addAction(mw->requireAction("inc_sub_scale"));
    addAction(mw->requireAction("dec_sub_scale"));

    addSeparator();
    addAction(mw->requireAction("inc_sub_delay"));
    addAction(mw->requireAction("dec_sub_delay"));
    addAction(mw->requireAction("sub_delay"));

    addSeparator();
    addAction(mw->requireAction("inc_sub_step"));
    addAction(mw->requireAction("dec_sub_step"));
    addAction(mw->requireAction("seek_next_sub"));
    addAction(mw->requireAction("seek_prev_sub"));

    // Subtitle tracks
    addSeparator();
    subtitleTrackMenu = new TMenu(this, "subtitle_track_menu",
                                  tr("Subtitle track"), "sub");
    subtitleTrackMenu->addAction(mw->requireAction("next_subtitle"));
    subtitleTrackMenu->addSeparator();
    addMenu(subtitleTrackMenu);

    // Secondary subtitle track
    secondarySubtitleTrackMenu = new TMenu(this, "subtitle_track2_menu",
        tr("Secondary track"), "secondary_sub");
    if (pref->isMPV()) {
        addMenu(secondarySubtitleTrackMenu);
    }

    connect(mw, &TMainWindow::subtitleTrackGroupsChanged,
            this, &TMenuSubtitle::subtitleTrackGroupsChanged);

    // Closed caption
    addMenu(new TMenuCC(this, mw));
    addAction(mw->requireAction("use_forced_subs_only"));

    addSeparator();
    addAction(mw->requireAction("load_subs"));
    addAction(mw->requireAction("unload_subs"));
    addMenu(new TMenuSubFPS(this, mw));

    addSeparator();
    addAction(mw->requireAction("use_custom_sub_style"));
}

void TMenuSubtitle::subtitleTrackGroupsChanged(
        Action::TActionGroup* subGroup,
        Action::TActionGroup* secSubGroup) {
    WZTRACE("");

    subtitleTrackMenu->addActions(subGroup->actions());
    secondarySubtitleTrackMenu->addActions(secSubGroup->actions());
}


} // namespace Menu
} // namespace Action
} // namespace Gui
