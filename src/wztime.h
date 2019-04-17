#ifndef WZTIME_H
#define WZTIME_H

#include <QString>


class TWZTime {
public:
    // Format time as hh:mm:ss
    static QString formatSec(int secs);
    static QString formatMS(int ms);
    static QString formatTimeMS(const double& aSecs);
    static QString formatTimeStampMS(int ms);
};


#endif // WZTIME_H
