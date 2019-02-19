#include "gui/action/menu/menuvideocolorspace.h"
#include "gui/mainwindow.h"
#include "settings/mediasettings.h"
#include "player/player.h"


namespace Gui {
namespace Action {
namespace Menu {

TColorSpaceGroup::TColorSpaceGroup(TMainWindow* mw) :
    TActionGroup(mw, "colorspace") {

    setEnabled(false);

    new TActionGroupItem(mw, this, "colorspace_auto",
        Settings::TMediaSettings::getColorSpaceDescriptionString(
            Settings::TMediaSettings::TColorSpace::COLORSPACE_AUTO),
            Settings::TMediaSettings::TColorSpace::COLORSPACE_AUTO);
    new TActionGroupItem(mw, this, "colorspace_bt_601",
        Settings::TMediaSettings::getColorSpaceDescriptionString(
            Settings::TMediaSettings::TColorSpace::COLORSPACE_BT_601),
            Settings::TMediaSettings::TColorSpace::COLORSPACE_BT_601);
    new TActionGroupItem(mw, this, "colorspace_bt_709",
        Settings::TMediaSettings::getColorSpaceDescriptionString(
            Settings::TMediaSettings::TColorSpace::COLORSPACE_BT_709),
            Settings::TMediaSettings::TColorSpace::COLORSPACE_BT_709);
    new TActionGroupItem(mw, this, "colorspace_smpte_240m",
        Settings::TMediaSettings::getColorSpaceDescriptionString(
            Settings::TMediaSettings::TColorSpace::COLORSPACE_SMPTE_240M),
            Settings::TMediaSettings::TColorSpace::COLORSPACE_SMPTE_240M);
    new TActionGroupItem(mw, this, "colorspace_bt_2020_ncl",
        Settings::TMediaSettings::getColorSpaceDescriptionString(
            Settings::TMediaSettings::TColorSpace::COLORSPACE_BT_2020_NCL),
            Settings::TMediaSettings::TColorSpace::COLORSPACE_BT_2020_NCL);
    new TActionGroupItem(mw, this, "colorspace_bt_2020_cl",
        Settings::TMediaSettings::getColorSpaceDescriptionString(
            Settings::TMediaSettings::TColorSpace::COLORSPACE_BT_2020_CL),
            Settings::TMediaSettings::TColorSpace::COLORSPACE_BT_2020_CL);
    new TActionGroupItem(mw, this, "colorspace_rgb",
        Settings::TMediaSettings::getColorSpaceDescriptionString(
            Settings::TMediaSettings::TColorSpace::COLORSPACE_RGB),
            Settings::TMediaSettings::TColorSpace::COLORSPACE_RGB);
    new TActionGroupItem(mw, this, "colorspace_xyz",
        Settings::TMediaSettings::getColorSpaceDescriptionString(
            Settings::TMediaSettings::TColorSpace::COLORSPACE_XYZ),
            Settings::TMediaSettings::TColorSpace::COLORSPACE_XYZ);
    new TActionGroupItem(mw, this, "colorspace_ycgco",
        Settings::TMediaSettings::getColorSpaceDescriptionString(
            Settings::TMediaSettings::TColorSpace::COLORSPACE_YCGCO),
            Settings::TMediaSettings::TColorSpace::COLORSPACE_YCGCO);

    setChecked(Settings::TMediaSettings::TColorSpace::COLORSPACE_AUTO);
}

TMenuVideoColorSpace::TMenuVideoColorSpace(TMainWindow* mw) :
    TMenu(mw, mw, "colorspace_menu", tr("Color space")) {

    group = new TColorSpaceGroup(mw);
    connect(group, &TColorSpaceGroup::activated,
            player, &Player::TPlayer::setColorSpace);
    addActions(group->actions());
}

void TMenuVideoColorSpace::enableActions() {

    // Using mset, so useless if stopped or no video
    group->setEnabled(Settings::pref->isMPV()
                      && player->statePOP()
                      && player->hasVideo());
    group->setChecked(player->mset.color_space);
}

void TMenuVideoColorSpace::onMediaSettingsChanged(Settings::TMediaSettings* m) {
    group->setChecked(m->color_space);
}

} // namespace Menu
} // namespace Action
} // namespace Gui
