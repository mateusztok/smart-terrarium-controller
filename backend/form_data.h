#pragma once

#include <string>

namespace terrarium {

std::string getFormValue(const std::string &body, const std::string &key);
float parseNumericValue(const std::string &value);

} // namespace terrarium
