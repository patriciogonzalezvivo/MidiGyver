#pragma once

#include <time.h>
#include <sys/time.h>

#include "nodes.h"

#define MS_MINUTE 60000
#define MS_SECOND 1000

inline bool getTickDuration(const YAML::Node& _node, int *_milliSecs) {
    if (_node["bpm"].IsDefined())
        *_milliSecs = MS_MINUTE/_node["bpm"].as<int>();
    else if (_node["fps"].IsDefined()) 
        *_milliSecs = int(MS_SECOND/_node["fps"].as<float>());
    else if (_node["tick"].IsDefined()) 
        *_milliSecs = int(_node["tick"].as<float>());
    else if (_node["tick_ms"].IsDefined()) 
        *_milliSecs = int(_node["tick_ms"].as<float>());
    else if (_node["tick_sec"].IsDefined()) 
        *_milliSecs = int(_node["tick_sec"].as<float>() * MS_SECOND);
    else if (_node["interval"].IsDefined()) 
        *_milliSecs = int(_node["interval"].as<float>());
    else
        return false;

    return true;
}


inline double getTimeSec(const timespec &time_start) {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    timespec temp;
    if ((now.tv_nsec-time_start.tv_nsec)<0) {
        temp.tv_sec = now.tv_sec-time_start.tv_sec-1;
        temp.tv_nsec = 1000000000+now.tv_nsec-time_start.tv_nsec;
    }
    else {
        temp.tv_sec = now.tv_sec-time_start.tv_sec;
        temp.tv_nsec = now.tv_nsec-time_start.tv_nsec;
    }
    return double(temp.tv_sec) + double(temp.tv_nsec/1000000000.);
}

inline int getTimeMs(const timespec &time_start) { return getTimeSec(time_start) * MS_SECOND; }