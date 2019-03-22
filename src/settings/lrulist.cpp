#include "settings/lrulist.h"
#include "wzdebug.h"


namespace Settings {

LOG4QT_DECLARE_STATIC_LOGGER(logger, Settings::TLRUList)

TLRUList::TLRUList() :
    maxItems(10) {
}

void TLRUList::setMaxItems(int aMaxItems) {

    maxItems = aMaxItems;
    while (count() > maxItems) {
        removeLast();
    }
}

void TLRUList::add(const QString& s) {
    WZDEBUG("'" + s + "'");

    int i = indexOf(s);
    if (i >= 0) {
        removeAt(i);
    }
    prepend(s);

    if (count() > maxItems) {
        removeLast();
    }
}

void TLRUList::fromStringList(const QStringList& list) {

    clear();

    int max = list.count();
    if (max > maxItems) {
        max = maxItems;
    }

    for (int n = 0; n < max; n++) {
        append(list[n]);
    }
}

} // namespace Settings
