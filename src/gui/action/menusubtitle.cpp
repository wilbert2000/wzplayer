#include "gui/action/menusubtitle.h"
#include <QWidget>
#include "settings/mediasettings.h"
#include "gui/action/actiongroup.h"
#include "player/player.h"
#include "gui/mainwindow.h"

using namespace Settings;

namespace Gui {
namespace Action {


class TMenuCC : public TMenu {
public:
    explicit TMenuCC(TMainWindow* mw);
protected:
    virtual void enableActions();
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
	virtual void onAboutToShow();
private:
    TActionGroup* group;
};

TMenuCC::TMenuCC(TMainWindow* mw)
    : TMenu(mw, mw, "closed_captions_menu", tr("&Closed captions"),
            "closed_caption") {

	group = new TActionGroup(this, "cc");
	group->setEnabled(false);
	new TActionGroupItem(this, group, "cc_none", tr("&Off"), 0);
	new TActionGroupItem(this, group, "cc_ch_1", tr("&1"), 1);
	new TActionGroupItem(this, group, "cc_ch_2", tr("&2"), 2);
	new TActionGroupItem(this, group, "cc_ch_3", tr("&3"), 3);
	new TActionGroupItem(this, group, "cc_ch_4", tr("&4"), 4);
    group->setChecked(player->mset.closed_caption_channel);
    connect(group, SIGNAL(activated(int)), player, SLOT(changeClosedCaptionChannel(int)));
	// Currently no one else sets it
    addActionsTo(main_window);
}

void TMenuCC::enableActions() {
	// Using mset, so useless to set if stopped.
    group->setEnabled(player->statePOP() && player->hasVideo());
}

void TMenuCC::onMediaSettingsChanged(TMediaSettings* mset) {
	group->setChecked(mset->closed_caption_channel);
}

void TMenuCC::onAboutToShow() {
    group->setChecked(player->mset.closed_caption_channel);
}


TMenuSubFPS::TMenuSubFPS(TMainWindow* mw)
    : TMenu(mw, mw, "subfps_menu", tr("F&rames per second external subtitles"),
            "subfps") {

	group = new TActionGroup(this, "subfps");
	group->setEnabled(false);
	new TActionGroupItem(this, group, "sub_fps_none", tr("&Default"), TMediaSettings::SFPS_None);
	new TActionGroupItem(this, group, "sub_fps_23976", tr("23.9&76"), TMediaSettings::SFPS_23976);
	new TActionGroupItem(this, group, "sub_fps_24", tr("2&4"), TMediaSettings::SFPS_24);
	new TActionGroupItem(this, group, "sub_fps_25", tr("2&5"), TMediaSettings::SFPS_25);
	new TActionGroupItem(this, group, "sub_fps_29970", tr("29.&970"), TMediaSettings::SFPS_29970);
	new TActionGroupItem(this, group, "sub_fps_30", tr("3&0"), TMediaSettings::SFPS_30);
    group->setChecked(player->mset.external_subtitles_fps);
    connect(group, SIGNAL(activated(int)), player, SLOT(changeExternalSubFPS(int)));
	// No one else sets it
    addActionsTo(main_window);
}

void TMenuSubFPS::enableActions() {
    group->setEnabled(player->statePOP() && player->hasExternalSubs());
}

void TMenuSubFPS::onMediaSettingsChanged(TMediaSettings* mset) {
	group->setChecked(mset->external_subtitles_fps);
}

void TMenuSubFPS::onAboutToShow() {
    group->setChecked(player->mset.external_subtitles_fps);
}


TMenuSubtitle::TMenuSubtitle(TMainWindow* mw)
    : TMenu(mw, mw, "subtitle_menu", tr("&Subtitles"), "noicon") {

    decSubPosAct = new TAction(this, "dec_sub_pos", tr("&Up"), "", Qt::Key_Up);
    connect(decSubPosAct, SIGNAL(triggered()), player, SLOT(decSubPos()));
    incSubPosAct = new TAction(this, "inc_sub_pos", tr("&Down"), "", Qt::Key_Down);
    connect(incSubPosAct, SIGNAL(triggered()), player, SLOT(incSubPos()));

	addSeparator();
    incSubScaleAct = new TAction(this, "inc_sub_scale", tr("Si&ze +"), "", Qt::Key_K);
    connect(incSubScaleAct, SIGNAL(triggered()), player, SLOT(incSubScale()));
    decSubScaleAct = new TAction(this, "dec_sub_scale", tr("S&ize -"), "", Qt::SHIFT | Qt::Key_K);
    connect(decSubScaleAct, SIGNAL(triggered()), player, SLOT(decSubScale()));

	addSeparator();
    incSubDelayAct = new TAction(this, "inc_sub_delay", tr("Delay &+"), "", Qt::ALT | Qt::Key_D);
    connect(incSubDelayAct, SIGNAL(triggered()), player, SLOT(incSubDelay()));
    decSubDelayAct = new TAction(this, "dec_sub_delay", tr("Delay &-"), "", Qt::SHIFT | Qt::Key_D);
    connect(decSubDelayAct, SIGNAL(triggered()), player, SLOT(decSubDelay()));
    subDelayAct = new TAction(this, "sub_delay", tr("Se&t delay..."), "", Qt::META | Qt::Key_D);
    connect(subDelayAct, SIGNAL(triggered()), main_window, SLOT(showSubDelayDialog()));

	addSeparator();
    incSubStepAct = new TAction(this, "inc_sub_step",
        tr("N&ext line in subtitles"), "", Qt::Key_L);
    connect(incSubStepAct, SIGNAL(triggered()), player, SLOT(incSubStep()));
    decSubStepAct = new TAction(this, "dec_sub_step",
        tr("&Previous line in subtitles"), "", Qt::SHIFT | Qt::Key_L);
    connect(decSubStepAct, SIGNAL(triggered()), player, SLOT(decSubStep()));

    seekNextSubAct = new TAction(this, "seek_next_sub",
        tr("Seek to next subtitle"), "", Qt::Key_N, pref->isMPV());
    connect(seekNextSubAct, SIGNAL(triggered()), player, SLOT(seekToNextSub()));
    seekPrevSubAct = new TAction(this, "seek_prev_sub",
        tr("Seek to previous subtitle"), "", Qt::SHIFT | Qt::Key_N, pref->isMPV());
    connect(seekPrevSubAct, SIGNAL(triggered()), player, SLOT(seekToPrevSub()));

	// Subtitle tracks
	addSeparator();
    subtitleTrackMenu = new TMenu(main_window, main_window,
        "subtitlestrack_menu", tr("Subtitle &track"), "sub");
    nextSubtitleAct = new TAction(this, "next_subtitle", tr("Next subtitle track"),
        "", Qt::CTRL | Qt::Key_N, false);
	subtitleTrackMenu->addAction(nextSubtitleAct);
    main_window->addAction(nextSubtitleAct);
    subtitleTrackMenu->addSeparator();
	addMenu(subtitleTrackMenu);
    connect(nextSubtitleAct, SIGNAL(triggered()), player, SLOT(nextSubtitle()));

	subtitleTrackGroup = new TActionGroup(this, "subtitletrack");
    connect(subtitleTrackGroup, SIGNAL(activated(int)), player, SLOT(changeSubtitle(int)));
    connect(player, SIGNAL(subtitlesChanged()), this, SLOT(updateSubtitles()));
    connect(player, SIGNAL(subtitleTrackChanged(int)), this, SLOT(updateSubtitles()));

	// Secondary subtitle track
    secondarySubtitleTrackMenu = new TMenu(main_window, main_window,
        "secondary_subtitles_track_menu", tr("Secondary trac&k"), "secondary_sub");
	if (pref->isMPV())
		addMenu(secondarySubtitleTrackMenu);
	secondarySubtitleTrackGroup = new TActionGroup(this, "secondarysubtitletrack");
    connect(secondarySubtitleTrackGroup, SIGNAL(activated(int)),
            player, SLOT(changeSecondarySubtitle(int)));
    connect(player, SIGNAL(secondarySubtitleTrackChanged(int)),
            this, SLOT(updateSubtitles()));

    addMenu(new TMenuCC(main_window));

    useForcedSubsOnlyAct = new TAction(this, "use_forced_subs_only",
        tr("&Forced subtitles only"), "forced_subs");
	useForcedSubsOnlyAct->setCheckable(true);
	useForcedSubsOnlyAct->setChecked(pref->use_forced_subs_only);
    connect(useForcedSubsOnlyAct, SIGNAL(triggered(bool)), player,
            SLOT(toggleForcedSubsOnly(bool)));

	addSeparator();
	loadSubsAct = new TAction(this, "load_subs", tr("&Load subtitles..."), "open");
    connect(loadSubsAct, SIGNAL(triggered()), main_window, SLOT(loadSub()));
	unloadSubsAct = new TAction(this, "unload_subs", tr("U&nload subtitles"), "unload");
    connect(unloadSubsAct, SIGNAL(triggered()), player, SLOT(unloadSub()));
    subFPSMenu = new TMenuSubFPS(main_window);
	addMenu(subFPSMenu);

	addSeparator();
    useCustomSubStyleAct = new TAction(this, "use_custom_sub_style",
                                       tr("Use custo&m style"));
	useCustomSubStyleAct->setCheckable(true);
	useCustomSubStyleAct->setChecked(pref->use_custom_ass_style);
    connect(useCustomSubStyleAct, SIGNAL(triggered(bool)),
            player, SLOT(changeUseCustomSubStyle(bool)));

    addActionsTo(main_window);
}

void TMenuSubtitle::enableActions() {

    bool pop = player->statePOP();
    bool e = pop && (player->mdat.subs.count() > 0
                     || player->mset.closed_caption_channel > 0);
	decSubPosAct->setEnabled(e);
	incSubPosAct->setEnabled(e);
	incSubScaleAct->setEnabled(e);
	decSubScaleAct->setEnabled(e);

	decSubDelayAct->setEnabled(e);
	incSubDelayAct->setEnabled(e);
	subDelayAct->setEnabled(e);

	incSubStepAct->setEnabled(e);
	decSubStepAct->setEnabled(e);

	seekNextSubAct->setEnabled(e && pref->isMPV());
	seekPrevSubAct->setEnabled(e && pref->isMPV());

    nextSubtitleAct->setEnabled(e && player->mdat.subs.count() > 1);

    // useForcedSubsOnlyAct always enabled

    loadSubsAct->setEnabled(pop);
    unloadSubsAct->setEnabled(e && player->hasExternalSubs());

	// useCustomSubStyleAct always enabled
}

void TMenuSubtitle::onMediaSettingsChanged(Settings::TMediaSettings*) {
	// Already handled by updateSubtitles
}

void TMenuSubtitle::updateSubtitles() {
	logger()->debug("updateSubtitles");

	// Note: use idx not ID
	subtitleTrackGroup->clear();
	secondarySubtitleTrackGroup->clear();

	QAction* subNoneAct = subtitleTrackGroup->addAction(tr("&None"));
	subNoneAct->setData(SubData::None);
	subNoneAct->setCheckable(true);
    if (player->mset.current_sub_idx < 0) {
		subNoneAct->setChecked(true);
	}
	subNoneAct = secondarySubtitleTrackGroup->addAction(tr("&None"));
	subNoneAct->setData(SubData::None);
	subNoneAct->setCheckable(true);
    if (player->mset.current_secondary_sub_idx < 0) {
		subNoneAct->setChecked(true);
	}

    for (int idx = 0; idx < player->mdat.subs.count(); idx++) {
        SubData sub = player->mdat.subs.itemAt(idx);
		QAction *a = new QAction(subtitleTrackGroup);
		a->setCheckable(true);
		a->setText(sub.displayName());
		a->setData(idx);
        if (idx == player->mset.current_sub_idx) {
			a->setChecked(true);
		}

		if (pref->isMPV()) {
            if (idx == player->mset.current_secondary_sub_idx) {
				a->setEnabled(false);
			}

			a = new QAction(secondarySubtitleTrackGroup);
			a->setCheckable(true);
			a->setText(sub.displayName());
			a->setData(idx);
            if (idx == player->mset.current_secondary_sub_idx) {
				a->setChecked(true);
			}
            if (idx == player->mset.current_sub_idx) {
				a->setEnabled(false);
			}
		}
	}

	subtitleTrackMenu->addActions(subtitleTrackGroup->actions());
	secondarySubtitleTrackMenu->addActions(secondarySubtitleTrackGroup->actions());

	// Enable actions
    enableActions();
    subFPSMenu->enableActions();
}

} // namespace Action
} // namespace Gui
