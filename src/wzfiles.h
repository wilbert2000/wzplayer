#ifndef WZFILES_H
#define WZFILES_H

#include <QString>


class TWZFiles {
public:
    static bool directoryContainsDVD(const QString& directory);
    static bool directoryIsEmpty(const QString& directory);
    // Tries to find the executable in the path.
    // Returns the path if found or empty string if not.
    static QString findExecutable(const QString& name);
};

#endif // WZFILES_H
