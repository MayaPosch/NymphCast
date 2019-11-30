#ifndef ZEROCONF_DETAIL_HPP
#define ZEROCONF_DETAIL_HPP

//////////////////////////////////////////////////////////////////////////
// zeroconf-detail.hpp

// (C) Copyright 2016 Yuri Yakovlev <yvzmail@gmail.com>
// Use, modification and distribution is subject to the GNU General Public License

#include <vector>
#include <memory>
#include <chrono>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#else
#include <error.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include "zeroconf-util.hpp"

namespace Zeroconf
{
    namespace Detail
    {
        const size_t MdnsMessageMaxLength = 512;
        const size_t MdnsRecordHeaderLength = 12;
    
        const uint8_t MdnsOffsetToken = 0xC0;
        const uint16_t MdnsResponseFlag = 0x8400;

        const uint32_t SockTrue = 1;

        const uint8_t MdnsQueryHeader[] = 
        {
            0x00, 0x00, // ID
            0x00, 0x00, // Flags
            0x00, 0x01, // QDCOUNT
            0x00, 0x00, // ANCOUNT
            0x00, 0x00, // NSCOUNT
            0x00, 0x00  // ARCOUNT
        };

        const uint8_t MdnsQueryFooter[] = 
        {
            0x00, 0x0c, // QTYPE
            0x00, 0x01  // QCLASS
        };

        struct raw_responce
        {
            sockaddr_storage peer;
            std::vector<uint8_t> data;
        };

        struct mdns_record
        {
            uint16_t type;
            size_t pos;
            size_t len;
            std::string name;
        };

        struct mdns_responce
        {
            sockaddr_storage peer;
            uint16_t qtype;
            std::string qname;
            std::vector<uint8_t> data;
            std::vector<mdns_record> records;
        };

        inline int GetSocketError()
        {
#ifdef WIN32
            return WSAGetLastError();
#else
            return errno;
#endif
        }

        inline void CloseSocket(int fd)
        {
#ifdef WIN32
            closesocket(fd);
#else
            close(fd);
#endif
        }

        inline void WriteFqdn(const std::string& name, std::vector<uint8_t>* result)
        {
            size_t len = 0;
            size_t pos = result->size();
            result->push_back(0);

            for (size_t i = 0; i < name.size(); i++)
            {
                if (name[i] != '.')
                {
                    result->push_back(name[i]);
                    len++;

                    if (len > UINT8_MAX)
                    {
                        result->clear();
                        break;
                    }
                }

                if (name[i] == '.' || i == name.size() - 1)
                {
                    if (len == 0) continue;
                    
                    result->at(pos) = len; // update component length
                    
                    len = 0;
                    pos = result->size();
                    result->push_back(0); // length placeholder or trailing zero
                }
            }
        }

        inline size_t ReadFqdn(const std::vector<uint8_t>& data, size_t offset, std::string* result)
        {
            result->clear();

            size_t pos = offset;
            while (1)
            {
                if (pos >= data.size())
                    return 0;

                uint8_t len = data[pos++];
                
                if (pos + len > data.size())
                    return 0;

                if (len == 0)
                    break;

                if (!result->empty())
                    result->append(".");
        
                result->append(reinterpret_cast<const char*>(&data[pos]), len);
                pos += len;
            }

            return pos - offset;
        }

        inline bool CreateSocket(int* result)
        {
            int fd = socket(AF_INET, SOCK_DGRAM, 0);
            if (fd < 0)
            {
                Log::Error("Failed to create socket with code " + std::to_string(GetSocketError()));
                return false;
            }

            int st = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&SockTrue), sizeof(SockTrue));
            if (st < 0)
            {
                CloseSocket(fd);
                Log::Error("Failed to set socket option SO_BROADCAST with code " + std::to_string(GetSocketError()));
                return false;
            }

            *result = fd;
            return true;
        }

        inline bool Send(int fd, const std::vector<uint8_t>& data)
        {
            sockaddr_in broadcastAddr = {0};
            broadcastAddr.sin_family = AF_INET;
            broadcastAddr.sin_port = htons(5353);
            broadcastAddr.sin_addr.s_addr = INADDR_BROADCAST;

            auto st = sendto(
                fd, 
                reinterpret_cast<const char*>(&data[0]), 
                data.size(), 
                0, 
                reinterpret_cast<const sockaddr*>(&broadcastAddr), 
                sizeof(broadcastAddr));

            // todo: st == data.size() ???
            if (st < 0)
            {
                Log::Error("Failed to send the query with code " + std::to_string(GetSocketError()));
                return false; 
            }

            return true;
        }

        inline bool Receive(int fd, time_t scanTime, std::vector<raw_responce>* result)
        {
            auto start = std::chrono::system_clock::now();

            while (1)
            {
                auto now = std::chrono::system_clock::now();
                if (now - start > std::chrono::seconds(scanTime))
                    break;

                fd_set fds;
                FD_ZERO(&fds);
                FD_SET(fd, &fds);

                timeval tv = {0};
                tv.tv_sec = static_cast<long>(scanTime);

                int st = select(fd+1, &fds, nullptr, nullptr, &tv);

                if (st < 0)
                {
                    Log::Error("Failed to wait on socket with code " + std::to_string(GetSocketError()));
                    return false; 
                }

                if (st > 0)
                {
#ifdef WIN32
                    int salen = sizeof(sockaddr_storage);
#else
                    unsigned int salen = sizeof(sockaddr_storage);
#endif

                    raw_responce item;
                    item.data.resize(MdnsMessageMaxLength);                    

                    auto cb = recvfrom(
                        fd, 
                        reinterpret_cast<char*>(&item.data[0]), 
                        item.data.size(), 
                        0, 
                        reinterpret_cast<sockaddr*>(&item.peer), 
                        &salen);

                    if (cb < 0)
                    {
                        Log::Error("Failed to receive with code " + std::to_string(GetSocketError()));
                        return false; 
                    }

                    item.data.resize((size_t)cb);
                    result->push_back(item);
                }
            }

            return true;
        }

        inline bool Parse(const raw_responce& input, mdns_responce* result)
        {
            // Structure:
            //   header (12b) 
            //   qname fqdn
            //   qtype (2b)
            //   qclass (2b)
            //   0xc0
            //   name offset (1b)
            //   DNS RR
            //
            // Note:
            //   GCC has bug in is.ignore(n)

            if (input.data.empty())
                return false;

            result->qname.clear();
            result->records.clear();

            memcpy(&result->peer, &input.peer, sizeof(sockaddr_storage));
            result->data = input.data;

            stdext::membuf buf(&input.data[0], input.data.size());
            std::istream is(&buf);

            const auto Flags = std::istream::failbit | std::istream::badbit | std::istream::eofbit;
            is.exceptions(Flags);

            try
            {
                uint8_t u8;
                uint16_t u16;

                is.ignore(); // id
                is.ignore();
    
                is.read(reinterpret_cast<char*>(&u16), 2); // flags
                if (ntohs(u16) != MdnsResponseFlag)
                {
                    Log::Warning("Found unexpected Flags value while parsing responce");
                    return false;
                }

                for (auto i = 0; i < 8; i++)
                    is.ignore(); // qdcount, ancount, nscount, arcount

                size_t cb = ReadFqdn(input.data, static_cast<size_t>(is.tellg()), &result->qname);
                if (cb == 0)
                {
                    Log::Error("Failed to parse query name");
                    return false;
                }

                for (auto i = 0; i < cb; i++)
                    is.ignore(); // qname
        
                is.read(reinterpret_cast<char*>(&u16), 2); // qtype
                result->qtype = ntohs(u16);

                is.ignore(); // qclass
                is.ignore();

                while (1)
                {
                    is.exceptions(std::istream::goodbit);
                    if (is.peek() == EOF)
                        break;
                    is.exceptions(Flags);

                    mdns_record rr = {0};
                    rr.pos = static_cast<size_t>(is.tellg());

                    is.read(reinterpret_cast<char*>(&u8), 1); // offset token
                    if (u8 != MdnsOffsetToken)
                    {
                        Log::Warning("Found incorrect offset token while parsing responce");
                        return false;
                    }

                    is.read(reinterpret_cast<char*>(&u8), 1); // offset value
                    if ((size_t)u8 >= input.data.size() || (size_t)u8 + input.data[u8] >= input.data.size())
                    {
                        Log::Warning("Failed to parse record name");
                        return false;
                    }

                    rr.name = std::string(reinterpret_cast<const char*>(&input.data[u8 + 1]), input.data[u8]);

                    is.read(reinterpret_cast<char*>(&u16), 2); // type
                    rr.type = ntohs(u16);

                    for (auto i = 0; i < 6; i++)
                        is.ignore(); // qclass, ttl

                    is.read(reinterpret_cast<char*>(&u16), 2); // length

                    for (auto i = 0; i < ntohs(u16); i++)
                        is.ignore(); // data

                    rr.len = MdnsRecordHeaderLength + ntohs(u16);
                    result->records.push_back(rr);
                }
            }
            catch (const std::istream::failure& ex)
            {
                Log::Warning(std::string("Stream error while parsing responce: ") + ex.what());
                return false;
            }

            return true;
        }

        inline bool Resolve(const std::string& serviceName, time_t scanTime, std::vector<mdns_responce>* result)
        {
            result->clear();

            std::vector<uint8_t> query;
            query.insert(query.end(), std::begin(MdnsQueryHeader), std::end(MdnsQueryHeader));
            WriteFqdn(serviceName, &query);
            query.insert(query.end(), std::begin(MdnsQueryFooter), std::end(MdnsQueryFooter));

            int fd = 0;
            if (!CreateSocket(&fd))
                return false;

            std::shared_ptr<void> guard(0, [fd](void*) { CloseSocket(fd); });

            if (!Send(fd, query))
                return false;
            
            std::vector<raw_responce> responces;
            if (!Receive(fd, scanTime, &responces))
                return false;
            
            for (auto& raw: responces)
            {
                mdns_responce parsed = {0};
                if (Parse(raw, &parsed))
                    result->push_back(parsed);
            }

            return true;
        }
    }
}

#endif // ZEROCONF_DETAIL_HPP