/*
 * rabbit.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef RABBIT_HXX
#define RABBIT_HXX

#include <pigpio.h>
#include "servos.hxx"
#include "adc.hxx"
#include "camera.hxx"
#include "wheels.hxx"
#include "arms.hxx"
#include "power.hxx"
#include "compass.hxx"
#include "ambience.hxx"
#include "wifi.hxx"

extern Servos *servos;
extern ADC *adc;
extern Camera *camera;
extern Wheels *wheels;
extern Power *power;
extern Compass *compass;
extern Ambience *ambience;
extern WIFI *wifi;
extern Arm *rightArm;
extern Arm *leftArm;

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
