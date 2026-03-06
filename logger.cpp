#include "logger.h"

// Static members definition
Logger* Logger::instance = nullptr;
std::mutex Logger::instanceMutex;
