#pragma once
#include <string>

namespace utils
{

class ComputerUUID
{
    bool        _uniquePerProcess = false;
    std::string _customData;

public:
    ComputerUUID() = default;

    std::string getUUID();

    void setCustomData(std::string_view v) { _customData = v; }
    std::string customData() const { return _customData; }

    // a different UUID wil be generated per each 
    // instance of the process
    void setUniquePerProcess(bool v) { _uniquePerProcess = v; }
    bool UniquePerProcess() const { return _uniquePerProcess; }
    
};

} // namespace utils