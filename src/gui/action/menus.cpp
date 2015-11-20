#include "gui/action/menus.h"
#include <QDebug>
#include <QMessageBox>
#include "desktop.h"
#include "images.h"
#include "settings/preferences.h"
#include "settings/paths.h"
#include "settings/mediasettings.h"
#include "gui/action/actiongroup.h"
#include "gui/action/widgetactions.h"
#include "playerwindow.h"
#include "gui/playlist.h"
#include "gui/base.h"


using namespace Settings;

namespace Gui {

void execPopup(QWidget* w, QMenu* popup, QPoint p) {
	//qDebug() << "Gui::execPopup:" << p;

	// Keep inside desktop
	QSize s = popup->sizeHint();
	QSize desktop = TDesktop::size(w);
	if (p.x() < 0) p.rx() = 0;
	else if (p.x() + s.width() > desktop.width()) {
		p.rx() = desktop.width() - s.width();
	}
	if (p.y() < 0) p.ry() = 0;
	else if (p.y() + s.height() > desktop.height()) {
		p.ry() = desktop.height() - s.height();
	}

	// Evade mouse
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

	// Popup exec keeps menu inside screen too
	popup->exec(p);
}


TMenu::TMenu(QWidget* parent,
			 QObject* aTranslator,
			 const QString& name,
			 const QString& text,
			 const QString& icon)
	: QMenu(parent)
	, translator(aTranslator)
	, text_en(text) {

	menuAction()->setObjectName(name);
	QString iconName = icon.isEmpty() ? name : icon;
	if (iconName != "noicon")
		menuAction()->setIcon(Images::icon(iconName));

	TBase* main_window = qobject_cast<TBase*>(parent);
	if (main_window) {
		connect(main_window, SIGNAL(enableActions(bool, bool, bool)),
				this, SLOT(enableActions(bool, bool, bool)));
		connect(main_window, SIGNAL(mediaSettingsChanged(Settings::TMediaSettings*)),
				this, SLOT(onMediaSettingsChanged(Settings::TMediaSettings*)));
	}

	retranslateStrings();
}

TMenu::~TMenu() {
}

void TMenu::retranslateStrings() {
	if (!text_en.isEmpty())
		menuAction()->setText(translator->tr(text_en.toUtf8().constData()));
}

void TMenu::changeEvent(QEvent* e) {

	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	} else {
		QMenu::changeEvent(e);
	}
}

void TMenu::enableActions(bool stopped, bool video, bool audio) {
	Q_UNUSED(stopped)
	Q_UNUSED(video)
	Q_UNUSED(audio)
	qDebug() << "TMenu::enableActions:" << menuAction()->objectName() << "always enabled";
}

void TMenu::onMediaSettingsChanged(Settings::TMediaSettings*) {
}

void TMenu::onAboutToShow() {
}

void TMenu::setVisible(bool visible) {

	if (visible)
		onAboutToShow();
	QMenu::setVisible(visible);
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


TAudioChannelMenu::TAudioChannelMenu(QWidget *parent, TCore* c)
	: TMenu(parent, this, "audiochannels_menu", QT_TR_NOOP("&Channels"), "audio_channels")
	, core(c) {

	group = new TActionGroup(this, "channels");
	group->setEnabled(false);
	new TActionGroupItem(this, group, "channels_stereo", QT_TR_NOOP("&Stereo"), TMediaSettings::ChStereo);
	new TActionGroupItem(this, group, "channels_surround", QT_TR_NOOP("&4.0 Surround"), TMediaSettings::ChSurround);
	new TActionGroupItem(this, group, "channels_ful51", QT_TR_NOOP("&5.1 Surround"), TMediaSettings::ChFull51);
	new TActionGroupItem(this, group, "channels_ful61", QT_TR_NOOP("&6.1 Surround"), TMediaSettings::ChFull61);
	new TActionGroupItem(this, group, "channels_ful71", QT_TR_NOOP("&7.1 Surround"), TMediaSettings::ChFull71);
	group->setChecked(core->mset.audio_use_channels);
	connect(group, SIGNAL(activated(int)), core, SLOT(setAudioChannels(int)));
	// No one else sets it
	addActionsTo(parent);
}

void TAudioChannelMenu::enableActions(bool stopped, bool, bool audio) {
	// Uses mset, so useless to set if stopped or no audio
	group->setEnabled(!stopped && audio);
}

void TAudioChannelMenu::onMediaSettingsChanged(TMediaSettings* mset) {
	group->setChecked(mset->audio_use_channels);
}

void TAudioChannelMenu::onAboutToShow() {
	group->setChecked(core->mset.audio_use_channels);
}


TCCMenu::TCCMenu(QWidget *parent, TCore* c)
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

void TCCMenu::enableActions(bool stopped, bool, bool) {
	// Using mset, so useless to set if stopped.
	// Assuming you can have closed captions on audio...
	group->setEnabled(!stopped);
}

void TCCMenu::onMediaSettingsChanged(TMediaSettings* mset) {
	group->setChecked(mset->closed_caption_channel);
}

void TCCMenu::onAboutToShow() {
	group->setChecked(core->mset.closed_caption_channel);
}


TOSDMenu::TOSDMenu(QWidget *parent, TCore* c)
	: TMenu(parent, this, "osd_menu", QT_TR_NOOP("&OSD"), "osd")
	, core(c) {

	group = new TActionGroup(this, "osd");
	// Always enabled
	new TActionGroupItem(this, group, "osd_none", QT_TR_NOOP("Subtitles onl&y"), Settings::TPreferences::None);
	new TActionGroupItem(this, group, "osd_seek", QT_TR_NOOP("Volume + &Seek"), Settings::TPreferences::Seek);
	new TActionGroupItem(this, group, "osd_timer", QT_TR_NOOP("Volume + Seek + &Timer"), Settings::TPreferences::SeekTimer);
	new TActionGroupItem(this, group, "osd_total", QT_TR_NOOP("Volume + Seek + Timer + T&otal time"), Settings::TPreferences::SeekTimerTotal);
	group->setChecked(pref->osd_level);
	connect(group, SIGNAL(activated(int)), core, SLOT(changeOSDLevel(int)));
	connect(core, SIGNAL(osdLevelChanged(int)), group, SLOT(setChecked(int)));

	addSeparator();
	TAction* a = new TAction(this, "inc_osd_scale", QT_TR_NOOP("Size &+"), "", Qt::SHIFT | Qt::Key_U);
	connect(a, SIGNAL(triggered()), core, SLOT(incOSDScale()));
	a = new TAction(this, "dec_osd_scale", QT_TR_NOOP("Size &-"), "", Qt::SHIFT | Qt::Key_Y);
	connect(a, SIGNAL(triggered()), core, SLOT(decOSDScale()));

	addActionsTo(parent);
}

void TOSDMenu::onAboutToShow() {
	group->setChecked((int) pref->osd_level);
}


TStayOnTopMenu::TStayOnTopMenu(QWidget *parent) :
	// TODO: rename to stay_on_top_menu?
	TMenu(parent, this, "ontop_menu", QT_TR_NOOP("&Stay on top"), "ontop") {

	group = new TActionGroup(this, "ontop");
	// Always enabled
	new TActionGroupItem(this, group, "on_top_always", QT_TR_NOOP("&Always"), Settings::TPreferences::AlwaysOnTop);
	new TActionGroupItem(this, group, "on_top_never", QT_TR_NOOP("&Never"), Settings::TPreferences::NeverOnTop);
	new TActionGroupItem(this, group, "on_top_playing", QT_TR_NOOP("While &playing"), Settings::TPreferences::WhilePlayingOnTop);
	group->setChecked((int) pref->stay_on_top);
	connect(group , SIGNAL(activated(int)), parent, SLOT(changeStayOnTop(int)));
	connect(parent , SIGNAL(stayOnTopChanged(int)), group, SLOT(setChecked(int)));

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
	group->setEnabled(false);
	new TActionGroupItem(this, group, "stereo", QT_TR_NOOP("&Stereo"), TMediaSettings::Stereo);
	new TActionGroupItem(this, group, "left_channel", QT_TR_NOOP("&Left channel"), TMediaSettings::Left);
	new TActionGroupItem(this, group, "right_channel", QT_TR_NOOP("&Right channel"), TMediaSettings::Right);
	new TActionGroupItem(this, group, "mono", QT_TR_NOOP("&Mono"), TMediaSettings::Mono);
	new TActionGroupItem(this, group, "reverse_channels", QT_TR_NOOP("Re&verse"), TMediaSettings::Reverse);
	group->setChecked(core->mset.stereo_mode);
	connect(group, SIGNAL(activated(int)), core, SLOT(setStereoMode(int)));
	// No one else changes it
	addActionsTo(parent);
}

void TStereoMenu::enableActions(bool stopped, bool, bool audio) {
	group->setEnabled(!stopped && audio);
}

void TStereoMenu::onMediaSettingsChanged(TMediaSettings* mset) {
	group->setChecked(mset->stereo_mode);
}

void TStereoMenu::onAboutToShow() {
	group->setChecked(core->mset.stereo_mode);
}


TSubFPSMenu::TSubFPSMenu(QWidget *parent, TCore* c)
	: TMenu(parent, this, "subfps_menu", QT_TR_NOOP("F&rames per second"), "subfps")
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

void TSubFPSMenu::enableActions(bool stopped, bool, bool audio) {
	group->setEnabled(!stopped && audio && core->haveExternalSubs());
}

void TSubFPSMenu::onMediaSettingsChanged(TMediaSettings* mset) {
	group->setChecked(mset->external_subtitles_fps);
}

void TSubFPSMenu::onAboutToShow() {
	group->setChecked(core->mset.external_subtitles_fps);
}

} // namespace Gui

