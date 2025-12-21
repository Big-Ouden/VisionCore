#include "utils/Logger.hpp"
#include <stdio.h>

using namespace visioncore::utils;

int main() {

  LOG_DEBUG("debug log msg");
  LOG_INFO("info log msg");
  LOG_WARNING("warning log msg");
  LOG_ERROR("error log msg");
  LOG_CRITICAL("critical log msg");

  return 0;
}
