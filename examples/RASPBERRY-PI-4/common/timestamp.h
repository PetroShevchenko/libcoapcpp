#ifndef _TIMESTAMP_H
#define _TIMESTAMP_H
#include <ctime>

inline double get_timestamp()
{
    time_t currentTime = time(nullptr);
    return static_cast<double>(mktime(localtime(&currentTime)));
}

#endif
