#ifndef ZEROCONF_UTIL_HPP
#define ZEROCONF_UTIL_HPP

//////////////////////////////////////////////////////////////////////////
// zeroconf-util.hpp

// (C) Copyright 2016 Yuri Yakovlev <yvzmail@gmail.com>
// Use, modification and distribution is subject to the GNU General Public License

#include <string>
#include <istream>
#include <streambuf>

#if _MSC_VER == 1700
#define thread_local __declspec(thread)
#endif

namespace Zeroconf
{
    namespace Detail
    {
        namespace stdext
        {
            class membuf : public std::streambuf
            {
            public: 
                membuf(const unsigned char* base, int size) 
                {
                    auto p = reinterpret_cast<char*>(const_cast<unsigned char*>(base));
                    setg(p, p, p + size);
                }

            protected:
                virtual pos_type seekoff(off_type offset, std::ios_base::seekdir, std::ios_base::openmode mode) override
                {
                    if (offset == 0 && mode == std::ios_base::in)
                        return pos_type(gptr() - eback());

                    return pos_type(-1);
                }
            };
        }

        namespace Log
        {
            enum class LogLevel { Error, Warning };

            typedef void(*LogCallback)(LogLevel, const std::string&);

            static thread_local LogCallback g_logcb = nullptr;

            inline void Error(const std::string& message) 
            {
                if (g_logcb != nullptr)
                    g_logcb(LogLevel::Error, message);
            }

            inline void Warning(const std::string& message) 
            {
                if (g_logcb != nullptr)
                    g_logcb(LogLevel::Warning, message);
            }

            inline void SetLogCallback(LogCallback logcb)
            {
                g_logcb = logcb;
            }
        }
    }
}

#endif // ZEROCONF_UTIL_HPP