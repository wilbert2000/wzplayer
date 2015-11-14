#include "gui/action/menus.h"
#include <QDebug>
#include "settings/preferences.h"
#include "gui/action/actiongroup.h"
#include "desktop.h"
#include "images.h"
#include "playerwindow.h"
#include "core.h"


using namespace Settings;

namespace Gui {

void execPopup(QWidget* w, QMenu* popup, QPoint p) {
	//qDebug() << "Gui::execPopup:" << p << popup->size();

	// Evade mouse
	// TODO: size popup not yet valid. Use abotToShow()?
	QSize s(popup->size());
	QSize desktop = TDesktop::size(w);
	if (p.x() < 0) p.rx() = 0;
	else if (p.x() + s.width() > desktop.width()) {
		p.rx() = desktop.width() - s.width();
	}
	if (p.y() < 0) p.ry() = 0;
	else if (p.y() + s.height() > desktop.height()) {
		p.ry() = desktop.height() - s.height();
	}
	//qDebug() << "Gui::execPopup:" << p;

	if (QCursor::pos().x() > p.x() && QCursor::pos().x() < p.x() + s.width()) {
		if (QCursor::pos().x() >= desktop.width() - s.width()) {
			// Place menu to the left of mouse
			p.rx() = QCursor::pos().x() - s.width();
		} else {
			// Place menu to the right of mouse
			p.rx() = QCursor::pos().x();
		}
	}
	if (QCursor::pos().y() > p.y() && QCursor::pos().y() < p.y() + s.height()) {
		if (QCursor::pos().y() >= desktop.height() - s.height()) {
			// Place menu above mouse
			p.ry() = QCursor::pos().y() - s.height();
		} else {
			// Place menu below mouse
			p.ry() = QCursor::pos().y();
		}
	}
	//qDebug() << "Gui::execPopup:" << p;

	// Popup exec keeps menu inside screen too
	popup->exec(p);
}

TMenu::TMenu(QWidget* parent,
			 QObject* aTranslator,
			 const QString& name,
			 const QString& text,
			 const QString& icon)
	: QMenu(parent)
	, text_en(text)
	, translator(aTranslator) {

	menuAction()->setObjectName(name);
	menuAction()->setIcon(Images::icon(icon));
	connect(this, SIGNAL(aboutToShow()), this, SLOT(onAboutToShow()));
	retranslateStrings();
}

void TMenu::retranslateStrings() {
	menuAction()->setText(translator->tr(text_en.toUtf8().constData()));
}

void TMenu::changeEvent(QEvent* e) {

	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	} else {
		QMenu::changeEvent(e);
	}
}

void TMenu::onAboutToShow() {
}

void TMenu::addActionsTo(QWidget* w) {

	w->addAction(menuAction());

	QList<QAction*> acts = actions();
	for(int i = 0; i < acts.count(); i++) {
		QAction* a = acts[i];
		if (!a->isSeparator()) {
			w->addAction(a);
		}
	}
}

TAspectMenu::TAspectMenu(QWidget* parent, TCore* c)
	: TMenu(parent, this, "aspect_menu", QT_TR_NOOP("&Aspect ratio"), "aspect")
	, core(c) {

	group = new TActionGroup(this, "aspect");
	new TActionGroupItem(this, group, "aspect_detect", QT_TR_NOOP("&Auto"), TMediaSettings::AspectAuto);
	new TActionGroupItem(this, group, "aspect_1_1", QT_TR_NOOP("1&:1"), TMediaSettings::Aspect11);
	new TActionGroupItem(this, group, "aspect_5_4", QT_TR_NOOP("&5:4"), TMediaSettings::Aspect54);
	new TActionGroupItem(this, group, "aspect_4_3", QT_TR_NOOP("&4:3"), TMediaSettings::Aspect43);
	new TActionGroupItem(this, group, "aspect_11_8", QT_TR_NOOP("11:&8"), TMediaSettings::Aspect118);
	new TActionGroupItem(this, group, "aspect_14_10", QT_TR_NOOP("1&4:10"), TMediaSettings::Aspect1410);
	new TActionGroupItem(this, group, "aspect_3_2", QT_TR_NOOP("&3:2"), TMediaSettings::Aspect32);
	new TActionGroupItem(this, group, "aspect_14_9", QT_TR_NOOP("&14:9"), TMediaSettings::Aspect149);
	new TActionGroupItem(this, group, "aspect_16_10", QT_TR_NOOP("1&6:10"), TMediaSettings::Aspect1610);
	new TActionGroupItem(this, group, "aspect_16_9", QT_TR_NOOP("16:&9"), TMediaSettings::Aspect169);
	new TActionGroupItem(this, group, "aspect_2.35_1", QT_TR_NOOP("&2.35:1"), TMediaSettings::Aspect235);
	addSeparator();
	new TActionGroupItem(this, group, "aspect_none", QT_TR_NOOP("&Disabled"), TMediaSettings::AspectNone);

	connect(group, SIGNAL(activated(int)), core, SLOT(changeAspectRatio(int)));
	connect(core, SIGNAL(aspectRatioChanged(int)), group, SLOT(setCheckedSlot(int)));
	addActionsTo(parent);
}

void TAspectMenu::onAboutToShow() {
	group->setChecked(core->mset.aspect_ratio_id);
}


TAudioChannelMenu::TAudioChannelMenu(QWidget *parent, TCore* c)
	: TMenu(parent, this, "audiochannels_menu", QT_TR_NOOP("&Channels"), "audio_channels")
	, core(c) {

	group = new TActionGroup(this, "channels");
	new TActionGroupItem(this, group, "channels_stereo", QT_TR_NOOP("&Stereo"), TMediaSettings::ChStereo);
	new TActionGroupItem(this, group, "channels_surround", QT_TR_NOOP("&4.0 Surround"), TMediaSettings::ChSurround);
	new TActionGroupItem(this, group, "channels_ful51", QT_TR_NOOP("&5.1 Surround"), TMediaSettings::ChFull51);
	new TActionGroupItem(this, group, "channels_ful61", QT_TR_NOOP("&6.1 Surround"), TMediaSettings::ChFull61);
	new TActionGroupItem(this, group, "channels_ful71", QT_TR_NOOP("&7.1 Surround"), TMediaSettings::ChFull71);

	connect(group, SIGNAL(activated(int)), core, SLOT(setAudioChannels(int)));
	addActionsTo(parent);
}

void TAudioChannelMenu::onAboutToShow() {
	group->setEnabled(core->state() != TCore::Stopped && core->mdat.audios.count() > 0);
	group->setChecked(core->mset.audio_use_channels);
}


TCCMenu::TCCMenu(QWidget *parent, TCore* c)
	: TMenu(parent, this, "closed_captions_menu", QT_TR_NOOP("&Closed captions"), "closed_caption")
	, core(c) {

	group = new TActionGroup(this, "cc");
	new TActionGroupItem(this, group, "cc_none", QT_TR_NOOP("&Off"), 0);
	new TActionGroupItem(this, group, "cc_ch_1", QT_TR_NOOP("&1"), 1);
	new TActionGroupItem(this, group, "cc_ch_2", QT_TR_NOOP("&2"), 2);
	new TActionGroupItem(this, group, "cc_ch_3", QT_TR_NOOP("&3"), 3);
	new TActionGroupItem(this, group, "cc_ch_4", QT_TR_NOOP("&4"), 4);

	connect(group, SIGNAL(activated(int)), core, SLOT(changeClosedCaptionChannel(int)));
	addActionsTo(parent);
}

void TCCMenu::onAboutToShow() {
	group->setChecked(core->mset.closed_caption_channel);
}


TDeinterlaceMenu::TDeinterlaceMenu(QWidget* parent, TCore* c)
	: TMenu(parent, this, "deinterlace_menu", QT_TR_NOOP("&Deinterlace"), "deinterlace")
	, core(c) {

	group = new TActionGroup(this, "deinterlace");
	new TActionGroupItem(this, group, "deinterlace_none", QT_TR_NOOP("&None"), TMediaSettings::NoDeinterlace);
	new TActionGroupItem(this, group, "deinterlace_l5", QT_TR_NOOP("&Lowpass5"), TMediaSettings::L5);
	new TActionGroupItem(this, group, "deinterlace_yadif0", QT_TR_NOOP("&Yadif (normal)"), TMediaSettings::Yadif);
	new TActionGroupItem(this, group, "deinterlace_yadif1", QT_TR_NOOP("Y&adif (double framerate)"), TMediaSettings::Yadif_1);
	new TActionGroupItem(this, group, "deinterlace_lb", QT_TR_NOOP("Linear &Blend"), TMediaSettings::LB);
	new TActionGroupItem(this, group, "deinterlace_kern", QT_TR_NOOP("&Kerndeint"), TMediaSettings::Kerndeint);

	connect(group, SIGNAL(activated(int)), core, SLOT(changeDeinterlace(int)));
	addActionsTo(parent);
}

void TDeinterlaceMenu::onAboutToShow() {
	group->setChecked(core->mset.current_deinterlacer);
}


TDiscMenu::TDiscMenu(QWidget* parent)
	: TMenu(parent, this, "disc_menu", QT_TR_NOOP("&Disc"), "open_disc") {

	// DVD
	TAction* a = new TAction(this, "open_dvd", QT_TR_NOOP("&DVD from drive"), "dvd");
	connect(a, SIGNAL(triggered()), parent, SLOT(openDVD()));
	a = new TAction(this, "open_dvd_folder", QT_TR_NOOP("D&VD from folder..."), "dvd_hd");
	connect(a, SIGNAL(triggered()), parent, SLOT(openDVDFromFolder()));
	// BluRay
	a = new TAction(this, "open_bluray", QT_TR_NOOP("&Blu-ray from drive"), "bluray");
	connect(a, SIGNAL(triggered()), parent, SLOT(openBluRay()));
	a = new TAction(this, "open_bluray_folder", QT_TR_NOOP("Blu-&ray from folder..."), "bluray_hd");
	connect(a, SIGNAL(triggered()), parent, SLOT(openBluRayFromFolder()));
	// VCD and audio
	a = new TAction(this, "open_vcd", QT_TR_NOOP("V&CD"), "vcd");
	connect(a, SIGNAL(triggered()), parent, SLOT(openVCD()));
	a = new TAction(this, "open_audio_cd", QT_TR_NOOP("&Audio CD"), "cdda");
	connect(a, SIGNAL(triggered()), parent, SLOT(openAudioCD()));

	addActionsTo(parent);
}


TOSDMenu::TOSDMenu(QWidget *parent, TCore* c)
	: TMenu(parent, this, "osd_menu", QT_TR_NOOP("&OSD"), "osd")
	, core(c) {

	group = new TActionGroup(this, "osd");
	new TActionGroupItem(this, group, "osd_none", QT_TR_NOOP("Subtitles onl&y"), Settings::TPreferences::None);
	new TActionGroupItem(this, group, "osd_seek", QT_TR_NOOP("Volume + &Seek"), Settings::TPreferences::Seek);
	new TActionGroupItem(this, group, "osd_timer", QT_TR_NOOP("Volume + Seek + &Timer"), Settings::TPreferences::SeekTimer);
	new TActionGroupItem(this, group, "osd_total", QT_TR_NOOP("Volume + Seek + Timer + T&otal time"), Settings::TPreferences::SeekTimerTotal);
	connect(group, SIGNAL(activated(int)), core, SLOT(changeOSDLevel(int)));

	addSeparator();
	TAction* a = new TAction(this, "inc_osd_scale", QT_TR_NOOP("Size &+"), "", Qt::SHIFT | Qt::Key_U);
	connect(a, SIGNAL(triggered()), core, SLOT(incOSDScale()));
	a = new TAction(this, "dec_osd_scale", QT_TR_NOOP("Size &-"), "", Qt::SHIFT | Qt::Key_Y);
	connect(a, SIGNAL(triggered()), core, SLOT(decOSDScale()));

	addActionsTo(parent);
}

void TOSDMenu::onAboutToShow() {
	// Always enabled
	group->setChecked((int) pref->osd_level);
}


TPlaySpeedMenu::TPlaySpeedMenu(QWidget *parent, TCore *c)
	: TMenu(parent, this, "speed_menu", QT_TR_NOOP("Sp&eed"), "speed")
	, core(c) {

	group = new QActionGroup(this);
	group->setExclusive(false);
	group->setEnabled(false);

	TAction* a = new TAction(this, "normal_speed", QT_TR_NOOP("&Normal speed"), "", Qt::Key_Backspace);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(normalSpeed()));

	addSeparator();
	a = new TAction(this, "halve_speed", QT_TR_NOOP("&Half speed"), "", Qt::Key_BraceLeft);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(halveSpeed()));
	a = new TAction(this, "double_speed", QT_TR_NOOP("&Double speed"), "", Qt::Key_BraceRight);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(doubleSpeed()));

	addSeparator();
	a = new TAction(this, "dec_speed", QT_TR_NOOP("Speed &-10%"), "", Qt::Key_BracketLeft);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(decSpeed10()));
	a = new TAction(this, "inc_speed", QT_TR_NOOP("Speed &+10%"), "", Qt::Key_BracketRight);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(incSpeed10()));

	addSeparator();
	a = new TAction(this, "dec_speed_4", QT_TR_NOOP("Speed -&4%"), "");
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(decSpeed4()));
	a = new TAction(this, "inc_speed_4", QT_TR_NOOP("&Speed +4%"), "");
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(incSpeed4()));

	addSeparator();
	a = new TAction(this, "dec_speed_1", QT_TR_NOOP("Speed -&1%"), "");
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(decSpeed1()));
	a = new TAction(this, "inc_speed_1", QT_TR_NOOP("S&peed +1%"), "");
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(incSpeed1()));

	addActionsTo(parent);
}


TRotateMenu::TRotateMenu(QWidget* parent, TCore* c)
	: TMenu(parent, this, "rotate_menu", QT_TR_NOOP("&Rotate"), "rotate")
	, core(c) {

	group = new TActionGroup(this, "rotate");
	new TActionGroupItem(this, group, "rotate_none", QT_TR_NOOP("&Off"), TMediaSettings::NoRotate);
	new TActionGroupItem(this, group, "rotate_clockwise_flip", QT_TR_NOOP("&Rotate by 90 degrees clockwise and flip"), TMediaSettings::Clockwise_flip);
	new TActionGroupItem(this, group, "rotate_clockwise", QT_TR_NOOP("Rotate by 90 degrees &clockwise"), TMediaSettings::Clockwise);
	new TActionGroupItem(this, group, "rotate_counterclockwise", QT_TR_NOOP("Rotate by 90 degrees counterclock&wise"), TMediaSettings::Counterclockwise);
	new TActionGroupItem(this, group, "rotate_counterclockwise_flip", QT_TR_NOOP("Rotate by 90 degrees counterclockwise and &flip"), TMediaSettings::Counterclockwise_flip);
	connect(group, SIGNAL(activated(int)), core, SLOT(changeRotate(int)));
	addActionsTo(parent);
}

void TRotateMenu::onAboutToShow() {
	group->setChecked(core->mset.rotate);
}


TStayOnTopMenu::TStayOnTopMenu(QWidget *parent) :
	// TODO: rename to stay_on_top_menu?
	TMenu(parent, this, "ontop_menu", QT_TR_NOOP("&Stay on top"), "ontop") {

	group = new TActionGroup(this, "ontop");
	new TActionGroupItem(this, group, "on_top_always", QT_TR_NOOP("&Always"), TPreferences::AlwaysOnTop);
	new TActionGroupItem(this, group, "on_top_never", QT_TR_NOOP("&Never"), TPreferences::NeverOnTop);
	new TActionGroupItem(this, group, "on_top_playing", QT_TR_NOOP("While &playing"), TPreferences::WhilePlayingOnTop);
	connect(group , SIGNAL(activated(int)), parent, SLOT(changeStayOnTop(int)));

	addSeparator();
	TAction* a = new TAction(this, "toggle_stay_on_top", QT_TR_NOOP("Toggle stay on top"), "");
	connect(a, SIGNAL(triggered()), parent, SLOT(toggleStayOnTop()));

	addActionsTo(parent);
}

void TStayOnTopMenu::onAboutToShow() {
	group->setChecked((int) pref->stay_on_top);
}


TStereoMenu::TStereoMenu(QWidget *parent, TCore* c)
	: TMenu(parent, this, "stereomode_menu", QT_TR_NOOP("&Stereo mode"), "stereo_mode")
	, core(c) {

	group = new TActionGroup(this, "stereo");
	new TActionGroupItem(this, group, "stereo", QT_TR_NOOP("&Stereo"), TMediaSettings::Stereo);
	new TActionGroupItem(this, group, "left_channel", QT_TR_NOOP("&Left channel"), TMediaSettings::Left);
	new TActionGroupItem(this, group, "right_channel", QT_TR_NOOP("&Right channel"), TMediaSettings::Right);
	new TActionGroupItem(this, group, "mono", QT_TR_NOOP("&Mono"), TMediaSettings::Mono);
	new TActionGroupItem(this, group, "reverse_channels", QT_TR_NOOP("Re&verse"), TMediaSettings::Reverse);
	connect(group, SIGNAL(activated(int)), core, SLOT(setStereoMode(int)));
	addActionsTo(parent);
}

void TStereoMenu::onAboutToShow() {
	group->setEnabled(core->state() != TCore::Stopped && core->mdat.audios.count() > 0);
	group->setChecked(core->mset.stereo_mode);
}


TSubFPSMenu::TSubFPSMenu(QWidget *parent, TCore* c)
	: TMenu(parent, this, "subfps_menu", QT_TR_NOOP("F&rames per second"), "subfps")
	, core(c) {

	group = new TActionGroup(this, "subfps");
	new TActionGroupItem(this, group, "sub_fps_none", QT_TR_NOOP("&Default"), TMediaSettings::SFPS_None);
	new TActionGroupItem(this, group, "sub_fps_23976", QT_TR_NOOP("23.9&76"), TMediaSettings::SFPS_23976);
	new TActionGroupItem(this, group, "sub_fps_24", QT_TR_NOOP("2&4"), TMediaSettings::SFPS_24);
	new TActionGroupItem(this, group, "sub_fps_25", QT_TR_NOOP("2&5"), TMediaSettings::SFPS_25);
	new TActionGroupItem(this, group, "sub_fps_29970", QT_TR_NOOP("29.&970"), TMediaSettings::SFPS_29970);
	new TActionGroupItem(this, group, "sub_fps_30", QT_TR_NOOP("3&0"), TMediaSettings::SFPS_30);
	connect(group, SIGNAL(activated(int)), core, SLOT(changeExternalSubFPS(int)));
	addActionsTo(parent);
}

void TSubFPSMenu::onAboutToShow() {
	group->setEnabled(core->haveExternalSubs());
	group->setChecked(core->mset.external_subtitles_fps);
}


TVideoFilterMenu::TVideoFilterMenu(QWidget *parent, TCore *c)
	: TMenu(parent, this, "videofilter_menu", QT_TR_NOOP("F&ilters"), "video_filters")
	, core(c) {

	group = new QActionGroup(this);
	group->setExclusive(false);

	postProcessingAct = new TAction(this, "postprocessing", QT_TR_NOOP("&Postprocessing"));
	postProcessingAct->setCheckable(true);
	group->addAction(postProcessingAct);
	connect(postProcessingAct, SIGNAL(toggled(bool)), core, SLOT(togglePostprocessing(bool)));

	deblockAct = new TAction(this, "deblock", QT_TR_NOOP("&Deblock"));
	deblockAct->setCheckable(true);
	group->addAction(deblockAct);
	connect(deblockAct, SIGNAL(toggled(bool)), core, SLOT(toggleDeblock(bool)));

	deringAct = new TAction(this, "dering", QT_TR_NOOP("De&ring"));
	deringAct->setCheckable(true);
	group->addAction(deringAct);
	connect(deringAct, SIGNAL(toggled(bool)), core, SLOT(toggleDering(bool)));

	gradfunAct = new TAction(this, "gradfun", QT_TR_NOOP("Debanding (&gradfun)"));
	gradfunAct->setCheckable(true);
	group->addAction(gradfunAct);
	connect(gradfunAct, SIGNAL(toggled(bool)), core, SLOT(toggleGradfun(bool)));

	addNoiseAct = new TAction(this, "add_noise", QT_TR_NOOP("Add n&oise"));
	addNoiseAct->setCheckable(true);
	group->addAction(addNoiseAct);
	connect(addNoiseAct, SIGNAL(toggled(bool)), core, SLOT(toggleNoise(bool)));

	addLetterboxAct = new TAction(this, "add_letterbox", QT_TR_NOOP("Add &black borders"), "letterbox");
	addLetterboxAct->setCheckable(true);
	group->addAction(addLetterboxAct);
	connect(addLetterboxAct, SIGNAL(toggled(bool)), core, SLOT(changeLetterbox(bool)));

	upscaleAct = new TAction(this, "upscaling", QT_TR_NOOP("Soft&ware scaling"));
	upscaleAct->setCheckable(true);
	group->addAction(upscaleAct);
	connect(upscaleAct, SIGNAL(toggled(bool)), core, SLOT(changeUpscale(bool)));

	phaseAct = new TAction(this, "autodetect_phase", QT_TR_NOOP("&Autodetect phase"));
	phaseAct->setCheckable(true);
	group->addAction(phaseAct);
	connect(phaseAct, SIGNAL(toggled(bool)), core, SLOT(toggleAutophase(bool)));

	// Denoise
	TMenu* menu = new TMenu(this, this, "denoise_menu", QT_TR_NOOP("De&noise"), "denoise");
	denoiseGroup = new TActionGroup(this, "denoise");
	denoiseNoneAct = new TActionGroupItem(this, denoiseGroup, "denoise_none", QT_TR_NOOP("&Off"), TMediaSettings::NoDenoise, false);
	denoiseNormalAct = new TActionGroupItem(this, denoiseGroup, "denoise_normal", QT_TR_NOOP("&Normal"), TMediaSettings::DenoiseNormal, false);
	denoiseSoftAct = new TActionGroupItem(this, denoiseGroup, "denoise_soft", QT_TR_NOOP("&Soft"), TMediaSettings::DenoiseSoft, false);
	menu->addActions(denoiseGroup->actions());
	menu->addActionsTo(parent);
	addMenu(menu);
	connect(denoiseGroup, SIGNAL(activated(int)), core, SLOT(changeDenoise(int)));
	connect(menu, SIGNAL(aboutToShow()), this, SLOT(onAboutToShowDenoise()));

	// Unsharp group
	menu = new TMenu(this, this, "unsharp_menu", QT_TR_NOOP("Blur/S&harp"), "unsharp");
	unsharpGroup = new TActionGroup(this, "unsharp");
	unsharpNoneAct = new TActionGroupItem(this, unsharpGroup, "unsharp_off", QT_TR_NOOP("&None"), 0, false);
	blurAct = new TActionGroupItem(this, unsharpGroup, "blur", QT_TR_NOOP("&Blur"), 1, false);
	sharpenAct = new TActionGroupItem(this, unsharpGroup, "sharpen", QT_TR_NOOP("&Sharpen"), 2, false);
	menu->addActions(unsharpGroup->actions());
	menu->addActionsTo(parent);
	addMenu(menu);
	connect(unsharpGroup, SIGNAL(activated(int)), core, SLOT(changeUnsharp(int)));
	connect(menu, SIGNAL(aboutToShow()), this, SLOT(onAboutToShowUnSharp()));
}

void TVideoFilterMenu::setEnabledX(bool enable) {

	enable = enable && !core->mdat.noVideo();
	group->setEnabled(enable);
	denoiseGroup->setEnabled(enable);
	unsharpGroup->setEnabled(enable);
}

void TVideoFilterMenu::updateFilters() {

	postProcessingAct->setChecked(core->mset.postprocessing_filter);
	deblockAct->setChecked(core->mset.deblock_filter);
	deringAct->setChecked(core->mset.dering_filter);
	gradfunAct->setChecked(core->mset.gradfun_filter);
	addNoiseAct->setChecked(core->mset.noise_filter);
	addLetterboxAct->setChecked(core->mset.add_letterbox);
	upscaleAct->setChecked(core->mset.upscaling_filter);
	phaseAct->setChecked(core->mset.phase_filter);
}

void TVideoFilterMenu::onAboutToShow() {
	updateFilters();
}

void TVideoFilterMenu::onAboutToShowDenoise() {
	denoiseGroup->setChecked(core->mset.current_denoiser);
}

void TVideoFilterMenu::onAboutToShowUnSharp() {
	unsharpGroup->setChecked(core->mset.current_unsharp);
}

TVideoSizeGroup::TVideoSizeGroup(QWidget* parent, TPlayerWindow* pw)
	: TActionGroup(parent, "size")
	, playerWindow(pw) {

	setEnabled(false);

	TActionGroupItem* a;
	new TActionGroupItem(this, this, "size_50", QT_TR_NOOP("5&0%"), 50, false);
	new TActionGroupItem(this, this, "size_75", QT_TR_NOOP("7&5%"), 75, false);
	a = new TActionGroupItem(this, this, "size_100", QT_TR_NOOP("&100%"), 100, false);
	a->setShortcut(Qt::CTRL | Qt::Key_1);
	new TActionGroupItem(this, this, "size_125", QT_TR_NOOP("1&25%"), 125, false);
	new TActionGroupItem(this, this, "size_150", QT_TR_NOOP("15&0%"), 150, false);
	new TActionGroupItem(this, this, "size_175", QT_TR_NOOP("1&75%"), 175, false);
	a = new TActionGroupItem(this, this, "size_200", QT_TR_NOOP("&200%"), 200, false);
	a->setShortcut(Qt::CTRL | Qt::Key_2);
	new TActionGroupItem(this, this, "size_300", QT_TR_NOOP("&300%"), 300, false);
	new TActionGroupItem(this, this, "size_400", QT_TR_NOOP("&400%"), 400, false);
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
	: TMenu(parent, this, "videosize_menu", QT_TR_NOOP("&Size"), "video_size") {

	group = new TVideoSizeGroup(this, pw);
	addActions(group->actions());
	connect(group, SIGNAL(activated(int)), parent, SLOT(changeSize(int)));

	addSeparator();
	doubleSizeAct = new TAction(this, "toggle_double_size", QT_TR_NOOP("&Toggle double size"), "", Qt::CTRL | Qt::Key_D);
	connect(doubleSizeAct, SIGNAL(triggered()), parent, SLOT(toggleDoubleSize()));

	addActionsTo(parent);
}

void TVideoSizeMenu::enableVideoSize(bool on) {

	group->enableVideoSizeGroup(on);
	doubleSizeAct->setEnabled(group->isEnabled());
}

void TVideoSizeMenu::onAboutToShow() {

	group->updateVideoSizeGroup();
	doubleSizeAct->setEnabled(group->isEnabled());
}


TVideoZoomAndPanMenu::TVideoZoomAndPanMenu(QWidget* parent, TCore* c)
	: TMenu(parent, this, "zoom_and_pan_menu", QT_TR_NOOP("&Zoom and pan"), "zoom_and_pan")
	, core(c) {

	group = new QActionGroup(this);
	group->setExclusive(false);

	// Zoom
	TAction* a = new TAction(this, "reset_zoom", QT_TR_NOOP("&Reset"), "zoom_reset", Qt::SHIFT | Qt::Key_E);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(resetZoomAndPan()));
	addSeparator();
	a = new TAction(this, "auto_zoom", QT_TR_NOOP("&Auto zoom"), "", Qt::SHIFT | Qt::Key_W);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(autoZoom()));
	a = new TAction(this, "zoom_169", QT_TR_NOOP("Zoom for &16:9"), "", Qt::SHIFT | Qt::Key_A);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(autoZoomFor169()));
	a = new TAction(this, "zoom_235", QT_TR_NOOP("Zoom for &2.35:1"), "", Qt::SHIFT | Qt::Key_S);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(autoZoomFor235()));
	addSeparator();
	a = new TAction(this, "dec_zoom", QT_TR_NOOP("Zoom &-"), "", Qt::Key_W);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(decZoom()));
	a = new TAction(this, "inc_zoom", QT_TR_NOOP("Zoom &+"), "", Qt::Key_E);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(incZoom()));

	// Pan
	addSeparator();
	a = new TAction(this, "move_left", QT_TR_NOOP("Move &left"), "", Qt::ALT | Qt::Key_Left);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(panRight()));
	a = new TAction(this, "move_right", QT_TR_NOOP("Move &right"), "", Qt::ALT | Qt::Key_Right);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(panLeft()));
	a = new TAction(this, "move_up", QT_TR_NOOP("Move &up"), "", Qt::ALT | Qt::Key_Up);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(panDown()));
	a = new TAction(this, "move_down", QT_TR_NOOP("Move &down"), "", Qt::ALT | Qt::Key_Down);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(panUp()));

	addActionsTo(parent);
}

} // namespace Gui

