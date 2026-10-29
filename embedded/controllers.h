#pragma once

#include "terrarium_types.h"

namespace controllers {

void setup();
void update(const Measurements &measurements, const Setpoints &setpoints);
float temperatureOutput();

} // namespace controllers
