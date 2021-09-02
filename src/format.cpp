#include <string>
#include <iomanip>

#include "format.h"

using std::string;

// TODO: Complete this helper function
// INPUT: Long int measuring seconds
// OUTPUT: HH:MM:SS
// REMOVE: [[maybe_unused]] once you define the function
string Format::ElapsedTime(long seconds) 
{
    long int const hours{seconds / (60*60)};
    seconds = seconds % (60*60);
    long int const minutes{seconds / 60};
    seconds = seconds % 60;
    std::stringstream stream;
    stream << std::setfill('0') << std::setw(2) << std::to_string(hours) << ':' << std::setw(2) << std::to_string(minutes) << ':' << std::setw(2) << std::to_string(seconds) << std::endl; 
    return stream.str();
}