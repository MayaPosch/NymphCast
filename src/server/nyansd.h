/*
	nyansd.h - Header file for the NyanSD service discovery library.
	
	Notes:
			- 
			
	2020/04/23, Maya Posch
*/


#ifndef NYANSD_H
#define NYANSD_H


#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <string>

#include "bytebauble.h"


enum NYSD_message_type {
	NYSD_MESSAGE_TYPE_BROADCAST	= 0x01,
	NYSD_MESSAGE_TYPE_RESPONSE	= 0x02
};


enum NYSD_protocol {
	NYSD_PROTOCOL_ALL = 0x00,
	NYSD_PROTOCOL_TCP = 0x01,
	NYSD_PROTOCOL_UDP = 0x02
};


struct NYSD_service {
	uint32_t ipv4 = 0;
	std::string ipv6;
	uint16_t port = 0;
	std::string hostname;
	std::string service;
	NYSD_protocol protocol = NYSD_PROTOCOL_ALL;
};


struct NYSD_query {
	NYSD_protocol protocol;
	std::string filter;
};


class NyanSD {
	static std::vector<NYSD_service> services;
	static std::mutex servicesMutex;
	static std::atomic<bool> running;
	static std::thread handler;
	static ByteBauble bb;
	
	static void clientHandler(uint16_t port);
	
public:
	static bool sendQuery(uint16_t port, std::vector<NYSD_query> queries, 
										std::vector<NYSD_service> &responses);
	static bool addService(NYSD_service service);
	static bool startListener(uint16_t port);
	static bool stopListener();
	
	static std::string ipv4_uintToString(uint32_t ipv4);
	static uint32_t ipv4_stringToUint(std::string ipv4);
};


#endif
