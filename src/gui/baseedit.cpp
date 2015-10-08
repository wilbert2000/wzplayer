#include "gui/baseedit.h"

namespace Gui {

TBaseEdit::TBaseEdit() : TBasePlus() {

	// Create edit control widget action
	editControlWidgetAct = new TAction(this, "edit_controlwidget");
	// Create control widget
	controlwidget = new TEditableToolbar(this);
	controlwidget->setObjectName("controlwidget");
	controlwidget->setLayoutDirection(Qt::LeftToRight);

	QStringList actions;
	actions << "play_or_pause"
			<< "stop"
			<< "separator"
			<< "rewindbutton_action"
			<< "timeslider_action"
			<< "forwardbutton_action"
			<< "separator"
			<< "fullscreen"
			<< "mute"
			<< "volumeslider_action";
	controlwidget->setDefaultActions(actions);

	controlwidget->setMovable(false);
	// controlwidget->setMovable(true);
	// controlwidget->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
	addToolBar(Qt::BottomToolBarArea, controlwidget);

	connect(editControlWidgetAct, SIGNAL(triggered()),
			controlwidget, SLOT(edit()));
}

TBaseEdit::~TBaseEdit() {
}

void TBaseEdit::retranslateStrings() {
	qDebug("Gui::TDefault::retranslateStrings");

	TBasePlus::retranslateStrings();

	editControlWidgetAct->change(tr("Edit &control bar"));
	controlwidget->setWindowTitle(tr("Control bar"));
} // retranslateStrings

void TBaseEdit::loadConfig(const QString &gui_group) {

	TBasePlus::loadConfig(gui_group);

	pref->beginGroup(gui_group);

	pref->beginGroup("actions");
	controlwidget->setActionsFromStringList(pref->value("controlwidget", controlwidget->defaultActions()).toStringList());
	pref->endGroup();

	pref->beginGroup("toolbars_icon_size");
	controlwidget->setIconSize(pref->value("controlwidget", controlwidget->iconSize()).toSize());
	pref->endGroup();

	pref->endGroup();

	if (pref->compact_mode) {
		controlwidget->hide();
	}
}

void TBaseEdit::saveConfig(const QString& gui_group) {
	qDebug("Gui::TBaseEdit::saveConfig");

	TBasePlus::saveConfig(gui_group);
	pref->beginGroup(gui_group);

	pref->beginGroup("actions");
	pref->setValue("controlwidget", controlwidget->actionsToStringList());
	pref->endGroup();

	pref->beginGroup("toolbars_icon_size");
	pref->setValue("controlwidget", controlwidget->iconSize());
	pref->endGroup();

	pref->endGroup();
}
} // namespace Gui

#include "moc_baseedit.cpp"
