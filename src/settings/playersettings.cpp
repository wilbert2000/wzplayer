#include "settings/playersettings.h"

namespace Settings {

TPlayerSettings::TPlayerSettings(const QString& filename) :
	QSettings(filename, QSettings::IniFormat) {
}

TPlayerSettings::~TPlayerSettings() {
}

} // namespace Settings

