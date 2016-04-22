/*  WZPlayer, GUI front-end for mplayer and MPV.
    Parts copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "gui/pref/input.h"
#include "images.h"
#include "settings/preferences.h"

using namespace Settings;

namespace Gui { namespace Pref {

TInput::TInput(QWidget* parent, Qt::WindowFlags f)
	: TWidget(parent, f) {

	setupUi(this);
	retranslateStrings();
}

TInput::~TInput() {
}

QString TInput::sectionName() {
	return tr("Actions");
}

QPixmap TInput::sectionIcon() {
	return Images::icon("pref_input", icon_size);
}

void TInput::createMouseCombos() {
	left_click_combo->clear();
	right_click_combo->clear();
	double_click_combo->clear();
	middle_click_combo->clear();
	xbutton1_click_combo->clear();
	xbutton2_click_combo->clear();

	left_click_combo->addItem(tr("None"), "");
	left_click_combo->addItem(tr("Play"), "play");
	left_click_combo->addItem(tr("Play / Pause"), "play_or_pause");
	left_click_combo->addItem(tr("Pause"), "pause");
	left_click_combo->addItem(tr("Stop"), "stop");
	left_click_combo->addItem(tr("Frame back step"), "frame_back_step");
	left_click_combo->addItem(tr("Go backward (short)"), "rewind1");
	left_click_combo->addItem(tr("Go backward (medium)"), "rewind2");
	left_click_combo->addItem(tr("Go backward (long)"), "rewind3");
	left_click_combo->addItem(tr("Frame step"), "frame_step");
	left_click_combo->addItem(tr("Go forward (short)"), "forward1");
	left_click_combo->addItem(tr("Go forward (medium)"), "forward2");
	left_click_combo->addItem(tr("Go forward (long)"), "forward3");
	left_click_combo->addItem(tr("Increase volume"), "increase_volume");
	left_click_combo->addItem(tr("Decrease volume"), "decrease_volume");
	left_click_combo->addItem(tr("Fullscreen"), "fullscreen");
	left_click_combo->addItem(tr("Screenshot"), "screenshot");
	left_click_combo->addItem(tr("Always on top"), "on_top_always");
	left_click_combo->addItem(tr("Never on top"), "on_top_never");
	left_click_combo->addItem(tr("On top while playing"), "on_top_while_playing");
	left_click_combo->addItem(tr("Mute"), "mute");
	left_click_combo->addItem(tr("OSD - Next level"), "next_osd");
	left_click_combo->addItem(tr("Playlist"), "show_playlist");
	left_click_combo->addItem(tr("Reset zoom"), "reset_zoom");
	left_click_combo->addItem(tr("Exit fullscreen"), "exit_fullscreen");
	left_click_combo->addItem(tr("Normal speed"), "normal_speed");
	left_click_combo->addItem(tr("Frame counter"), "frame_counter");
	left_click_combo->addItem(tr("Preferences"), "show_preferences");
	left_click_combo->addItem(tr("Double size"), "toggle_double_size");
	left_click_combo->addItem(tr("Next chapter"), "next_chapter");
	left_click_combo->addItem(tr("Previous chapter"), "prev_chapter");
	left_click_combo->addItem(tr("Show video equalizer"), "video_equalizer");
	left_click_combo->addItem(tr("Show audio equalizer"), "audio_equalizer");
	left_click_combo->addItem(tr("Show context menu"), "show_context_menu");
	left_click_combo->addItem(tr("Next wheel action"), "next_wheel_function");
	left_click_combo->addItem(tr("DVD mouse click"), "dvdnav_mouse");
	left_click_combo->addItem(tr("DVD return to main menu"), "dvdnav_menu");
	left_click_combo->addItem(tr("DVD return to previous menu"), "dvdnav_prev");
	left_click_combo->addItem(tr("DVD move cursor up"), "dvdnav_up");
	left_click_combo->addItem(tr("DVD move cursor down"), "dvdnav_down");
	left_click_combo->addItem(tr("DVD move cursor left"), "dvdnav_left");
	left_click_combo->addItem(tr("DVD move cursor right"), "dvdnav_right");
	left_click_combo->addItem(tr("DVD select button"), "dvdnav_select");

	// Copy to other combos
	for (int n=0; n < left_click_combo->count(); n++) {
		double_click_combo->addItem(left_click_combo->itemText(n),
                                     left_click_combo->itemData(n));

		right_click_combo->addItem(left_click_combo->itemText(n),
                                    left_click_combo->itemData(n));

		middle_click_combo->addItem(left_click_combo->itemText(n),
                                     left_click_combo->itemData(n));

		xbutton1_click_combo->addItem(left_click_combo->itemText(n),
                                       left_click_combo->itemData(n));

		xbutton2_click_combo->addItem(left_click_combo->itemText(n),
                                       left_click_combo->itemData(n));
	}
}

void TInput::retranslateStrings() {

	int wheel_function = wheel_function_combo->currentIndex();
	int timeslider_pos = timeslider_behaviour_combo->currentIndex();

	retranslateUi(this);

	keyboard_icon->setPixmap(Images::icon("keyboard"));
	mouse_icon->setPixmap(Images::icon("mouse"));

    // Mouse function combos
	int mouse_left = left_click_combo->currentIndex();
	int mouse_right = right_click_combo->currentIndex();
	int mouse_double = double_click_combo->currentIndex();
	int mouse_middle = middle_click_combo->currentIndex();
	int mouse_xclick1 = xbutton1_click_combo->currentIndex();
	int mouse_xclick2 = xbutton2_click_combo->currentIndex();

	createMouseCombos();

	left_click_combo->setCurrentIndex(mouse_left);
	right_click_combo->setCurrentIndex(mouse_right);
	double_click_combo->setCurrentIndex(mouse_double);
	middle_click_combo->setCurrentIndex(mouse_middle);
	xbutton1_click_combo->setCurrentIndex(mouse_xclick1);
	xbutton2_click_combo->setCurrentIndex(mouse_xclick2);

	wheel_function_combo->clear();
	wheel_function_combo->addItem(tr("No function"), TPreferences::DoNothing);
	wheel_function_combo->addItem(tr("Media seeking"), TPreferences::Seeking);
	wheel_function_combo->addItem(tr("Volume control"), TPreferences::Volume);
	wheel_function_combo->addItem(tr("Zoom video"), TPreferences::Zoom);
	wheel_function_combo->addItem(tr("Change speed"), TPreferences::ChangeSpeed);
	wheel_function_combo->setCurrentIndex(wheel_function);

	wheel_function_seek->setText(tr("Media &seeking"));
	wheel_function_zoom->setText(tr("&Zoom video"));
	wheel_function_volume->setText(tr("&Volume control"));
	wheel_function_speed->setText(tr("&Change speed"));

	// Seek tab
    if (!qApp->isLeftToRight()) {
        seek1_icon->setPixmap(Images::flippedIcon("forward1", 32));
        seek2_icon->setPixmap(Images::flippedIcon("forward2", 32));
        seek3_icon->setPixmap(Images::flippedIcon("forward3", 32));
	}

	timeslider_behaviour_combo->setCurrentIndex(timeslider_pos);

	createHelp();
}

void TInput::setData(Settings::TPreferences* pref) {

	setLeftClickFunction(pref->mouse_left_click_function);
	setRightClickFunction(pref->mouse_right_click_function);
	setDoubleClickFunction(pref->mouse_double_click_function);
	setMiddleClickFunction(pref->mouse_middle_click_function);
	setXButton1ClickFunction(pref->mouse_xbutton1_click_function);
	setXButton2ClickFunction(pref->mouse_xbutton2_click_function);
	setWheelFunction(pref->wheel_function);
	setWheelFunctionCycle(pref->wheel_function_cycle);
	setWheelFunctionSeekingReverse(pref->wheel_function_seeking_reverse);
	wait_for_double_click_check->setChecked(pref->delay_left_click);

	setSeeking1(pref->seeking1);
	setSeeking2(pref->seeking2);
	setSeeking3(pref->seeking3);
	setSeeking4(pref->seeking4);

	setUpdateWhileDragging(pref->update_while_seeking);
	setRelativeSeeking(pref->relative_seeking);
	setPreciseSeeking(pref->precise_seeking);
}

void TInput::getData(Settings::TPreferences* pref) {

	requires_restart = false;
	pref->mouse_left_click_function = leftClickFunction();
	pref->mouse_right_click_function = rightClickFunction();
	pref->mouse_double_click_function = doubleClickFunction();
	pref->mouse_middle_click_function = middleClickFunction();
	pref->mouse_xbutton1_click_function = xButton1ClickFunction();
	pref->mouse_xbutton2_click_function = xButton2ClickFunction();
	pref->wheel_function = wheelFunction();
	pref->wheel_function_cycle = wheelFunctionCycle();
	pref->wheel_function_seeking_reverse = wheelFunctionSeekingReverse();
	pref->delay_left_click = wait_for_double_click_check->isChecked();

	// Seeking tab
	pref->seeking1 = seeking1();
	pref->seeking2 = seeking2();
	pref->seeking3 = seeking3();
	pref->seeking4 = seeking4();

	pref->update_while_seeking = updateWhileDragging();
	pref->relative_seeking= relativeSeeking();
	pref->precise_seeking = preciseSeeking();
}

/*
void TInput::setActionsList(QStringList l) {
	left_click_combo->insertStringList(l);
	double_click_combo->insertStringList(l);
}
*/

void TInput::setLeftClickFunction(const QString& f) {
	int pos = left_click_combo->findData(f);
	if (pos == -1) pos = 0; //None
	left_click_combo->setCurrentIndex(pos);
}

QString TInput::leftClickFunction() {
	return left_click_combo->itemData(left_click_combo->currentIndex()).toString();
}

void TInput::setRightClickFunction(const QString& f) {
	int pos = right_click_combo->findData(f);
	if (pos == -1) pos = 0; //None
	right_click_combo->setCurrentIndex(pos);
}

QString TInput::rightClickFunction() {
	return right_click_combo->itemData(right_click_combo->currentIndex()).toString();
}

void TInput::setDoubleClickFunction(const QString& f) {
	int pos = double_click_combo->findData(f);
	if (pos == -1) pos = 0; //None
	double_click_combo->setCurrentIndex(pos);
}

QString TInput::doubleClickFunction() {
	return double_click_combo->itemData(double_click_combo->currentIndex()).toString();
}

void TInput::setMiddleClickFunction(const QString& f) {
	int pos = middle_click_combo->findData(f);
	if (pos == -1) pos = 0; //None
	middle_click_combo->setCurrentIndex(pos);
}

QString TInput::middleClickFunction() {
	return middle_click_combo->itemData(middle_click_combo->currentIndex()).toString();
}

void TInput::setXButton1ClickFunction(const QString& f) {
	int pos = xbutton1_click_combo->findData(f);
	if (pos == -1) pos = 0; //None
	xbutton1_click_combo->setCurrentIndex(pos);
}

QString TInput::xButton1ClickFunction() {
	return xbutton1_click_combo->itemData(xbutton1_click_combo->currentIndex()).toString();
}

void TInput::setXButton2ClickFunction(const QString& f) {
	int pos = xbutton2_click_combo->findData(f);
	if (pos == -1) pos = 0; //None
	xbutton2_click_combo->setCurrentIndex(pos);
}

QString TInput::xButton2ClickFunction() {
	return xbutton2_click_combo->itemData(xbutton2_click_combo->currentIndex()).toString();
}

void TInput::setWheelFunction(int function) {
	int d = wheel_function_combo->findData(function);
	if (d < 0) d = 0;
	wheel_function_combo->setCurrentIndex(d);
}

int TInput::wheelFunction() {
	return wheel_function_combo->itemData(wheel_function_combo->currentIndex()).toInt();
}

void TInput::setWheelFunctionCycle(TPreferences::TWheelFunctions flags){

	wheel_function_seek->setChecked(flags.testFlag(TPreferences::Seeking));
	wheel_function_volume->setChecked(flags.testFlag(TPreferences::Volume));
	wheel_function_zoom->setChecked(flags.testFlag(TPreferences::Zoom));
	wheel_function_speed->setChecked(flags.testFlag(TPreferences::ChangeSpeed));
}

Settings::TPreferences::TWheelFunctions TInput::wheelFunctionCycle() {

	Settings::TPreferences::TWheelFunctions seekflags (QFlag ((int) TPreferences::Seeking)) ;
	Settings::TPreferences::TWheelFunctions volumeflags (QFlag ((int) TPreferences::Volume)) ;
	Settings::TPreferences::TWheelFunctions zoomflags (QFlag ((int) TPreferences::Zoom)) ;
	Settings::TPreferences::TWheelFunctions speedflags (QFlag ((int) TPreferences::ChangeSpeed)) ;
	Settings::TPreferences::TWheelFunctions out (QFlag (0));
	if(wheel_function_seek->isChecked()){
		out = out | seekflags;
	}
	if(wheel_function_volume->isChecked()){
		out = out | volumeflags;
	}
	if(wheel_function_zoom->isChecked()){
		out = out | zoomflags;
	}
	if(wheel_function_speed->isChecked()){
		out = out | speedflags;
	}
	return out;
}

void TInput::setWheelFunctionSeekingReverse(bool b) {
	wheel_function_seeking_reverse_check->setChecked(b);
}

bool TInput::wheelFunctionSeekingReverse() {
	return wheel_function_seeking_reverse_check->isChecked();
}

void TInput::setSeeking1(int n) {
    int m = n / 60;
    int s = n - 60 * m;
    seek1_minutes_spin->setValue(m);
    seek1_seconds_spin->setValue(s);
}

int TInput::seeking1() {
    int s = seek1_minutes_spin->value() * 60 + seek1_seconds_spin->value();
    if (s == 0) {
        s = 1;
    }
    return s;
}

void TInput::setSeeking2(int n) {
    int m = n / 60;
    int s = n - 60 * m;
    seek2_minutes_spin->setValue(m);
    seek2_seconds_spin->setValue(s);
}

int TInput::seeking2() {
    int s = seek2_minutes_spin->value() * 60 + seek2_seconds_spin->value();
    if (s == 0) {
        s = 1;
    }
    return s;
}

void TInput::setSeeking3(int n) {
    int m = n / 60;
    int s = n - 60 * m;
    seek3_minutes_spin->setValue(m);
    seek3_seconds_spin->setValue(s);
}

int TInput::seeking3() {
    int s = seek3_minutes_spin->value() * 60 + seek3_seconds_spin->value();
    if (s == 0) {
        s = 1;
    }
    return s;
}

void TInput::setSeeking4(int n) {
    int m = n / 60;
    int s = n - 60 * m;
    seek4_minutes_spin->setValue(m);
    seek4_seconds_spin->setValue(s);
}

int TInput::seeking4() {
    int s = seek4_minutes_spin->value() * 60 + seek4_seconds_spin->value();
    if (s == 0) {
        s = 1;
    }
    return s;
}

void TInput::setUpdateWhileDragging(bool b) {
	if (b)
		timeslider_behaviour_combo->setCurrentIndex(0);
	else
		timeslider_behaviour_combo->setCurrentIndex(1);
}

bool TInput::updateWhileDragging() {
	return (timeslider_behaviour_combo->currentIndex() == 0);
}

void TInput::setRelativeSeeking(bool b) {
	relative_seeking_button->setChecked(b);
	absolute_seeking_button->setChecked(!b);
}

bool TInput::relativeSeeking() {
	return relative_seeking_button->isChecked();
}

void TInput::setPreciseSeeking(bool b) {
	precise_seeking_check->setChecked(b);
}

bool TInput::preciseSeeking() {
	return precise_seeking_check->isChecked();
}

void TInput::createHelp() {

	clearHelp();

	addSectionTitle(tr("Keyboard"));

	setWhatsThis(actions_editor, tr("Shortcut editor"),
        tr("This table allows you to change the keyboard shortcuts of actions."
           " Double click or press enter on a item, or press the"
           " <b>Change shortcut</b> button to open the"
           " <i>Modify shortcut</i> dialog.<br><br>"
           "There are two ways to change a shortcut:<br>"
           "If the <b>Capture</b> button is on then just press the key"
           " or combination of keys you want to assign to the shortcut.<br>"
           "If the <b>Capture</b> button is off you can enter the full name"
           " of the key.<br><br>"
           "Sadly shortcuts cannot differentiate between the numeric key pad"
           " and normal keys or between the left Shift, Ctrl, Meta, Alt"
           " and the right Shift, Ctrl, Meta and AltGr modifiers.<br>"
           "You can assign multiple shortcuts to one action and one shortcut"
           " may consist of up to 4 keys with multiple modifiers."),
           false);

	addSectionTitle(tr("Mouse"));

	addSectionGroup(tr("Buttons"));

	setWhatsThis(left_click_combo, tr("Left click"),
		tr("Select the action for left click on the mouse."));

	setWhatsThis(double_click_combo, tr("Double click"),
		tr("Select the action for double click on the mouse."));

	setWhatsThis(wait_for_double_click_check,
		tr("Wait for double click before triggering left click action"),
		tr("When checked, a left click will be delayed by %1 millisecond"
		   " to see whether it will turn into a double click. When a double"
		   " click is detected, the left click action is canceled and the"
		   " double click action is triggered instead. This is the default"
		   " left click behaviour.").arg(qApp->doubleClickInterval())
		+ "<br><br>"
		+ tr("For precise seeking, it can be convenient to trigger the left"
			 " click action right away, without delay, to instantly pause a"
			 " video. Consequently this will cause a left click action being"
             " triggered before and after each double click action."
             " This won't cause much trouble if the default action for the"
             " left mouse button, play_or_pause, is still assigned, because"
             " the first left click will be canceled by the second."));

	setWhatsThis(middle_click_combo, tr("Middle click"),
		tr("Select the action for middle click on the mouse."));

	setWhatsThis(xbutton1_click_combo, tr("X Button 1"),
		tr("Select the action for the X button 1."));

	setWhatsThis(xbutton2_click_combo, tr("X Button 2"),
		tr("Select the action for the X button 2."));


	addSectionGroup(tr("Wheel"));

	setWhatsThis(wheel_function_combo, tr("Current scroll action"),
		tr("Select the action for scrolling the mouse wheel."));

	setWhatsThis(wheel_function_seek, tr("Media seeking"),
		tr("Check it to enable seeking as action for the mouse wheel."));

	setWhatsThis(wheel_function_volume, tr("Volume control"),
		tr("Check it to enable changing volume as action for the mouse wheel."));

	setWhatsThis(wheel_function_zoom, tr("Zoom video"),
		tr("Check it to enable zooming as action for the mouse wheel."));

	setWhatsThis(wheel_function_speed, tr("Change speed"),
		tr("Check it to enable changing speed as action for the mouse wheel."));

	setWhatsThis(wheel_function_seeking_reverse_check, tr("Reverse mouse wheel seeking"),
		tr("Check it to seek in the opposite direction."));


	addSectionTitle(tr("Seeking"));

    setWhatsThis(seek1_minutes_spin, tr("Short jump"),
		tr("Select the time that should be go forward or backward when you "
		   "choose the %1 action.").arg(tr("short jump")));

    setWhatsThis(seek2_minutes_spin, tr("Medium jump"),
		tr("Select the time that should be go forward or backward when you "
		   "choose the %1 action.").arg(tr("medium jump")));

    setWhatsThis(seek3_minutes_spin, tr("Long jump"),
		tr("Select the time that should be go forward or backward when you "
		   "choose the %1 action.").arg(tr("long jump")));

    setWhatsThis(seek4_minutes_spin, tr("Mouse wheel jump"),
		tr("Select the time that should be go forward or backward when you "
		   "move the mouse wheel."));

	setWhatsThis(seeking_method_group, tr("Seeking method"),
		tr("Sets the method to be used when seeking with the time slider."
		   " Absolute seeking seeks with the requested timestamp. It may be"
		   " a little bit more accurate, while relative seeking, seeks with"
		   " a time offset, 5 seconds forward from here, and may work better"
		   " with files having a wrong duration."));

	setWhatsThis(precise_seeking_check, tr("Precise seeking"),
        tr("If checked, seeks are more accurate but slower."
           " If not checked seeks are done to keyframes, faster,"
           " but can be off by quite a few seconds for heavily"
           " compressed videos with only a few keyframes."));

	setWhatsThis(timeslider_behaviour_combo, tr("Behaviour of time slider"),
        tr("Select whether or not to update the video while dragging the time slider."));
}

}} // namespace Gui::Pref

#include "moc_input.cpp"
