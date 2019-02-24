#ifndef GUI_ACTION_ACTIONITEM_H
#define GUI_ACTION_ACTIONITEM_H

#include <QListWidgetItem>


namespace Gui {
namespace Action {

class TActionItem : public QListWidgetItem {
public:
    static QList<QAction*> allActions;
    static QString iconDisplayText(QAction* action);
    static QString actionNameFromListWidgetItem(QListWidgetItem* item);

    TActionItem(QAction* act);
    TActionItem(const QString& actionName);
    TActionItem(const TActionItem& other);

    virtual TActionItem* clone() const override;

    virtual QVariant data(int role) const override;
    virtual void setData(int role, const QVariant& value) override;

    virtual void read(QDataStream& in) override;
    virtual void write(QDataStream& out) const override;

    QAction* getAction() const;
    QString getActionName() const;

    QString getIconText() const;
    void setIconText(const QString& s);

private:
    QAction* action;

    void init();
};

} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_ACTIONITEM_H
