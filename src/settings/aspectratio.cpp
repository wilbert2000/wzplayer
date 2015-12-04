#include "settings/aspectratio.h"
#include <QString>
#include <QVariant>

namespace Settings {

// List of aspect ratios
const double TAspectRatio::RATIOS[] = {
	1.0,
	(double) 5 / 4,   // 1.2
	(double) 4 / 3,   // 1.3r
	(double) 11 / 8,  // 1.375
	(double) 14 / 10, // 1.4
	(double) 3 / 2,   // 1.5
	(double) 14 / 9,  // 1.5r
	(double) 16 / 10, // 1.6
	(double) 16 / 9,  // 1.7r
	2.35
};

const char* TAspectRatio::RATIO_NAMES[] = {
	QT_TR_NOOP("1&:1"),
	QT_TR_NOOP("&5:4"),
	QT_TR_NOOP("&4:3"),
	QT_TR_NOOP("11:&8"),
	QT_TR_NOOP("1&4:10"),
	QT_TR_NOOP("&3:2"),
	QT_TR_NOOP("&14:9"),
	QT_TR_NOOP("1&6:10"),
	QT_TR_NOOP("16:&9"),
	QT_TR_NOOP("&2.35:1")
};


TAspectRatio::TMenuID TAspectRatio::toTMenuID(const QVariant& id) {

	int i = id.toInt();
	if (i >= 0 && i <= MAX_MENU_ID) {
		return (TMenuID) i;
	}
	return AspectAuto;
}

QString TAspectRatio::doubleToString(double aspect) {

	for(unsigned int i = 0; i < RATIOS_COUNT; i++) {
		if (aspect == RATIOS[i]) {
			QString s = RATIO_NAMES[i];
			s.replace("&", "");
			return tr("Aspect ratio %1 (%2)").arg(s, QString::number(aspect));
		}
	}
	return QString::number(aspect);
}

TAspectRatio::TAspectRatio()
	: QObject()
	, id(AspectAuto) {
}

TAspectRatio::TMenuID TAspectRatio::nextMenuID() const {

	if (id == MAX_MENU_ID)
		return AspectNone;
	return static_cast<TMenuID>(id + 1);
}

double TAspectRatio::toDouble(int w, int h) const {

	double asp;

	switch (id) {
		case AspectNone: asp = 0; break;
		case Aspect43: asp = (double) 4 / 3; break;
		case Aspect169: asp = (double) 16 / 9; break;
		case Aspect149: asp = (double) 14 / 9; break;
		case Aspect1610: asp = (double) 16 / 10; break;
		case Aspect54: asp = (double) 5 / 4; break;
		case Aspect235: asp = 2.35; break;
		case Aspect11: asp = 1; break;
		case Aspect32: asp = (double) 3 / 2; break;
		case Aspect1410: asp = (double) 14 / 10; break;
		case Aspect118: asp = (double) 11 / 8; break;
		default: asp = (double) w / h;
	}

	return asp;
}

QString TAspectRatio::toString() const {

	QString name;

	switch (id) {
		case AspectNone: name = tr("disabled"); break;
		case Aspect43: name = "4:3"; break;
		case Aspect169: name = "16:9"; break;
		case Aspect149: name = "14:9"; break;
		case Aspect1610: name = "16:10"; break;
		case Aspect54: name = "5:4"; break;
		case Aspect235: name = "2.35:1"; break;
		case Aspect11: name = "1:1"; break;
		case Aspect32: name = "3:2"; break;
		case Aspect1410: name = "14:10"; break;
		case Aspect118: name = "11:8"; break;
		case AspectAuto: name = tr("auto"); break;
		default: name = tr("unknown");
	}

	return name;
}

} // namespace Settings
