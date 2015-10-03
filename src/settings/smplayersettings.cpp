
#include "settings/smplayersettings.h"

namespace Settings {

TSMPlayerSettings::TSMPlayerSettings(const QString& filename, QObject* parent) :
	QSettings(filename, QSettings::IniFormat, parent) {
}


} // namespace Settings

