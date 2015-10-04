#include "settings/smplayersettings.h"

namespace Settings {

TSMPlayerSettings::TSMPlayerSettings(const QString& filename) :
	QSettings(filename, QSettings::IniFormat) {
}

TSMPlayerSettings::~TSMPlayerSettings() {
}

} // namespace Settings

