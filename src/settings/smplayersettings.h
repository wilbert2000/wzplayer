#ifndef _SETTINGS_SMPLAYERSETTINGS_
#define _SETTINGS_SMPLAYERSETTINGS_

#include <QString>
#include <QSettings>

namespace Settings {

class TSMPlayerSettings : public QSettings {
public:
	TSMPlayerSettings(const QString& filename, QObject* parent);
	virtual ~TSMPlayerSettings() {}
};

} // namespace Settings

#endif // _SETTINGS_SMPLAYERSETTINGS_
