#ifndef WZFILES_H
#define WZFILES_H

#include "settings/preferences.h"


class TWZFiles {
public:
    static bool directoryContainsDVD(const QString& directory);

    // Tries to find the executable in the path.
    // Returns the path if found or empty string if not.
    static QString findExecutable(const QString& name);

    static QStringList filesForPlaylist(
        const QString& initial_file,
        Settings::TPreferences::TAddToPlaylist filter);

private:
    static QStringList searchForConsecutiveFiles(const QString& initial_file);
    static QStringList filesInDirectory(const QString& initial_file,
                                        const QStringList& filter);
};

#endif // WZFILES_H
