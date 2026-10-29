#pragma once

#include "terrarium_types.h"

namespace network {

bool connectToWifi();
bool fetchSetpoints(Setpoints &setpoints);
bool sendMeasurements(const Measurements &measurements);
bool sendOutput(float output);

} // namespace network
