#include "form_data.h"

#include <cmath>
#include <stdexcept>

namespace terrarium {
namespace {

int hexDigit(char digit) {
  if (digit >= '0' && digit <= '9') {
    return digit - '0';
  }
  if (digit >= 'A' && digit <= 'F') {
    return digit - 'A' + 10;
  }
  if (digit >= 'a' && digit <= 'f') {
    return digit - 'a' + 10;
  }
  return -1;
}

std::string decodeFormValue(const std::string &value) {
  std::string decoded;
  decoded.reserve(value.size());

  for (std::size_t index = 0; index < value.size(); ++index) {
    if (value[index] == '+') {
      decoded.push_back(' ');
      continue;
    }

    if (value[index] != '%') {
      decoded.push_back(value[index]);
      continue;
    }

    if (index + 2 >= value.size()) {
      throw std::invalid_argument("Malformed URL-encoded form value.");
    }

    const int high = hexDigit(value[index + 1]);
    const int low = hexDigit(value[index + 2]);
    if (high < 0 || low < 0) {
      throw std::invalid_argument("Malformed URL-encoded form value.");
    }

    decoded.push_back(static_cast<char>((high << 4) | low));
    index += 2;
  }

  return decoded;
}

} // namespace

std::string getFormValue(const std::string &body, const std::string &key) {
  std::size_t start = 0;
  while (start <= body.size()) {
    const std::size_t end = body.find('&', start);
    const std::string field = body.substr(
        start, end == std::string::npos ? std::string::npos : end - start);
    const std::size_t separator = field.find('=');
    if (separator != std::string::npos &&
        decodeFormValue(field.substr(0, separator)) == key) {
      return decodeFormValue(field.substr(separator + 1));
    }

    if (end == std::string::npos) {
      break;
    }
    start = end + 1;
  }

  return {};
}

float parseNumericValue(const std::string &value) {
  std::size_t parsedLength = 0;
  const float number = std::stof(value, &parsedLength);
  if (parsedLength != value.size() || !std::isfinite(number)) {
    throw std::invalid_argument("Value must be a finite number.");
  }
  return number;
}

} // namespace terrarium
