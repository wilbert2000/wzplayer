#include "gui/action/menusubtitle.h"
#include <QWidget>
#include "settings/mediasettings.h"
#include "gui/action/actiongroup.h"
#include "core.h"

using namespace Settings;

namespace Gui {
namespace Action {


class TMenuCC : public TMenu {
public:
	explicit TMenuCC(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool video, bool audio);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
	virtual void onAboutToShow();
private:
	TCore* core;
	TActionGroup* group;
};

TMenuCC::TMenuCC(QWidget *parent, TCore* c)
	: TMenu(parent, this, "closed_captions_menu", QT_TR_NOOP("&Closed captions"), "closed_caption")
	, core(c) {

	group = new TActionGroup(this, "cc");
	group->setEnabled(false);
	new TActionGroupItem(this, group, "cc_none", QT_TR_NOOP("&Off"), 0);
	new TActionGroupItem(this, group, "cc_ch_1", QT_TR_NOOP("&1"), 1);
	new TActionGroupItem(this, group, "cc_ch_2", QT_TR_NOOP("&2"), 2);
	new TActionGroupItem(this, group, "cc_ch_3", QT_TR_NOOP("&3"), 3);
	new TActionGroupItem(this, group, "cc_ch_4", QT_TR_NOOP("&4"), 4);
	group->setChecked(core->mset.closed_caption_channel);
	connect(group, SIGNAL(activated(int)), core, SLOT(changeClosedCaptionChannel(int)));
	// Currently no one else sets it
	addActionsTo(parent);
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


TMenuSubFPS::TMenuSubFPS(QWidget *parent, TCore* c)
	: TMenu(parent, this, "subfps_menu", QT_TR_NOOP("F&rames per second external subtitles"), "subfps")
	, core(c) {

	group = new TActionGroup(this, "subfps");
	group->setEnabled(false);
	new TActionGroupItem(this, group, "sub_fps_none", QT_TR_NOOP("&Default"), TMediaSettings::SFPS_None);
	new TActionGroupItem(this, group, "sub_fps_23976", QT_TR_NOOP("23.9&76"), TMediaSettings::SFPS_23976);
	new TActionGroupItem(this, group, "sub_fps_24", QT_TR_NOOP("2&4"), TMediaSettings::SFPS_24);
	new TActionGroupItem(this, group, "sub_fps_25", QT_TR_NOOP("2&5"), TMediaSettings::SFPS_25);
	new TActionGroupItem(this, group, "sub_fps_29970", QT_TR_NOOP("29.&970"), TMediaSettings::SFPS_29970);
	new TActionGroupItem(this, group, "sub_fps_30", QT_TR_NOOP("3&0"), TMediaSettings::SFPS_30);
	group->setChecked(core->mset.external_subtitles_fps);
	connect(group, SIGNAL(activated(int)), core, SLOT(changeExternalSubFPS(int)));
	// No one else sets it
	addActionsTo(parent);
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


TMenuSubtitle::TMenuSubtitle(QWidget* parent, TCore* c)
	: TMenu(parent, this, "subtitle_menu", QT_TR_NOOP("&Subtitles"), "noicon")
	, core(c) {

	decSubPosAct = new TAction(this, "dec_sub_pos", QT_TR_NOOP("&Up"), "sub_up", Qt::Key_R);
	connect(decSubPosAct, SIGNAL(triggered()), core, SLOT(decSubPos()));
	incSubPosAct = new TAction(this, "inc_sub_pos", QT_TR_NOOP("&Down"), "sub_down", Qt::Key_T);
	connect(incSubPosAct, SIGNAL(triggered()), core, SLOT(incSubPos()));

	addSeparator();
	decSubScaleAct = new TAction(this, "dec_sub_scale", QT_TR_NOOP("S&ize -"), "", Qt::SHIFT | Qt::Key_R);
	connect(decSubScaleAct, SIGNAL(triggered()), core, SLOT(decSubScale()));
	incSubScaleAct = new TAction(this, "inc_sub_scale", QT_TR_NOOP("Si&ze +"), "", Qt::SHIFT | Qt::Key_T);
	connect(incSubScaleAct, SIGNAL(triggered()), core, SLOT(incSubScale()));

	addSeparator();
	decSubDelayAct = new TAction(this, "dec_sub_delay", QT_TR_NOOP("Delay &-"), "delay_down", Qt::Key_Z);
	connect(decSubDelayAct, SIGNAL(triggered()), core, SLOT(decSubDelay()));
	incSubDelayAct = new TAction(this, "inc_sub_delay", QT_TR_NOOP("Delay &+"), "delay_up", Qt::Key_X);
	connect(incSubDelayAct, SIGNAL(triggered()), core, SLOT(incSubDelay()));
	subDelayAct = new TAction(this, "sub_delay", QT_TR_NOOP("Se&t delay..."), "sub_delay");
	connect(subDelayAct, SIGNAL(triggered()), parent, SLOT(showSubDelayDialog()));

	addSeparator();
	decSubStepAct = new TAction(this, "dec_sub_step", QT_TR_NOOP("&Previous line in subtitles"), "", Qt::Key_G);
	connect(decSubStepAct, SIGNAL(triggered()), core, SLOT(decSubStep()));
	incSubStepAct = new TAction(this, "inc_sub_step", QT_TR_NOOP("N&ext line in subtitles"), "", Qt::Key_Y);
	connect(incSubStepAct, SIGNAL(triggered()), core, SLOT(incSubStep()));

	seekNextSubAct = new TAction(this, "seek_next_sub", QT_TR_NOOP("Seek to next subtitle"),
								 "", Qt::CTRL | Qt::Key_Right, pref->isMPV());
	connect(seekNextSubAct, SIGNAL(triggered()), core, SLOT(seekToNextSub()));
	seekPrevSubAct = new TAction(this, "seek_prev_sub", QT_TR_NOOP("Seek to previous subtitle"),
								 "", Qt::CTRL | Qt::Key_Left, pref->isMPV());
	connect(seekPrevSubAct, SIGNAL(triggered()), core, SLOT(seekToPrevSub()));

	// Subtitle tracks
	addSeparator();
	subtitleTrackMenu = new TMenu(parent, this, "subtitlestrack_menu", QT_TR_NOOP("Subtitle &track"), "sub");
	nextSubtitleAct = new TAction(this, "next_subtitle", QT_TR_NOOP("Next subtitle"), "", Qt::Key_J, false);
	subtitleTrackMenu->addAction(nextSubtitleAct);
	subtitleTrackMenu->addSeparator();
	parent->addAction(nextSubtitleAct);
	addMenu(subtitleTrackMenu);
	connect(nextSubtitleAct, SIGNAL(triggered()), core, SLOT(nextSubtitle()));

	subtitleTrackGroup = new TActionGroup(this, "subtitletrack");
	connect(subtitleTrackGroup, SIGNAL(activated(int)), core, SLOT(changeSubtitle(int)));
	connect(core, SIGNAL(subtitlesChanged()), this, SLOT(updateSubtitles()));
	connect(core, SIGNAL(subtitleTrackChanged(int)), this, SLOT(updateSubtitles()));

	// Secondary subtitle track
	secondarySubtitleTrackMenu = new TMenu(parent, this, "secondary_subtitles_track_menu", QT_TR_NOOP("Secondary trac&k"), "secondary_sub");
	if (pref->isMPV())
		addMenu(secondarySubtitleTrackMenu);
	secondarySubtitleTrackGroup = new TActionGroup(this, "secondarysubtitletrack");
	connect(secondarySubtitleTrackGroup, SIGNAL(activated(int)), core, SLOT(changeSecondarySubtitle(int)));
	connect(core, SIGNAL(secondarySubtitleTrackChanged(int)), this, SLOT(updateSubtitles()));

	addMenu(new TMenuCC(parent, core));

	useForcedSubsOnlyAct = new TAction(this, "use_forced_subs_only", QT_TR_NOOP("&Forced subtitles only"), "forced_subs");
	useForcedSubsOnlyAct->setCheckable(true);
	useForcedSubsOnlyAct->setChecked(pref->use_forced_subs_only);
	connect(useForcedSubsOnlyAct, SIGNAL(triggered(bool)), core, SLOT(toggleForcedSubsOnly(bool)));

	addSeparator();
	loadSubsAct = new TAction(this, "load_subs", QT_TR_NOOP("&Load subtitles..."), "open");
	connect(loadSubsAct, SIGNAL(triggered()), parent, SLOT(loadSub()));
	unloadSubsAct = new TAction(this, "unload_subs", QT_TR_NOOP("U&nload subtitles"), "unload");
	connect(unloadSubsAct, SIGNAL(triggered()), core, SLOT(unloadSub()));
	subFPSMenu = new TMenuSubFPS(parent, core);
	addMenu(subFPSMenu);

	addSeparator();
	useCustomSubStyleAct = new TAction(this, "use_custom_sub_style", QT_TR_NOOP("Use custo&m style"));
	useCustomSubStyleAct->setCheckable(true);
	useCustomSubStyleAct->setChecked(pref->use_custom_ass_style);
	connect(useCustomSubStyleAct, SIGNAL(triggered(bool)), core, SLOT(changeUseCustomSubStyle(bool)));

#ifdef FIND_SUBTITLES
	addSeparator();
	showFindSubtitlesDialogAct = new TAction(this, "show_find_sub_dialog", QT_TR_NOOP("Find subtitles at &OpenSubtitles.org..."), "download_subs");
	connect(showFindSubtitlesDialogAct, SIGNAL(triggered()), parent, SLOT(showFindSubtitlesDialog()));

	openUploadSubtitlesPageAct = new TAction(this, "upload_subtitles", QT_TR_NOOP("Upload su&btitles to OpenSubtitles.org..."), "upload_subs");
	connect(openUploadSubtitlesPageAct, SIGNAL(triggered()), parent, SLOT(openUploadSubtitlesPage()));
#endif

	addActionsTo(parent);
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
