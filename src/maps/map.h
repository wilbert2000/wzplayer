#ifndef _MAPS_MAP_H
#define _MAPS_MAP_H

#include <QMap>

namespace Maps {

class TData {
public:
	TData(): ID(-1) {}
	virtual ~TData() {}

	int getID() const { return ID; }
	void setID(int id) { ID = id; }

protected:
	int ID;
};

template <class T>
class TMap : public QMap<int, T> {
public:
	TMap() : selectedID(-1) {}
	virtual ~TMap() {}

	typedef QMap<int, T> TQMap;
	typedef QMapIterator<int, T> TMapIterator;

	int getSelectedID() const { return selectedID; }
	void setSelectedID(int id) { selectedID = id; }

	TMapIterator getIterator() const { return TMapIterator(*this); }

	int firstID() const {
		TMapIterator i(*this);
		if (i.hasNext()) {
			return i.next().key();
		}
		return -1;
	}

	int previousID(int ID) const {

		typename TQMap::const_iterator i = TQMap::find(ID);
		if (i != TQMap::constEnd()) {
			if (i == TQMap::constBegin()) {
				i = TQMap::constEnd();
			}
			i--;
			return i.key();
		}
		// Not found
		if (TQMap::count() > 0) {
			i = TQMap::constEnd();
			i--;
			return i.key();
		}

		return -1;
	}

	int nextID(int id) const {
		typename TQMap::const_iterator i = TQMap::find(id);
		if (i == TQMap::constEnd()) {
			// Not found
			if (TQMap::count() > 0) {
				return TQMap::constBegin().key();
			}
			return -1;
		}
		// Found
		i++;
		if (i == TQMap::constEnd()) {
			return TQMap::constBegin().key();
		}

		return i.key();
	}

	void addID(int id) {
		T& data = (*this)[id];
		data.setID(id);
	}

protected:
	int selectedID;
};

} // namespace maps

#endif // _MAPS_MAP_H
