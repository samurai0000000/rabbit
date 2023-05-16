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
#include "head.hxx"
#include "wifi.hxx"
#include "speech.hxx"
#include "websock.hxx"
#include "logging.hxx"
#include "crond.hxx"

extern Servos *servos;
extern ADC *adc;
extern Camera *camera;
extern Wheels *wheels;
extern Arm *rightArm;
extern Arm *leftArm;
extern Power *power;
extern Compass *compass;
extern Ambience *ambience;
extern Head *head;
extern WIFI *wifi;
extern Speech *speech;
extern Crond *crond;

extern "C" void rabbit_keycontrol(uint8_t key);

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
