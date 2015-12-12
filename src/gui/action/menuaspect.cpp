#include "gui/action/menuaspect.h"
#include "gui/action/actiongroup.h"
#include "settings/aspectratio.h"
#include "settings/mediasettings.h"
#include "core.h"


using namespace Settings;

namespace Gui {

TMenuAspect::TMenuAspect(QWidget* parent, TCore* c)
	: TMenu(parent, this, "aspect_menu", QT_TR_NOOP("&Aspect ratio"), "aspect")
	, core(c) {

	group = new TActionGroup(this, "aspect");
	group->setEnabled(false);
	aspectAutoAct = new TActionGroupItem(this, group, "aspect_detect", QT_TR_NOOP("Au&to"), TAspectRatio::AspectAuto);
	addSeparator();
	new TActionGroupItem(this, group, "aspect_1_1", TAspectRatio::aspectIDToString(0), TAspectRatio::Aspect11);
	new TActionGroupItem(this, group, "aspect_5_4", TAspectRatio::aspectIDToString(1), TAspectRatio::Aspect54);
	new TActionGroupItem(this, group, "aspect_4_3", TAspectRatio::aspectIDToString(2), TAspectRatio::Aspect43);
	new TActionGroupItem(this, group, "aspect_11_8", TAspectRatio::aspectIDToString(3), TAspectRatio::Aspect118);
	new TActionGroupItem(this, group, "aspect_14_10", TAspectRatio::aspectIDToString(4), TAspectRatio::Aspect1410);
	new TActionGroupItem(this, group, "aspect_3_2", TAspectRatio::aspectIDToString(5), TAspectRatio::Aspect32);
	new TActionGroupItem(this, group, "aspect_14_9", TAspectRatio::aspectIDToString(6), TAspectRatio::Aspect149);
	new TActionGroupItem(this, group, "aspect_16_10", TAspectRatio::aspectIDToString(7), TAspectRatio::Aspect1610);
	new TActionGroupItem(this, group, "aspect_16_9", TAspectRatio::aspectIDToString(8), TAspectRatio::Aspect169);
	new TActionGroupItem(this, group, "aspect_2.35_1", TAspectRatio::aspectIDToString(9), TAspectRatio::Aspect235);
	addSeparator();
	new TActionGroupItem(this, group, "aspect_none", QT_TR_NOOP("&Disabled"), TAspectRatio::AspectNone);

	connect(group, SIGNAL(activated(int)), core, SLOT(changeAspectRatio(int)));
	connect(core, SIGNAL(aspectRatioChanged(Settings::TAspectRatio::TMenuID)),
			this, SLOT(onAspectRatioChanged(Settings::TAspectRatio::TMenuID)));

	addSeparator();
	nextAspectAct = new TAction(this, "next_aspect", QT_TR_NOOP("Next &aspect ratio"), "", Qt::Key_A);
	connect(nextAspectAct, SIGNAL(triggered()), core, SLOT(nextAspectRatio()));

	addActionsTo(parent);
	upd();
}

void TMenuAspect::upd() {

	group->setChecked(core->mset.aspect_ratio.ID());

	double aspect = core->mset.aspectToDouble();
	QString tip = TAspectRatio::doubleToString(aspect);
	aspectAutoAct->setTextAndTip(tr("Au&to %1").arg(tip));
	menuAction()->setToolTip(tr("Aspect ratio %1").arg(tip));
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

void TMenuAspect::onAspectRatioChanged(Settings::TAspectRatio::TMenuID) {
	upd();
}

} // namespace Gui
