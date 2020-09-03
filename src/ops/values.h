#pragma once

inline float map(float value, float inputMin, float inputMax, float outputMin, float outputMax) {
    float outVal = ((value - inputMin) / (inputMax - inputMin) * (outputMax - outputMin) + outputMin);
    return outVal;
}

inline float lerp(float start, float stop, float amt) {
	return start + (stop-start) * amt;
}