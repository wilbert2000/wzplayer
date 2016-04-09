#ifndef SETTINGS_PLAYERSETTINGS
#define SETTINGS_PLAYERSETTINGS

#include <QString>
#include <QSettings>

namespace Settings {

class TPlayerSettings : public QSettings {
public:
	TPlayerSettings(const QString& filename);
	virtual ~TPlayerSettings();
};

} // namespace Settings

#endif // SETTINGS_PLAYERSETTINGS
