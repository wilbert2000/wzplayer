#include "gui/action/menusubtitle.h"
#include <QWidget>
#include "settings/mediasettings.h"
#include "gui/action/actiongroup.h"
#include "core.h"
#include "gui/base.h"

using namespace Settings;

namespace Gui {
namespace Action {


class TMenuCC : public TMenu {
public:
    explicit TMenuCC(TBase* mw, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool video, bool audio);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
	virtual void onAboutToShow();
private:
	TCore* core;
	TActionGroup* group;
};

TMenuCC::TMenuCC(TBase* mw, TCore* c)
    : TMenu(mw, mw, "closed_captions_menu", tr("&Closed captions"), "closed_caption")
	, core(c) {

	group = new TActionGroup(this, "cc");
	group->setEnabled(false);
	new TActionGroupItem(this, group, "cc_none", tr("&Off"), 0);
	new TActionGroupItem(this, group, "cc_ch_1", tr("&1"), 1);
	new TActionGroupItem(this, group, "cc_ch_2", tr("&2"), 2);
	new TActionGroupItem(this, group, "cc_ch_3", tr("&3"), 3);
	new TActionGroupItem(this, group, "cc_ch_4", tr("&4"), 4);
	group->setChecked(core->mset.closed_caption_channel);
	connect(group, SIGNAL(activated(int)), core, SLOT(changeClosedCaptionChannel(int)));
	// Currently no one else sets it
    addActionsTo(main_window);
}

void TMenuCC::enableActions(bool stopped, bool, bool) {
	// Using mset, so useless to set if stopped.
	// Assuming you can have closed captions on audio...
	group->setEnabled(!stopped);
}

void TMenuCC::onMediaSettingsChanged(TMediaSettings* mset) {
	group->setChecked(mset->closed_caption_channel);
}

void TMenuCC::onAboutToShow() {
	group->setChecked(core->mset.closed_caption_channel);
}


TMenuSubFPS::TMenuSubFPS(TBase* mw, TCore* c)
    : TMenu(mw, mw, "subfps_menu", tr("F&rames per second external subtitles"), "subfps")
	, core(c) {

	group = new TActionGroup(this, "subfps");
	group->setEnabled(false);
	new TActionGroupItem(this, group, "sub_fps_none", tr("&Default"), TMediaSettings::SFPS_None);
	new TActionGroupItem(this, group, "sub_fps_23976", tr("23.9&76"), TMediaSettings::SFPS_23976);
	new TActionGroupItem(this, group, "sub_fps_24", tr("2&4"), TMediaSettings::SFPS_24);
	new TActionGroupItem(this, group, "sub_fps_25", tr("2&5"), TMediaSettings::SFPS_25);
	new TActionGroupItem(this, group, "sub_fps_29970", tr("29.&970"), TMediaSettings::SFPS_29970);
	new TActionGroupItem(this, group, "sub_fps_30", tr("3&0"), TMediaSettings::SFPS_30);
	group->setChecked(core->mset.external_subtitles_fps);
	connect(group, SIGNAL(activated(int)), core, SLOT(changeExternalSubFPS(int)));
	// No one else sets it
    addActionsTo(main_window);
}

void TMenuSubFPS::enableActions(bool stopped, bool, bool) {
	group->setEnabled(!stopped && core->haveExternalSubs());
}

void TMenuSubFPS::onMediaSettingsChanged(TMediaSettings* mset) {
	group->setChecked(mset->external_subtitles_fps);
}

void TMenuSubFPS::onAboutToShow() {
	group->setChecked(core->mset.external_subtitles_fps);
}


TMenuSubtitle::TMenuSubtitle(TBase* mw, TCore* c)
    : TMenu(mw, mw, "subtitle_menu", tr("&Subtitles"), "noicon")
	, core(c) {

    decSubPosAct = new TAction(this, "dec_sub_pos", tr("&Up"), "", Qt::Key_Up);
	connect(decSubPosAct, SIGNAL(triggered()), core, SLOT(decSubPos()));
    incSubPosAct = new TAction(this, "inc_sub_pos", tr("&Down"), "", Qt::Key_Down);
	connect(incSubPosAct, SIGNAL(triggered()), core, SLOT(incSubPos()));

	addSeparator();
    incSubScaleAct = new TAction(this, "inc_sub_scale", tr("Si&ze +"), "", Qt::Key_K);
    connect(incSubScaleAct, SIGNAL(triggered()), core, SLOT(incSubScale()));
    decSubScaleAct = new TAction(this, "dec_sub_scale", tr("S&ize -"), "", Qt::SHIFT | Qt::Key_K);
	connect(decSubScaleAct, SIGNAL(triggered()), core, SLOT(decSubScale()));

	addSeparator();
    incSubDelayAct = new TAction(this, "inc_sub_delay", tr("Delay &+"), "", Qt::ALT | Qt::Key_D);
	connect(incSubDelayAct, SIGNAL(triggered()), core, SLOT(incSubDelay()));
    decSubDelayAct = new TAction(this, "dec_sub_delay", tr("Delay &-"), "", Qt::SHIFT | Qt::Key_D);
    connect(decSubDelayAct, SIGNAL(triggered()), core, SLOT(decSubDelay()));
    subDelayAct = new TAction(this, "sub_delay", tr("Se&t delay..."), "", Qt::META | Qt::Key_D);
    connect(subDelayAct, SIGNAL(triggered()), main_window, SLOT(showSubDelayDialog()));

	addSeparator();
    incSubStepAct = new TAction(this, "inc_sub_step",
        tr("N&ext line in subtitles"), "", Qt::Key_L);
    connect(incSubStepAct, SIGNAL(triggered()), core, SLOT(incSubStep()));
    decSubStepAct = new TAction(this, "dec_sub_step",
        tr("&Previous line in subtitles"), "", Qt::SHIFT | Qt::Key_L);
	connect(decSubStepAct, SIGNAL(triggered()), core, SLOT(decSubStep()));

    seekNextSubAct = new TAction(this, "seek_next_sub",
        tr("Seek to next subtitle"), "", Qt::Key_N, pref->isMPV());
	connect(seekNextSubAct, SIGNAL(triggered()), core, SLOT(seekToNextSub()));
    seekPrevSubAct = new TAction(this, "seek_prev_sub",
        tr("Seek to previous subtitle"), "", Qt::SHIFT | Qt::Key_N, pref->isMPV());
	connect(seekPrevSubAct, SIGNAL(triggered()), core, SLOT(seekToPrevSub()));

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
	connect(nextSubtitleAct, SIGNAL(triggered()), core, SLOT(nextSubtitle()));

	subtitleTrackGroup = new TActionGroup(this, "subtitletrack");
	connect(subtitleTrackGroup, SIGNAL(activated(int)), core, SLOT(changeSubtitle(int)));
	connect(core, SIGNAL(subtitlesChanged()), this, SLOT(updateSubtitles()));
	connect(core, SIGNAL(subtitleTrackChanged(int)), this, SLOT(updateSubtitles()));

	// Secondary subtitle track
    secondarySubtitleTrackMenu = new TMenu(main_window, main_window, "secondary_subtitles_track_menu", tr("Secondary trac&k"), "secondary_sub");
	if (pref->isMPV())
		addMenu(secondarySubtitleTrackMenu);
	secondarySubtitleTrackGroup = new TActionGroup(this, "secondarysubtitletrack");
	connect(secondarySubtitleTrackGroup, SIGNAL(activated(int)), core, SLOT(changeSecondarySubtitle(int)));
	connect(core, SIGNAL(secondarySubtitleTrackChanged(int)), this, SLOT(updateSubtitles()));

    addMenu(new TMenuCC(main_window, core));

	useForcedSubsOnlyAct = new TAction(this, "use_forced_subs_only", tr("&Forced subtitles only"), "forced_subs");
	useForcedSubsOnlyAct->setCheckable(true);
	useForcedSubsOnlyAct->setChecked(pref->use_forced_subs_only);
	connect(useForcedSubsOnlyAct, SIGNAL(triggered(bool)), core, SLOT(toggleForcedSubsOnly(bool)));

	addSeparator();
	loadSubsAct = new TAction(this, "load_subs", tr("&Load subtitles..."), "open");
    connect(loadSubsAct, SIGNAL(triggered()), main_window, SLOT(loadSub()));
	unloadSubsAct = new TAction(this, "unload_subs", tr("U&nload subtitles"), "unload");
	connect(unloadSubsAct, SIGNAL(triggered()), core, SLOT(unloadSub()));
    subFPSMenu = new TMenuSubFPS(main_window, core);
	addMenu(subFPSMenu);

	addSeparator();
	useCustomSubStyleAct = new TAction(this, "use_custom_sub_style", tr("Use custo&m style"));
	useCustomSubStyleAct->setCheckable(true);
	useCustomSubStyleAct->setChecked(pref->use_custom_ass_style);
	connect(useCustomSubStyleAct, SIGNAL(triggered(bool)), core, SLOT(changeUseCustomSubStyle(bool)));

#ifdef FIND_SUBTITLES
	addSeparator();
	showFindSubtitlesDialogAct = new TAction(this, "show_find_sub_dialog", tr("Find subtitles at &OpenSubtitles.org..."), "download_subs");
    connect(showFindSubtitlesDialogAct, SIGNAL(triggered()), main_window, SLOT(showFindSubtitlesDialog()));

	openUploadSubtitlesPageAct = new TAction(this, "upload_subtitles", tr("Upload su&btitles to OpenSubtitles.org..."), "upload_subs");
    connect(openUploadSubtitlesPageAct, SIGNAL(triggered()), main_window, SLOT(openUploadSubtitlesPage()));
#endif

    addActionsTo(main_window);
}

void TMenuSubtitle::enableActions(bool stopped, bool, bool) {

	bool e = !stopped
			 && (core->mdat.subs.count() > 0
				 || core->mset.closed_caption_channel > 0);
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

	nextSubtitleAct->setEnabled(e && core->mdat.subs.count() > 1);

	// useForcedSubsOnlyAct always enabled

	loadSubsAct->setEnabled(!stopped);
	unloadSubsAct->setEnabled(e && core->haveExternalSubs());

	// useCustomSubStyleAct always enabled

	// Depends on mset
	showFindSubtitlesDialogAct->setEnabled(!stopped);
	// openUploadSubtitlesPageAct always enabled
}

void TMenuSubtitle::onMediaSettingsChanged(Settings::TMediaSettings*) {
	// Already handled by updateSubtitles
}

void TMenuSubtitle::updateSubtitles() {
	qDebug("Gui::Action::TMenuSubtitle::updateSubtitles");

	// Note: use idx not ID
	subtitleTrackGroup->clear();
	secondarySubtitleTrackGroup->clear();

	QAction* subNoneAct = subtitleTrackGroup->addAction(tr("&None"));
	subNoneAct->setData(SubData::None);
	subNoneAct->setCheckable(true);
	if (core->mset.current_sub_idx < 0) {
		subNoneAct->setChecked(true);
	}
	subNoneAct = secondarySubtitleTrackGroup->addAction(tr("&None"));
	subNoneAct->setData(SubData::None);
	subNoneAct->setCheckable(true);
	if (core->mset.current_secondary_sub_idx < 0) {
		subNoneAct->setChecked(true);
	}

	for (int idx = 0; idx < core->mdat.subs.count(); idx++) {
		SubData sub = core->mdat.subs.itemAt(idx);
		QAction *a = new QAction(subtitleTrackGroup);
		a->setCheckable(true);
		a->setText(sub.displayName());
		a->setData(idx);
		if (idx == core->mset.current_sub_idx) {
			a->setChecked(true);
		}

		if (pref->isMPV()) {
			if (idx == core->mset.current_secondary_sub_idx) {
				a->setEnabled(false);
			}

			a = new QAction(secondarySubtitleTrackGroup);
			a->setCheckable(true);
			a->setText(sub.displayName());
			a->setData(idx);
			if (idx == core->mset.current_secondary_sub_idx) {
				a->setChecked(true);
			}
			if (idx == core->mset.current_sub_idx) {
				a->setEnabled(false);
			}
		}
	}

	subtitleTrackMenu->addActions(subtitleTrackGroup->actions());
	secondarySubtitleTrackMenu->addActions(secondarySubtitleTrackGroup->actions());

	// Enable actions
	enableActions(false, true, true);
	subFPSMenu->enableActions(false, true, true);
}

} // namespace Action
} // namespace Gui
