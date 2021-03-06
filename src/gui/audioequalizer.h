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


#ifndef GUI_AUDIOOEQUALIZER_H
#define GUI_AUDIOOEQUALIZER_H

#include <QWidget>
#include <QHideEvent>
#include <QShowEvent>

#include "log4qt/logger.h"
#include "settings/preferences.h"

class QLabel;
class QComboBox;
class QPushButton;

namespace Gui {

class TEqSlider;

class TAudioEqualizer : public QWidget {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    enum Preset { User_defined = 0, Flat = 1, Pop = 2, Rock = 3, Classical = 4, Club = 5, Dance = 6, Fullbass = 7,
                  FullbassTreble = 8, Fulltreble = 9, Headphones = 10, LargeHall = 11, Live = 12,
                  Party = 13, Reggae = 14, Ska = 15, Soft = 16, SoftRock = 17, Techno = 18 };

    TAudioEqualizer(QWidget* parent = 0, Qt::WindowFlags f = Qt::Dialog);

    TEqSlider* eq[10];

    void setEqualizer(const Settings::TAudioEqualizerList& l);

signals:
    void visibilityChanged(bool visible);
    void applyClicked(const Settings::TAudioEqualizerList& new_values);
    void valuesChanged(const Settings::TAudioEqualizerList& values);

public slots:
    void reset();
    void setDefaults();

protected:
    QLabel* presets_label;
    QComboBox* presets_combo;
    QPushButton* apply_button;
    QPushButton* reset_button;
    QPushButton* set_default_button;
    QMap<int,Settings::TAudioEqualizerList> preset_list;

    virtual void hideEvent(QHideEvent*);
    virtual void showEvent(QShowEvent*);
    virtual void changeEvent(QEvent* event);
    virtual void retranslateStrings();

    void createPresets();
    void setValues(const Settings::TAudioEqualizerList& l, bool emitValuesChanged = true);
    int findPreset(const Settings::TAudioEqualizerList& l);

protected slots:
    void applyButtonClicked();
    void presetChanged(int index);
    void updatePresetCombo();
};

} // namespace Gui

#endif // GUI_AUDIOOEQUALIZER_H
