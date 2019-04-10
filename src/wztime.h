#ifndef WZTIME_H
#define WZTIME_H

#include <QString>


class TWZTime {
public:
    // Format time as hh:mm:ss
    static QString formatTimeSec(int secs);
    static QString formatTimeMS(const double& aSecs,
                                bool wantMinutes = false,
                                bool zeroPadded = false);
};


#endif // WZTIME_H
