#ifndef CONFIG_H
#define CONFIG_H

#include <QString>


class TConfig {
public:
	static const int MESSAGE_DURATION;
	static const double ZOOM_MIN;
	static const double ZOOM_MAX;

	static const QString PROGRAM_ID;
	static const QString PROGRAM_NAME;

	static const QString URL_HOMEPAGE;
	static const QString URL_ISSUES;
	static const QString URL_TRANSLATORS;
	static const QString URL_CHANGES;
	static const QString URL_OPENSSL;
	static const QString URL_VERSION_INFO;
	static const QString URL_SMPLAYER;
};


#endif // CONFIG_H
