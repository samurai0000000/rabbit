/*
 * rabbit.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef RABBIT_HXX
#define RABBIT_HXX

#include <pigpio.h>
#include "camera.hxx"
#include "wheels.hxx"
#include "power.hxx"
#include "compass.hxx"
#include "ambience.hxx"

extern Camera *camera;
extern Wheels *wheels;
extern Power *power;
extern Compass *compass;
extern Ambience *ambience;

#endif

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
