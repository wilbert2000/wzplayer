#ifndef SETTINGS_LRULIST_H
#define SETTINGS_LRULIST_H

#include <QStringList>


namespace Settings {

class TLRUList : public QStringList {
public:
    TLRUList();
    virtual ~TLRUList();

    int getMaxItems() const { return maxItems; }
    void setMaxItems(int aMaxItems);

    void add(const QString& s);
    void fromStringList(const QStringList& list);

private:
    int maxItems;
};

} // namespace Settings

#endif // SETTINGS_LRULIST_H
