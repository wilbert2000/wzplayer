#include "gui/action/menu/menuvideocolorspace.h"
#include "gui/mainwindow.h"
#include "settings/mediasettings.h"
#include "player/player.h"


namespace Gui {
namespace Action {
namespace Menu {

TColorSpaceGroup::TColorSpaceGroup(QWidget* parent) :
    TActionGroup(parent, "colorspace") {

    setEnabled(false);

    new TActionGroupItem(this, this, "video_colorspace_auto",
        Settings::TMediaSettings::getColorSpaceDescriptionString(
            Settings::TMediaSettings::TColorSpace::COLORSPACE_AUTO),
            Settings::TMediaSettings::TColorSpace::COLORSPACE_AUTO,
            false);
    new TActionGroupItem(this, this, "video_colorspace_bt_601",
        Settings::TMediaSettings::getColorSpaceDescriptionString(
            Settings::TMediaSettings::TColorSpace::COLORSPACE_BT_601),
            Settings::TMediaSettings::TColorSpace::COLORSPACE_BT_601,
            false);
    new TActionGroupItem(this, this, "video_colorspace_bt_709",
        Settings::TMediaSettings::getColorSpaceDescriptionString(
            Settings::TMediaSettings::TColorSpace::COLORSPACE_BT_709),
            Settings::TMediaSettings::TColorSpace::COLORSPACE_BT_709,
            false);
    new TActionGroupItem(this, this, "video_colorspace_smpte_240m",
        Settings::TMediaSettings::getColorSpaceDescriptionString(
            Settings::TMediaSettings::TColorSpace::COLORSPACE_SMPTE_240M),
            Settings::TMediaSettings::TColorSpace::COLORSPACE_SMPTE_240M,
            false);
    new TActionGroupItem(this, this, "video_colorspace_bt_2020_ncl",
        Settings::TMediaSettings::getColorSpaceDescriptionString(
            Settings::TMediaSettings::TColorSpace::COLORSPACE_BT_2020_NCL),
            Settings::TMediaSettings::TColorSpace::COLORSPACE_BT_2020_NCL,
            false);
    new TActionGroupItem(this, this, "video_colorspace_bt_2020_cl",
        Settings::TMediaSettings::getColorSpaceDescriptionString(
            Settings::TMediaSettings::TColorSpace::COLORSPACE_BT_2020_CL),
            Settings::TMediaSettings::TColorSpace::COLORSPACE_BT_2020_CL,
            false);
    new TActionGroupItem(this, this, "video_colorspace_rgb",
        Settings::TMediaSettings::getColorSpaceDescriptionString(
            Settings::TMediaSettings::TColorSpace::COLORSPACE_RGB),
            Settings::TMediaSettings::TColorSpace::COLORSPACE_RGB,
            false);
    new TActionGroupItem(this, this, "video_colorspace_xyz",
        Settings::TMediaSettings::getColorSpaceDescriptionString(
            Settings::TMediaSettings::TColorSpace::COLORSPACE_XYZ),
            Settings::TMediaSettings::TColorSpace::COLORSPACE_XYZ,
            false);
    new TActionGroupItem(this, this, "video_colorspace_ycgco",
        Settings::TMediaSettings::getColorSpaceDescriptionString(
            Settings::TMediaSettings::TColorSpace::COLORSPACE_YCGCO),
            Settings::TMediaSettings::TColorSpace::COLORSPACE_YCGCO,
            false);

    setChecked(Settings::TMediaSettings::TColorSpace::COLORSPACE_AUTO);
}

TMenuVideoColorSpace::TMenuVideoColorSpace(TMainWindow* mw) :
    TMenu(mw, mw, "video_colorspace_menu", tr("&Color space"),
          "video_colorspace") {

    group = new TColorSpaceGroup(this);
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
