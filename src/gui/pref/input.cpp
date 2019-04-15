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
#include "gui/mainwindow.h"
#include "gui/action/actionseditor.h"
#include "settings/preferences.h"
#include "images.h"

using namespace Settings;

namespace Gui { namespace Pref {

TInput::TInput(QWidget* parent)
    : TSection(parent) {

    setupUi(this);
    retranslateUi(this);

    keyboard_icon->setPixmap(Images::icon("keyboard"));
    mouse_icon->setPixmap(Images::icon("mouse"));

    createMouseCombos();

    wheel_function_combo->addItem(tr("No function"), TPreferences::DoNothing);
    wheel_function_combo->addItem(tr("Media seeking"), TPreferences::Seeking);
    wheel_function_combo->addItem(tr("Volume control"), TPreferences::Volume);
    wheel_function_combo->addItem(tr("Zoom video"), TPreferences::Zoom);
    wheel_function_combo->addItem(tr("Change speed"), TPreferences::ChangeSpeed);

    wheel_function_seek->setText(tr("Media &seeking"));
    wheel_function_zoom->setText(tr("&Zoom video"));
    wheel_function_volume->setText(tr("&Volume control"));
    wheel_function_speed->setText(tr("&Change speed"));

    // Seek tab
    seek1_icon->setPixmap(Images::icon("seek_forward1", 32));
    seek2_icon->setPixmap(Images::icon("seek_forward2", 32));
    seek3_icon->setPixmap(Images::icon("seek_forward3", 32));
    seek4_icon->setPixmap(Images::icon("mouse", 32));
    seek_rate_icon->setPixmap(Images::icon("seek_frequency", 32));

    createHelp();
}

QString TInput::sectionName() {
    return tr("Input");
}

QPixmap TInput::sectionIcon() {
    return Images::icon("pref_input", iconSize);
}

void TInput::addActionItem(const QString& name) {

    QAction* action = Gui::mainWindow->requireAction(name);
    left_click_combo->addItem(
                tr("%1 (%2)")
                .arg(Action::TActionsEditor::cleanActionText(action->text(),
                                                             name))
                .arg(name),
                name);
}

void TInput::createMouseCombos() {

    left_click_combo->clear();
    right_click_combo->clear();
    double_click_combo->clear();
    middle_click_combo->clear();
    xbutton1_click_combo->clear();
    xbutton2_click_combo->clear();

    left_click_combo->addItem(tr("None"), "");
    addActionItem("pl_play");
    addActionItem("play_pause");
    addActionItem("play_pause_stop");
    addActionItem("pause");
    addActionItem("stop");
    addActionItem("seek_rewind_frame");
    addActionItem("seek_rewind1");
    addActionItem("seek_rewind2");
    addActionItem("seek_rewind3");
    addActionItem("seek_forward_frame");
    addActionItem("seek_forward1");
    addActionItem("seek_forward2");
    addActionItem("seek_forward3");
    addActionItem("increase_volume");
    addActionItem("decrease_volume");
    addActionItem("fullscreen");
    addActionItem("screenshot");
    addActionItem("stay_on_top_always");
    addActionItem("stay_on_top_never");
    addActionItem("stay_on_top_playing");
    addActionItem("mute");
    addActionItem("osd_next");
    addActionItem("view_playlist");
    addActionItem("reset_zoom_pan");
    addActionItem("exit_fullscreen");
    addActionItem("speed_normal");
    addActionItem("toggle_frames");
    addActionItem("view_settings");
    addActionItem("size_toggle_double");
    addActionItem("next_chapter");
    addActionItem("prev_chapter");
    addActionItem("video_equalizer");
    addActionItem("audio_equalizer");
    addActionItem("show_context_menu");
    addActionItem("next_wheel_function");
    addActionItem("dvdnav_mouse");
    addActionItem("dvdnav_menu");
    addActionItem("dvdnav_prev");
    addActionItem("dvdnav_up");
    addActionItem("dvdnav_down");
    addActionItem("dvdnav_left");
    addActionItem("dvdnav_right");
    addActionItem("dvdnav_select");

    // Copy to other combos
    for (int n = 0; n < left_click_combo->count(); n++) {
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

void TInput::setData(Settings::TPreferences* pref) {

    actions_editor->setActionsTable();

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
    seek_rate_spin->setValue(pref->seek_rate);

    setSeekRelative(pref->seek_relative);
    setSeekKeyframes(pref->seek_keyframes);
    seek_preview_check->setChecked(pref->seek_preview);
}

void TInput::getData(Settings::TPreferences* pref) {

    TSection::getData(pref);

    // Save new action shortcuts to the currently active actions
    actions_editor->applyChanges();
    // Save actions to pref
    Action::TActionsEditor::saveSettings(pref);

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
    pref->seek_rate = seek_rate_spin->value();

    pref->seek_relative= seekRelative();
    pref->seek_keyframes = seekKeyframes();

    if (seek_preview_check->isChecked() && !pref->seek_preview) {
        _requiresRestartPlayer = true;
    }
    pref->seek_preview = seek_preview_check->isChecked();
}

/*
void TInput::setActionsList(QStringList l) {
    left_click_combo->insertStringList(l);
    double_click_combo->insertStringList(l);
}
*/

void TInput::setLeftClickFunction(const QString& f) {

    int pos = left_click_combo->findData(f);
    if (pos == -1) pos = 0; // None
    left_click_combo->setCurrentIndex(pos);
}

QString TInput::leftClickFunction() {
    return left_click_combo->itemData(left_click_combo->currentIndex())
            .toString();
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

    TPreferences::TWheelFunctions seekflags (QFlag ((int) TPreferences::Seeking)) ;
    TPreferences::TWheelFunctions volumeflags (QFlag ((int) TPreferences::Volume)) ;
    TPreferences::TWheelFunctions zoomflags (QFlag ((int) TPreferences::Zoom)) ;
    TPreferences::TWheelFunctions speedflags (QFlag ((int) TPreferences::ChangeSpeed)) ;
    TPreferences::TWheelFunctions out (QFlag (0));
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

void TInput::setSeekRelative(bool b) {
    seek_relative_button->setChecked(b);
    seek_absolute_button->setChecked(!b);
}

bool TInput::seekRelative() {
    return seek_relative_button->isChecked();
}

void TInput::setSeekKeyframes(bool b) {
    seek_keyframes_check->setChecked(b);
}

bool TInput::seekKeyframes() {
    return seek_keyframes_check->isChecked();
}

void TInput::createHelp() {

    clearHelp();

    addSectionTitle(tr("Keyboard"));

    setWhatsThis(actions_editor, tr("Shortcut editor"),
        tr("This table allows you to change the keyboard shortcuts of actions."
           " Double click or press enter on a item, or press the"
           " <b>Change shortcut</b> button to open the"
           " <i>Modify shortcut</i> dialog.<br/><br/>"
           "There are two ways to change a shortcut:<br/>"
           "If the <b>Capture</b> button is on then just press the key"
           " or combination of keys you want to assign to the shortcut.<br/>"
           "If the <b>Capture</b> button is off you can enter the full name"
           " of the key.<br/><br/>"
           "Sadly shortcuts cannot differentiate between the numeric key pad"
           " and normal keys or between the left Shift, Ctrl, Meta, Alt"
           " and the right Shift, Ctrl, Meta and AltGr modifiers.<br/>"
           "You can assign multiple shortcuts to one action and one shortcut"
           " may consist of up to 4 keys with multiple modifiers."),
           false);

    addSectionTitle(tr("Mouse"));

    addSectionGroup(tr("Buttons"));

    setWhatsThis(left_click_combo, tr("Left click"),
        tr("Select the action for the left mouse button."));

    setWhatsThis(double_click_combo, tr("Double click"),
        tr("Select the action for double clicking the left mouse button."));

    setWhatsThis(wait_for_double_click_check,
        tr("Wait for double click before triggering left click action"),
        tr("When checked, a left click will be delayed by %1 millisecond"
           " to see whether it will turn into a double click. When a double"
           " click is detected, the left click action is canceled and the"
           " double click action is triggered instead. This is the default"
           " left click behaviour.").arg(qApp->doubleClickInterval())
        + "<br/><br/>"
        + tr("While seeking, it can be convenient to trigger the left"
             " click action right away, without delay, to instantly pause a"
             " video. Consequently this will cause a left click action being"
             " triggered before and after each double click action."
             " This won't cause much trouble if the default action for the"
             " left mouse button, play_pause, is still assigned,"
             " because the first left click will be canceled by the second."));

    setWhatsThis(middle_click_combo, tr("Middle click"),
       tr("Select the action for the middle mouse button."));

    setWhatsThis(xbutton1_click_combo, tr("X Button 1"),
        tr("Select the action for X button 1."));

    setWhatsThis(xbutton2_click_combo, tr("X Button 2"),
        tr("Select the action for X button 2."));


    addSectionGroup(tr("Wheel"));

    setWhatsThis(wheel_function_combo, tr("Current scroll action"),
        tr("Select the action for scrolling the mouse wheel."));

    setWhatsThis(wheel_function_seek, tr("Media seeking"),
        tr("Enable seeking as action for the mouse wheel."));

    setWhatsThis(wheel_function_volume, tr("Volume control"),
        tr("Enable changing volume as action for the mouse wheel."));

    setWhatsThis(wheel_function_zoom, tr("Zoom video"),
        tr("Enable zooming as action for the mouse wheel."));

    setWhatsThis(wheel_function_speed, tr("Change speed"),
        tr("Enable changing speed as action for the mouse wheel."));

    setWhatsThis(wheel_function_seeking_reverse_check,
                 tr("Reverse wheel direction"),
                 tr("Reverse the direction of the mouse wheel."));


    addSectionTitle(tr("Seeking"));

    setWhatsThis(seek1_minutes_spin, tr("Short jump"),
                 tr("Set the time to seek forward or rewind for the %1 action.")
                 .arg(tr("short jump")));
    setWhatsThis(seek1_seconds_spin, tr("Short jump"),
                 tr("Set the time to seek forward or rewind for the %1 action.")
                 .arg(tr("short jump")), true, false);

    setWhatsThis(seek2_minutes_spin, tr("Medium jump"),
                 tr("Set the time to seek forward or rewind for the %1 action.")
                 .arg(tr("medium jump")));
    setWhatsThis(seek2_seconds_spin, tr("Medium jump"),
                 tr("Set the time to seek forward or rewind for the %1 action.")
                 .arg(tr("medium jump")), true, false);

    setWhatsThis(seek3_minutes_spin, tr("Long jump"),
                 tr("Set the time to seek forward or rewind for the %1 action.")
                 .arg(tr("long jump")));
    setWhatsThis(seek3_seconds_spin, tr("Long jump"),
                 tr("Set the time to seek forward or rewind for the %1 action.")
                 .arg(tr("long jump")), true, false);

    setWhatsThis(seek4_minutes_spin, tr("Mouse wheel jump"),
        tr("Set the time to seek forward or rewind for the mouse wheel."));
    setWhatsThis(seek4_seconds_spin, tr("Mouse wheel jump"),
        tr("Set the time to seek forward or rewind for the mouse wheel."));

    setWhatsThis(seek_rate_spin, tr("Drag rate"),
        tr("Rate limit the number of seeks send to the player, while dragging"
           " the time slider or previewing, to once every specified number of"
           " milliseconds."));

    setWhatsThis(seeking_method_group, tr("Seeking method"),
        tr("Sets the method to be used for seeking while dragging the"
           " time slider or previewing a video.<br/><br/>"

           "<b>Seek with time stamp</b> seeks with an absolute time stamp and"
           " tends to be the most accurate.<br/>"
           "<b>Seek with percentage of duration</b> uses a percentage to seek,"
           " like seek to 10.53% of the video, and might help out for streams"
           " with misunderstood or corrupted time stamps."));

    setWhatsThis(seek_keyframes_check, tr("Seek to key frames"),
        tr("If checked, seeks are done to the nearest key frame, which speeds"
           " up seeking considerably, but is less accurate, especially in"
           " heavely compressed videos with only a few keyframes.<br/><br/>"
           "If unchecked, seeks are a best attempt to seek to the requested"
           " absolute or relative time, but take quite a bit longer, depending"
           " on how well the playing media supports seeking."));

    setWhatsThis(seek_preview_check,
                 tr("Show preview when hoovering the time slider"),
                 tr("Show a preview, for local files, when hoovering over the"
                    " time slider if enabled.<br/>"
                    "Disable it when you have performance issues."));
}

}} // namespace Gui::Pref

#include "moc_input.cpp"
