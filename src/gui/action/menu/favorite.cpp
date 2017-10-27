#include "gui/action/menu/favorite.h"

namespace Gui {
namespace Action {
namespace Menu {

TFavorite::TFavorite() : is_subentry(false) {
}

TFavorite::TFavorite(const QString& name,
                     const QString& file,
                     const QString& icon,
                     bool subentry) :
    _name(name),
    _file(file),
    _icon(icon),
    is_subentry(subentry) {
}

TFavorite::~TFavorite() {
}

} // namespace Menu
} // namespace Action
} // namespace Gui
