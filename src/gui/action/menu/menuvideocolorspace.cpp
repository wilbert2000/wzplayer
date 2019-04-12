#include "gui/action/menu/menuvideocolorspace.h"
#include "gui/mainwindow.h"
#include "settings/mediasettings.h"


namespace Gui {
namespace Action {
namespace Menu {

TColorSpaceGroup::TColorSpaceGroup(TMainWindow* mw) :
    TActionGroup(mw, "colorspace") {

    setEnabled(false);

    using namespace Settings;

    new TActionGroupItem(mw, this, "color_auto",
        TMediaSettings::getColorSpaceDescriptionString(
            TMediaSettings::TColorSpace::COLORSPACE_AUTO),
            TMediaSettings::TColorSpace::COLORSPACE_AUTO);
    new TActionGroupItem(mw, this, "color_bt_601",
        TMediaSettings::getColorSpaceDescriptionString(
            TMediaSettings::TColorSpace::COLORSPACE_BT_601),
            TMediaSettings::TColorSpace::COLORSPACE_BT_601);
    new TActionGroupItem(mw, this, "color_bt_709",
        TMediaSettings::getColorSpaceDescriptionString(
            TMediaSettings::TColorSpace::COLORSPACE_BT_709),
            TMediaSettings::TColorSpace::COLORSPACE_BT_709);
    new TActionGroupItem(mw, this, "color_smpte_240m",
        TMediaSettings::getColorSpaceDescriptionString(
            TMediaSettings::TColorSpace::COLORSPACE_SMPTE_240M),
            TMediaSettings::TColorSpace::COLORSPACE_SMPTE_240M);
    new TActionGroupItem(mw, this, "color_bt_2020_ncl",
        TMediaSettings::getColorSpaceDescriptionString(
            TMediaSettings::TColorSpace::COLORSPACE_BT_2020_NCL),
            TMediaSettings::TColorSpace::COLORSPACE_BT_2020_NCL);
    new TActionGroupItem(mw, this, "color_bt_2020_cl",
        TMediaSettings::getColorSpaceDescriptionString(
            TMediaSettings::TColorSpace::COLORSPACE_BT_2020_CL),
            TMediaSettings::TColorSpace::COLORSPACE_BT_2020_CL);
    new TActionGroupItem(mw, this, "color_rgb",
        TMediaSettings::getColorSpaceDescriptionString(
            TMediaSettings::TColorSpace::COLORSPACE_RGB),
            TMediaSettings::TColorSpace::COLORSPACE_RGB);
    new TActionGroupItem(mw, this, "color_xyz",
        TMediaSettings::getColorSpaceDescriptionString(
            TMediaSettings::TColorSpace::COLORSPACE_XYZ),
            TMediaSettings::TColorSpace::COLORSPACE_XYZ);
    new TActionGroupItem(mw, this, "color_ycgco",
        TMediaSettings::getColorSpaceDescriptionString(
            TMediaSettings::TColorSpace::COLORSPACE_YCGCO),
            TMediaSettings::TColorSpace::COLORSPACE_YCGCO);

    setChecked(TMediaSettings::TColorSpace::COLORSPACE_AUTO);
}

TMenuVideoColorSpace::TMenuVideoColorSpace(QWidget* parent, TMainWindow* mw) :
    TMenu(parent, "colorspace_menu", tr("Color space")) {

    TColorSpaceGroup* group = mw->findChild<TColorSpaceGroup*>("colorspace");
    addActions(group->actions());
}

} // namespace Menu
} // namespace Action
} // namespace Gui
