#ifndef ZEROCONF_HPP
#define ZEROCONF_HPP

//////////////////////////////////////////////////////////////////////////
// zeroconf.hpp

// (C) Copyright 2016 Yuri Yakovlev <yvzmail@gmail.com>
// Use, modification and distribution is subject to the GNU General Public License

#include <ctime>
#include <string>
#include <vector>

#include "zeroconf-util.hpp"
#include "zeroconf-detail.hpp"

namespace Zeroconf
{    
    typedef Detail::Log::LogLevel LogLevel;
    typedef Detail::Log::LogCallback LogCallback;
    typedef Detail::mdns_responce mdns_responce;

    inline bool Resolve(const std::string& serviceName, time_t scanTime, std::vector<mdns_responce>* result)
    {
        return Detail::Resolve(serviceName, scanTime, result);
    }

    inline void SetLogCallback(LogCallback callback)
    {
        Detail::Log::SetLogCallback(callback);
    }
}

#endif // ZEROCONF_HPP