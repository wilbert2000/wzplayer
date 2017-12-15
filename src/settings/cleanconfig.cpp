
#include "settings/cleanconfig.h"
#include "settings/paths.h"
#include <QFile>
#include <QDir>

#define DO_REMOVE

namespace Settings {

void TCleanConfig::clean() {

    QStringList files_to_delete;

    QString s = TPaths::iniFileName();
    if (QFile::exists(s)) files_to_delete << s;

    s = TPaths::subtitleStyleFileName();
    if (QFile::exists(s)) files_to_delete << s;

    s = TPaths::configPath() + "/wzplayer_files.ini";
    if (QFile::exists(s)) files_to_delete << s;

    s = TPaths::configPath() + "/player_info_version_3.ini";
    if (QFile::exists(s)) files_to_delete << s;

    s = TPaths::configPath() + "/file_settings";
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

