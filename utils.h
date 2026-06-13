#pragma once
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

const std::string currentDateTime();

#ifdef _DEBUG
#define DEBUG_LOG(x, file)  file << currentDateTime() << ": " << x << std::endl;
#else
#define DEBUG_LOG(x) 
#endif

