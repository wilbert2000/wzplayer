#include "gui/action/menus.h"
#include <QDebug>
#include "settings/preferences.h"
#include "gui/action/actiongroup.h"
#include "images.h"
#include "playerwindow.h"
#include "core.h"


using namespace Settings;

namespace Gui {

TAudioChannelMenu::TAudioChannelMenu(QWidget *parent, TCore* c)
	: QMenu(parent)
	, core(c) {

	menuAction()->setObjectName("audiochannels_menu");
	channelsGroup = new TActionGroup("channels", this);
	/* channelsDefaultAct = new TActionGroupItem(this, channelsGroup, "channels_default", TMediaSettings::ChDefault); */
	channelsStereoAct = new TActionGroupItem(this, channelsGroup, "channels_stereo", TMediaSettings::ChStereo);
	channelsSurroundAct = new TActionGroupItem(this, channelsGroup, "channels_surround", TMediaSettings::ChSurround);
	channelsFull51Act = new TActionGroupItem(this, channelsGroup, "channels_ful51", TMediaSettings::ChFull51);
	channelsFull61Act = new TActionGroupItem(this, channelsGroup, "channels_ful61", TMediaSettings::ChFull61);
	channelsFull71Act = new TActionGroupItem(this, channelsGroup, "channels_ful71", TMediaSettings::ChFull71);
	addActions(channelsGroup->actions());
	connect(channelsGroup, SIGNAL(activated(int)),
			core, SLOT(setAudioChannels(int)));

	connect(this, SIGNAL(aboutToShow()), this, SLOT(onAboutToShow()));
}

void TAudioChannelMenu::retranslateStrings() {

	menuAction()->setText(tr("&Channels"));
	menuAction()->setIcon(Images::icon("audio_channels"));

	/* channelsDefaultAct->change(tr("&Default")); */
	channelsStereoAct->change(tr("&Stereo"));
	channelsSurroundAct->change(tr("&4.0 Surround"));
	channelsFull51Act->change(tr("&5.1 Surround"));
	channelsFull61Act->change(tr("&6.1 Surround"));
	channelsFull71Act->change(tr("&7.1 Surround"));
}

void TAudioChannelMenu::onAboutToShow() {
	channelsGroup->setChecked(core->mset.audio_use_channels);
}


TCCMenu::TCCMenu(QWidget *parent, TCore* c)
	: QMenu(parent)
	, core(c) {

	menuAction()->setObjectName("closed_captions_menu");
	ccGroup = new TActionGroup("cc", this);
	ccNoneAct = new TActionGroupItem(this, ccGroup, "cc_none", 0);
	ccChannel1Act = new TActionGroupItem(this, ccGroup, "cc_ch_1", 1);
	ccChannel2Act = new TActionGroupItem(this, ccGroup, "cc_ch_2", 2);
	ccChannel3Act = new TActionGroupItem(this, ccGroup, "cc_ch_3", 3);
	ccChannel4Act = new TActionGroupItem(this, ccGroup, "cc_ch_4", 4);
	addActions(ccGroup->actions());
	connect(ccGroup, SIGNAL(activated(int)),
			core, SLOT(changeClosedCaptionChannel(int)));

	connect(this, SIGNAL(aboutToShow()), this, SLOT(onAboutToShow()));
}

void TCCMenu::retranslateStrings() {

	menuAction()->setText(tr("&Closed captions"));
	menuAction()->setIcon(Images::icon("closed_caption"));

	ccNoneAct->change(tr("&Off", "closed captions menu"));
	ccChannel1Act->change("&1");
	ccChannel2Act->change("&2");
	ccChannel3Act->change("&3");
	ccChannel4Act->change("&4");
}

void TCCMenu::onAboutToShow() {
	ccGroup->setChecked(core->mset.closed_caption_channel);
}


TSubFPSMenu::TSubFPSMenu(QWidget *parent, TCore* c)
	: QMenu(parent)
	, core(c) {

	menuAction()->setObjectName("subfps_menu");
	subFPSGroup = new TActionGroup("subfps", this);
	subFPSNoneAct = new TActionGroupItem(this, subFPSGroup, "sub_fps_none", TMediaSettings::SFPS_None);
	/* subFPS23Act = new TActionGroupItem(this, subFPSGroup, "sub_fps_23", TMediaSettings::SFPS_23); */
	subFPS23976Act = new TActionGroupItem(this, subFPSGroup, "sub_fps_23976", TMediaSettings::SFPS_23976);
	subFPS24Act = new TActionGroupItem(this, subFPSGroup, "sub_fps_24", TMediaSettings::SFPS_24);
	subFPS25Act = new TActionGroupItem(this, subFPSGroup, "sub_fps_25", TMediaSettings::SFPS_25);
	subFPS29970Act = new TActionGroupItem(this, subFPSGroup, "sub_fps_29970", TMediaSettings::SFPS_29970);
	subFPS30Act = new TActionGroupItem(this, subFPSGroup, "sub_fps_30", TMediaSettings::SFPS_30);
	addActions(subFPSGroup->actions());
	connect(subFPSGroup, SIGNAL(activated(int)),
			core, SLOT(changeExternalSubFPS(int)));

	connect(this, SIGNAL(aboutToShow()), this, SLOT(onAboutToShow()));
}

void TSubFPSMenu::retranslateStrings() {

	menuAction()->setText(tr("F&rames per second"));
	menuAction()->setIcon(Images::icon("subfps"));

	subFPSNoneAct->change(tr("&Default", "subfps menu"));
	/* subFPS23Act->change("2&3"); */
	subFPS23976Act->change("23.9&76");
	subFPS24Act->change("2&4");
	subFPS25Act->change("2&5");
	subFPS29970Act->change("29.&970");
	subFPS30Act->change("3&0");
}

void TSubFPSMenu::onAboutToShow() {
	subFPSGroup->setEnabled(core->haveExternalSubs());
	subFPSGroup->setChecked(core->mset.external_subtitles_fps);
}


TOnTopMenu::TOnTopMenu(QWidget *parent) :
	QMenu(parent) {

	menuAction()->setObjectName("ontop_menu");

	onTopActionGroup = new TActionGroup("ontop", this);
	onTopAlwaysAct = new TActionGroupItem(this, onTopActionGroup, "on_top_always", TPreferences::AlwaysOnTop);
	onTopNeverAct = new TActionGroupItem(this, onTopActionGroup, "on_top_never", TPreferences::NeverOnTop);
	onTopWhilePlayingAct = new TActionGroupItem(this, onTopActionGroup, "on_top_playing", TPreferences::WhilePlayingOnTop);
	addActions(onTopActionGroup->actions());
	connect(onTopActionGroup , SIGNAL(activated(int)), parent, SLOT(changeStayOnTop(int)));

	toggleStayOnTopAct = new TAction(this, "toggle_stay_on_top");
	connect(toggleStayOnTopAct, SIGNAL(triggered()), parent, SLOT(toggleStayOnTop()));

	connect(this, SIGNAL(aboutToShow()), this, SLOT(onAboutToShow()));
}

void TOnTopMenu::retranslateStrings() {

	menuAction()->setText(tr("S&tay on top"));
	menuAction()->setIcon(Images::icon("ontop"));

	onTopAlwaysAct->change(tr("&Always"));
	onTopNeverAct->change(tr("&Never"));
	onTopWhilePlayingAct->change(tr("While &playing"));

	toggleStayOnTopAct->change(tr("Toggle stay on top"));
}

void TOnTopMenu::onAboutToShow() {
	onTopActionGroup->setChecked((int) pref->stay_on_top);
}


TVideoSizeGroup::TVideoSizeGroup(QWidget* parent, TPlayerWindow* pw)
	: TActionGroup("size", parent)
	, playerWindow(pw) {

	setEnabled(false);

	TActionGroupItem* a;
	a = new TActionGroupItem(this, this, "5&0%", "size_50", 50);
	a = new TActionGroupItem(this, this, "7&5%", "size_75", 75);
	a = new TActionGroupItem(this, this, "&100%", "size_100", 100);
	a->setShortcut(Qt::CTRL | Qt::Key_1);
	a = new TActionGroupItem(this, this, "1&25%", "size_125", 125);
	a = new TActionGroupItem(this, this, "15&0%", "size_150", 150);
	a = new TActionGroupItem(this, this, "1&75%", "size_175", 175);
	a = new TActionGroupItem(this, this, "&200%", "size_200", 200);
	a->setShortcut(Qt::CTRL | Qt::Key_2);
	a = new TActionGroupItem(this, this, "&300%", "size_300", 300);
	a = new TActionGroupItem(this, this, "&400%", "size_400", 400);
}

void TVideoSizeGroup::uncheck() {

	QAction* current = checkedAction();
	if (current)
		current->setChecked(false);
}

void TVideoSizeGroup::enableVideoSizeGroup(bool on) {

	QSize s = playerWindow->resolution();
	setEnabled(on && !pref->fullscreen && s.width() > 0 && s.height() > 0);
}

void TVideoSizeGroup::updateVideoSizeGroup() {
	// qDebug("Gui::TVideoSizeGroup::updateVideoSizeGroup");

	uncheck();
	QSize s = playerWindow->resolution();
	if (pref->fullscreen || s.width() <= 0 || s.height() <= 0) {
		setEnabled(false);
	} else {
		setEnabled(true);

		// Update size factor
		QSize video_size = playerWindow->getAdjustedSize(s.width(), s.height(), 1.0);
		int factor_x = qRound((double) playerWindow->width() * 100 / video_size.width());
		int factor_y = qRound((double) playerWindow->height() * 100/ video_size.height());

		// Set when x and y factor agree
		if (factor_x == factor_y) {
			setChecked(factor_x);
		}
	}
}


TVideoSizeMenu::TVideoSizeMenu(QWidget* parent, TPlayerWindow* pw)
	: QMenu(parent) {

	menuAction()->setObjectName("videosize_menu");
	sizeGroup = new TVideoSizeGroup(this, pw);
	addActions(sizeGroup->actions());
	connect(sizeGroup, SIGNAL(activated(int)), parent, SLOT(changeSize(int)));

	addSeparator();
	doubleSizeAct = new TAction(Qt::CTRL | Qt::Key_D, this, "toggle_double_size");
	addAction(doubleSizeAct);
	connect(doubleSizeAct, SIGNAL(triggered()), parent, SLOT(toggleDoubleSize()));

	connect(this, SIGNAL(aboutToShow()), this, SLOT(onAboutToShow()));
}

void TVideoSizeMenu::retranslateStrings() {

	menuAction()->setText(tr("Si&ze"));
	menuAction()->setIcon(Images::icon("video_size"));
	doubleSizeAct->change(tr("&Toggle double size"));
}

void TVideoSizeMenu::enableVideoSize(bool on) {

	sizeGroup->enableVideoSizeGroup(on);
	doubleSizeAct->setEnabled(sizeGroup->isEnabled());
}

void TVideoSizeMenu::onAboutToShow() {
	//qDebug("TVideoSizeMenu::onAboutToShow");

	sizeGroup->updateVideoSizeGroup();
	doubleSizeAct->setEnabled(sizeGroup->isEnabled());
}

} // namespace Gui

