#ifndef WZTIME_H
#define WZTIME_H

#include <QString>


class TWZTime {
public:
    // Format time as hh:mm:ss
    static QString formatTimeSec(int secs);
    static QString formatMS(int ms,
                                      bool wantMinutes = false,
                                      bool zeroPadded = false);
    static QString formatTimeMS(const double& aSecs,
                                bool wantMinutes = false,
                                bool zeroPadded = false);
    static QString formatTimeStampMS(const double& secs);
    static QString formatDurationMS(const double& secs);
};


#endif // WZTIME_H
