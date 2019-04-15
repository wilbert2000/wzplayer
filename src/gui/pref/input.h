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

#ifndef GUI_PREF_INPUT_H
#define GUI_PREF_INPUT_H

#include "gui/pref/section.h"
#include "settings/preferences.h"
#include "ui_input.h"


namespace Gui {

class TMainWindow;

namespace Pref {

class TInput : public TSection, public Ui::TInput {
    Q_OBJECT
public:
    TInput(QWidget* parent);

    virtual QString sectionName();
    virtual QPixmap sectionIcon();

    // Pass data to the dialog
    void setData(Settings::TPreferences* pref);
    // Apply changes
    virtual void getData(Settings::TPreferences* pref);

private:
    void addActionItem(const QString& name);
    void createMouseCombos();
    void createHelp();

    void setLeftClickFunction(const QString& f);
    QString leftClickFunction();

    void setRightClickFunction(const QString& f);
    QString rightClickFunction();

    void setDoubleClickFunction(const QString& f);
    QString doubleClickFunction();

    void setMiddleClickFunction(const QString& f);
    QString middleClickFunction();

    void setXButton1ClickFunction(const QString& f);
    QString xButton1ClickFunction();

    void setXButton2ClickFunction(const QString& f);
    QString xButton2ClickFunction();

    void setWheelFunction(int function);
    int wheelFunction();

    void setWheelFunctionCycle(Settings::TPreferences::TWheelFunctions flags);
    Settings::TPreferences::TWheelFunctions wheelFunctionCycle();

    void setWheelFunctionSeekingReverse(bool b);
    bool wheelFunctionSeekingReverse();

    void setSeeking1(int n);
    int seeking1();

    void setSeeking2(int n);
    int seeking2();

    void setSeeking3(int n);
    int seeking3();

    void setSeeking4(int n);
    int seeking4();

    void setSeekRelative(bool);
    bool seekRelative();

    void setSeekKeyframes(bool);
    bool seekKeyframes();
}; // class TInput

} // namespace Pref
} // namespace Gui

#endif // GUI_PREF_INPUT_H
