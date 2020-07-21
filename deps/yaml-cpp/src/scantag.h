#pragma once

#include <string>
#include "stream.h"

namespace YAML {
const std::string ScanVerbatimTag(Stream& INPUT);
const std::string ScanTagHandle(Stream& INPUT, bool& canBeHandle);
const std::string ScanTagSuffix(Stream& INPUT);
}
