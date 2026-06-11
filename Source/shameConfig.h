//
//  shameConfig.h
//  KissOfShame
//
//  Created by Brian Hansen on 9/11/14.
//  Rev 2: stripped of hard-coded paths and sample-rate assumptions.
//  All DSP modules now learn the sample rate in prepare(); all resources
//  are embedded as BinaryData.
//

#pragma once

#include <JuceHeader.h>

enum EShameEnvironments
{
    eEnvironmentOff,
    eEnvironmentStudioCloset,
    eEnvironmentHumidCellar,
    eEnvironmentHotLocker,
    eEnvironmentHurricaneSandy,

    eEnvironmentTotalEnvironments
};
