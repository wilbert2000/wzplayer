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
	//qDebug() << "Gui::TMenu::enableActions:" << menuAction()->objectName() << "always enabled";
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

} // namespace Gui

