#ifndef GUI_ACTION_MENU_FAVORITE_H
#define GUI_ACTION_MENU_FAVORITE_H

#include <QString>


namespace Gui {
namespace Action {
namespace Menu {

class TFavorite {
public:
    TFavorite();
    TFavorite(const QString& name,
              const QString& file,
              const QString& icon = QString::null,
              bool subentry = false);
    virtual ~TFavorite();

    void setName(const QString& name) { _name = name; }
    void setFile(const QString& file) { _file = file; }
    void setIcon(const QString& file) { _icon = file; }
    void setSubentry(bool b) { is_subentry = b; }

    QString name() const { return _name; }
    QString file() const { return _file; }
    QString icon() const { return _icon; }
    bool isSubentry() const { return is_subentry; }

private:
    QString _name, _file, _icon;
    bool is_subentry; // Not a favorite file, but a new favorite list
};

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_FAVORITE_H
