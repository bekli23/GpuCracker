// Windows conflict fix - MUST be first
#include "win_fix.h"

#include "logger.h"

// Static members definition
Logger* Logger::instance = nullptr;
std::mutex Logger::instanceMutex;
