#ifndef WZTIME_H
#define WZTIME_H

#include <QString>


class TWZTime {
public:
    // Format time as hh:mm:ss
    static QString formatTime(int secs);
    static QString formatTimeMS(int ms);
};


#endif // WZTIME_H
