#include "motion.h"

int motion_running(Uint32 start, Uint32 duration, Uint32 now)
{
    return now < start + duration;
}

float motion_progress(Uint32 start, Uint32 duration, Uint32 now)
{
    if (!motion_running(start, duration, now)) {
        return 1;
    }
    float t = (float)(now - start) / (float)duration;
    /* "Smoothstep": the curve 3t^2 - 2t^3 is flat at t = 0 and at t = 1. */
    return t * t * (3 - 2 * t);
}
