#include "utils.h"

const std::string currentDateTime() {
    using namespace std::chrono;

    auto now = system_clock::now();

    auto seconds = system_clock::to_time_t(now);

    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tstruct = *std::localtime(&seconds);

    std::ostringstream oss;

    oss << std::put_time(&tstruct, "%Y-%m-%d %H:%M:%S")
        << "." << std::setw(3) << std::setfill('0') << ms.count();

    return oss.str();
}