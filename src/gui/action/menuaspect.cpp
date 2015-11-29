#include "gui/action/menuaspect.h"
#include "gui/action/actiongroup.h"
#include "settings/mediasettings.h"
#include "core.h"


using namespace Settings;

namespace Gui {

TMenuAspect::TMenuAspect(QWidget* parent, TCore* c)
	: TMenu(parent, this, "aspect_menu", QT_TR_NOOP("&Aspect ratio"), "aspect")
	, core(c) {

	group = new TActionGroup(this, "aspect");
	group->setEnabled(false);
	new TActionGroupItem(this, group, "aspect_detect", QT_TR_NOOP("&Auto"), TMediaSettings::AspectAuto);
	addSeparator();
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
	connect(core, SIGNAL(aspectRatioChanged(int)), this, SLOT(onAspectRatioChanged(int)));

	addSeparator();
	nextAspectAct = new TAction(this, "next_aspect", QT_TR_NOOP("Next aspect ratio"), "", Qt::Key_A);
	connect(nextAspectAct, SIGNAL(triggered()), core, SLOT(nextAspectRatio()));

	addActionsTo(parent);
}

void TMenuAspect::upd() {

	int id = core->mset.aspect_ratio_id;
	QAction* action = group->setChecked(id);

	QString tip;
	if (id == TMediaSettings::AspectAuto && core->mdat.video_aspect != 0) {
		tip = tr("Aspect ratio %1").arg(QString::number(core->mdat.video_aspect));
	} else {
		if (action) {
			tip = action->text();
			tip.replace("&", "");
			tip = tr("Aspect ratio %1").arg(tip.toLower());
		} else {
			tip = tr("Aspect ratio");
		}
	}
	menuAction()->setToolTip(tip);
}

void TMenuAspect::enableActions(bool stopped, bool video, bool) {
	// Uses mset, so useless to set if stopped or no video
	bool enabled = !stopped && video;
	group->setEnabled(enabled);
	nextAspectAct->setEnabled(enabled);
	upd();
}

void TMenuAspect::onMediaSettingsChanged(TMediaSettings*) {
	upd();
}

void TMenuAspect::onAboutToShow() {
	upd();
}

void TMenuAspect::onAspectRatioChanged(int) {
	upd();
}

} // namespace Gui
