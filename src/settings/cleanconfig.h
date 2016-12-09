
#ifndef SETTINGS_CLEANCONFIG_H
#define SETTINGS_CLEANCONFIG_H

#include <QString>
#include <QStringList>


namespace Settings {

class TCleanConfig {
public:
    static void clean();

private:
	static QStringList listDir(const QString& path);
};

} // namespace Settings

#endif // SETTINGS_CLEANCONFIG_H
