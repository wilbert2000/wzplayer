
#include "settings/cleanconfig.h"
#include <QFile>
#include <QDir>

#define DO_REMOVE

namespace Settings {

void TCleanConfig::clean(const QString& config_path) {

	QStringList files_to_delete;

	QString s = config_path + "/wzplayer.ini";
	if (QFile::exists(s)) files_to_delete << s;

	s = config_path + "/styles.ass";
	if (QFile::exists(s)) files_to_delete << s;

	s = config_path + "/wzplayer_files.ini";
	if (QFile::exists(s)) files_to_delete << s;

	s = config_path + "/ytcode.script";
	if (QFile::exists(s)) files_to_delete << s;

	s = config_path + "/yt.js";
	if (QFile::exists(s)) files_to_delete << s;
	
	s = config_path + "/player_info.ini";
	if (QFile::exists(s)) files_to_delete << s;

	s = config_path + "/file_settings";
	if (QFile::exists(s)) files_to_delete << listDir(s);

	printf("Deleting files:\n");
	for (int n = 0; n < files_to_delete.count(); n++) {
		printf("Delete: %s\n", files_to_delete[n].toUtf8().constData());
		#ifdef DO_REMOVE
		QFile::remove(files_to_delete[n]);
		#endif
	}
}

QStringList TCleanConfig::listDir(const QString& path) {

	QDir dir(path);
	QStringList file_list;

    foreach(const QString file, dir.entryList(QDir::Files)) {
		file_list << QFileInfo(dir, file).absoluteFilePath();
	}

    foreach(const QString sub_dir, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
		file_list << listDir(path +"/"+ sub_dir);
	}

	return file_list;
}

} // namespace Settings

