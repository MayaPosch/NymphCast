/*
	nyansd.cpp - Implementation file for the NyanSD service discovery library.
	
	Notes:
			- 
			
	2020/04/23, Maya Posch
*/


#include "nyansd.h"

#include <iostream>
#include <map>

#include <Poco/Net/DatagramSocket.h>
#include <Poco/Net/NetworkInterface.h>
#include <Poco/Net/DNS.h>


// Static variables.
std::vector<NYSD_service> NyanSD::services;
std::mutex NyanSD::servicesMutex;
std::atomic<bool> NyanSD::running{false};
std::thread NyanSD::handler;
ByteBauble NyanSD::bb;


struct ResponseStruct {
	char* data;
	uint32_t length;
};


// --- SEND QUERY ---
bool NyanSD::sendQuery(uint16_t port, std::vector<NYSD_query> queries, 
									std::vector<NYSD_service> &responses) {
	if (queries.size() > 255) {
		std::cerr << "No more than 255 queries can be send simultaneously." << std::endl;
		return false;
	}
	else if (queries.size() < 1) {
		std::cerr << "At least one query must be sent. No query found." << std::endl;
		return false;
	}
				
	// Compose the NYSD message.
	BBEndianness he = bb.getHostEndian();
	std::string msg = "NYANSD";
	uint16_t len = 0;
	uint8_t type = (uint8_t) NYSD_MESSAGE_TYPE_BROADCAST;
	
	std::string body;
	uint8_t qnum = queries.size();
	body += std::string((char*) &qnum, 1);
	for (int i = 0; i < qnum; ++i) {
		body += std::string("Q");
		uint8_t prot = (uint8_t) queries[i].protocol;
		uint8_t qlen = (uint8_t) queries[i].filter.length();
		body += (char) prot;
		body += (char) qlen;
		if (qlen > 0) {
			body += queries[i].filter;
		}
	}
	
	len = body.length() + 1;	// Add one byte for the message type.
	len = bb.toGlobal(len, he);
	msg += std::string((char*) &len, 2);
	msg += (char) type;
	msg += body;
	
	std::cout << "Message length: " << msg.length() << std::endl;
	
	// Open UDP socket, send message.
	Poco::Net::DatagramSocket udpsocket(Poco::Net::IPAddress::IPv4);
	udpsocket.setBroadcast(true);
	Poco::Net::SocketAddress sa("255.255.255.255", port);
	udpsocket.sendTo(msg.data(), msg.length(), sa);
	 
	// Listen for responses for 500 milliseconds.
	Poco::Timespan ts(500000);	// 500 ms timeout.
	int n;
	Poco::Net::Socket::SocketList readList, writeList, exceptList;
	readList.push_back(udpsocket);
	std::vector<ResponseStruct> buffers;
	while (Poco::Net::Socket::select(readList, writeList, exceptList, ts)) {
		ResponseStruct rs;
		rs.data = new char[2048];
		try {
			rs.length = udpsocket.receiveBytes(rs.data, 2048, 0);
		}
		catch (Poco::TimeoutException &exc) {
			std::cerr << "ReceiveBytes: " << exc.displayText() << std::endl;
			udpsocket.close();
			return false;
		}
		catch (...) {
			std::cerr << "ReceiveBytes: Unknown exception." << std::endl;
			return false;
		}
		
		std::cout << "Received message with length " << rs.length << std::endl;
		
		buffers.push_back(rs);
	}
	
	// Close socket as we're done with the network side.
	udpsocket.close();
	
	std::cout << "Parsing " << buffers.size() << " response(s)..." << std::endl;
	
	// Copy parsed responses into the 'responses' vector.
	for (int i = 0; i < buffers.size(); ++i) {
		int n = buffers[i].length;
		if (n < 8) {
			// Nothing to do.
			std::cout << "No responses were received." << std::endl;
			return false;
		}
		
		// The received data can contain more than one response. Start parsing from the beginning until
		// we are done.
		char* buffer = buffers[i].data;
		int index = 0;
		while (index < n) {
			std::string signature = std::string(buffer, 6);
			index += 6;
			if (signature != "NYANSD") {
				std::cerr << "Signature of message incorrect: " << signature << std::endl;
				return false;
			}
			
			len = *((uint16_t*) &buffer[index]);
			len = bb.toHost(len, BB_LE);
			index += 2;
			
			if (len > buffers[i].length - (index)) {
				std::cerr << "Insufficient data in buffer to finish parsing message: " << len << "/" 
							<< (buffers[i].length - (index + 6)) << std::endl;
				return false;
			}
			
			std::cout << "Found message with length: " << len << std::endl;
			
			type = *((uint8_t*) &buffer[index++]);		
			std::cout << "Message type: " << (uint16_t) type << std::endl;
			
			if (type != NYSD_MESSAGE_TYPE_RESPONSE) {
				std::cout << "Not a response message type. Skipping..." << std::endl;
				continue;
			}
			
			uint8_t rnum = *((char*) &buffer[index++]);		
			std::cout << "Response count: " << (uint16_t) rnum << std::endl;
			
			// Service sections.
			for (int i = 0; i < rnum; ++i) {
				if (buffer[index] != 'S' != 0) {
					std::cerr << "Invalid service section signature. Aborting parsing." << std::endl;
					return false;
				}
				
				index++;
				uint32_t ipv4 = *((uint32_t*) &buffer[index]);
				ipv4 = bb.toHost(ipv4, BB_LE);
				index += 4;
				
				uint8_t ipv6len = *((uint8_t*) &buffer[index++]);
				
				std::cout << "IPv6 string with length: " << (uint16_t) ipv6len << std::endl;
				
				std::string ipv6 = std::string(buffer + index, buffer + (index + ipv6len));
				index += ipv6len;
				
				uint16_t hostlen = *((uint16_t*) &buffer[index]);
				hostlen = bb.toHost(hostlen, BB_LE);
				index += 2;
				
				std::string hostname = std::string(buffer + index, buffer + (index + hostlen));
				index += hostlen;
				
				uint16_t port = *((uint16_t*) &buffer[index]);
				port = bb.toHost(port, BB_LE);
				index += 2;
				
				uint8_t prot = *((uint8_t*) &buffer[index++]);
				
				uint16_t snlen = *((uint16_t*) &buffer[index]);
				snlen = bb.toHost(snlen, BB_LE);
				index += 2;
				
				std::string svname = std::string(buffer + index, buffer + (index + snlen));
				index += snlen;
				
				std::cout << "Adding service with name: " << svname << std::endl;
				
				NYSD_service sv;
				sv.ipv4 = ipv4;
				sv.ipv6 = ipv6;
				sv.port = port;
				sv.hostname = hostname;
				sv.service = svname;
				if (prot == NYSD_PROTOCOL_ALL) {
					sv.protocol = NYSD_PROTOCOL_ALL;
				}
				else if (prot == NYSD_PROTOCOL_TCP) {
					sv.protocol = NYSD_PROTOCOL_TCP;
				}
				else if (prot == NYSD_PROTOCOL_UDP) {
					sv.protocol = NYSD_PROTOCOL_UDP;
				}
				
				responses.push_back(sv);
			}
			
			std::cout << "Buffer: " << index << "/" << n << std::endl;
		}
			
		delete[] buffers[i].data;
	}
	
	return true;
}


// --- SET SERVICE ---
// Adds a service entry to the list. Only the protocol, port and service entries are required, 
// the others will be filled in if left empty.
bool NyanSD::addService(NYSD_service service) {
	if (service.port == 0 || service.service.empty()) {
		std::cerr << "Invalid service entry: " << service.service << ":" << service.port << std::endl;
		return false;
	}
	
	if (service.hostname.empty()) {
		// Fill in the hostname of the system.
		service.hostname = Poco::Net::DNS::hostName();
	}
	
	servicesMutex.lock();
	services.push_back(service);
	servicesMutex.unlock();
	
	return true;
}


// --- START LISTENER ---
bool NyanSD::startListener(uint16_t port) {
	if (running) {
		std::cerr << "Client handler thread is already running." << std::endl;
		return false;
	}
	
	// Create new thread with the client handler.
	handler = std::thread(&NyanSD::clientHandler, port);
	
	return true;
}


// --- STOP LISTENER ---
bool NyanSD::stopListener() {
	// Stop the listening socket and clean-up resources.
	running = false;
	handler.join();
	
	return true;
}


std::string NyanSD::ipv4_uintToString(uint32_t ipv4) {
	std::string out;
	for (int i = 0; i < 4; ++i) {
		out += std::to_string(*(((uint8_t*) &ipv4) + i));
		if (i < 3) { out += "."; }
	}
	
	return out;
}


uint32_t NyanSD::ipv4_stringToUint(std::string ipv4) {
	// String should have the format: DD.DD.DD.DD, where 'DD' is a value between 0-255.
	std::cout << "IP to convert: " << ipv4 << std::endl;
	uint32_t out;
	uint8_t* op = (uint8_t*) &out;
	std::size_t pos = 0;
	std::size_t pos_end = 0;
	for (int i = 0; i < 4; i++) {
		pos_end = ipv4.find(".", pos + 1);
		*op = (uint8_t) std::stoul(ipv4.substr(pos, pos_end - pos));
		pos = pos_end;
		pos++;
		op++;
	}
	
	std::cout << "Converted IP: " << std::showbase << std::hex << out << std::dec << std::endl;
	
	return out;
}


// --- REMOTE TO LOCAL IP ---
bool remoteToLocalIP(Poco::Net::SocketAddress &sa, uint32_t &ipv4, std::string &ipv6) {
	std::map<unsigned, Poco::Net::NetworkInterface> map = Poco::Net::NetworkInterface::map(true, false);
	std::map<unsigned, Poco::Net::NetworkInterface>::const_iterator it = map.begin();
	std::map<unsigned, Poco::Net::NetworkInterface>::const_iterator end = map.end();

	bool isIPv6 = true;
	if (sa.family() == Poco::Net::IPAddress::IPv4) { isIPv6 = false; }
	
	std::string addr = sa.toString();
	std::cout << "Sender was IP: " << addr << std::endl;
	if (isIPv6) {
		addr.erase(addr.find_last_of(':') + 1);
	}
	else {
		addr.erase(addr.find_last_of('.') + 1);
	}
	
	std::cout << "LAN base IP: " << addr << std::endl;
	for (; it != end; ++it) {
		const std::size_t count = it->second.addressList().size();
		for (int i = 0; i < count; ++i) {
			std::string ip = it->second.address(i).toString();
			std::cout << "Checking IP: " << ip << std::endl;
			if (addr.compare(0, addr.length(), ip, 0, addr.length()) == 0) {
				std::cout << "Found IP: " << ip << std::endl;
				if (isIPv6) {
					ipv6 = it->second.address(i).toString();
					
					// Remove trailing '%<if>' section on certain OSes.
					std::string::size_type st = ipv6.find_last_of('%');
					if (st != std::string::npos) { ipv6.erase(st); }
					
					// Find first IPv4 address on this network interface.
					for (int j = 0; j < count; ++j) {
						if (it->second.address(j).af() == AF_INET) {
							ipv4 = NyanSD::ipv4_stringToUint(it->second.address(j).toString());
							return true;
						}
					}
				}
				else {
					ipv4 = NyanSD::ipv4_stringToUint(it->second.address(i).toString());
					
					// Find first IPv6 address on this network interface.
					for (int j = 0; j < count; ++j) {
						if (it->second.address(j).af() == AF_INET6) {
							ipv6 = it->second.address(j).toString();
							
							// Remove trailing '%<if>' section on certain OSes.
							std::string::size_type st = ipv6.find_last_of('%');
							if (st != std::string::npos) { ipv6.erase(st); }
					
							return true;
						}
					}
				}
				
				return false;
			}
		}
	}
	
	std::cout << "LAN IP not found on interfaces." << std::endl;
	
	return false;
}


// --- CLIENT HANDLER ---
void NyanSD::clientHandler(uint16_t port) {
	// Set up listening socket on the provided port.
	Poco::Net::DatagramSocket udpsocket;
	Poco::Net::SocketAddress sa(Poco::Net::IPAddress(), port);
	udpsocket.bind(sa, true);
	
	// Start listening loop.
	running = true;
	while (running) {
		// Read data in from socket.
		Poco::Net::SocketAddress sender;
		Poco::Timespan span(250000);
		if (udpsocket.poll(span, Poco::Net::Socket::SELECT_READ)) {
			char buffer[2048];
			int n = 0;
			try {
				n = udpsocket.receiveFrom(buffer, sizeof(buffer), sender);
			}
			catch (Poco::Exception &exc) {
				std::cerr << "ReceiveFrom: " << exc.displayText() << std::endl;
				continue;
			}
			
			if (n < 10) {
				std::cerr << "Received incomplete message. Skipping." << std::endl;
				continue;
			}
			
			std::cout << "Message length: " << n << std::endl;
			
			// Validate signature.
			int index = 0;
			std::string signature = std::string(buffer, 6);
			index += 6;
			if (signature != "NYANSD") {
				std::cerr << "Signature of message incorrect: " << signature << std::endl;
				continue;
			}
			
			uint16_t len = *((uint16_t*) &buffer[index]);
			len = bb.toHost(len, BB_LE);
			index += 2;
			
			if ((n - 8) != len) {
				std::cerr << "Failed to read full message: " << n << "/" << len << std::endl;
				continue;
			}
		
			uint8_t type = *((uint8_t*) &buffer[index++]);		
			std::cout << "Message type: " << (uint16_t) type << std::endl;
			if (type != NYSD_MESSAGE_TYPE_BROADCAST) {
				std::cout << "Not a broadcast message type. Skipping..." << std::endl;
				continue;
			}
			
			// Parse message for queries.
			BBEndianness he = bb.getHostEndian();
			uint8_t rnum = *((char*) &buffer[index++]);		
			std::cout << "Query count: " << (uint16_t) rnum << std::endl;
			
			if (rnum == 0) {
				std::cerr << "Broadcast message didn't contain any queries. Skipping..." << std::endl;
				continue;
			}
		
			// Query sections.
			for (int i = 0; i < rnum; ++i) {
				if (buffer[index] != 'Q') {
					std::cerr << "Invalid query section signature: " << std::showbase << std::hex 
								<< (uint16_t) buffer[index] << std::dec
								<< ". Aborting parsing." << std::endl;
					continue;
				}
				
				index++;
				uint8_t prot = *((uint8_t*) &buffer[index++]);				
				uint8_t qlen = *((uint8_t*) &buffer[index++]);
				
				std::string servicesBody;
				uint8_t scount = 0;
				if (qlen != 0) {
					std::string filter = std::string(&(buffer[index]), qlen);
					index += qlen;
					
					// Create response body if request matches local data.
					// The match is done using a simple substring compare on the beginning of the
					// filter and service string.
					servicesMutex.lock();
					for (int i = 0; i < services.size(); ++i) {
						if (filter.compare(0, filter.length(), services[i].service) == 0) {
							servicesBody += "S";
							if (services[i].ipv4 == 0) {
								// Fill in the IP address of the interface we are listening on.
								uint32_t ipv4;
								std::string ipv6;
								if (!remoteToLocalIP(sender, ipv4, ipv6)) {
									std::cerr << "Failed to convert remote IP to local." << std::endl;
									continue;
								}
								
								if (ipv6.length() > 39) {
									std::cerr << "Got wrong ipv6 string length: " << ipv6.length() 
												<< std::endl;
									continue;
								}
								
								ipv4 = bb.toGlobal(ipv4, he);
								servicesBody += std::string((char*) &ipv4, 4);
								uint8_t ipv6len = ipv6.length();
								servicesBody += (char) ipv6len;
								servicesBody += ipv6;
							}
							else {
								uint32_t ipv4 = bb.toGlobal(services[i].ipv4, he);
								servicesBody += std::string((char*) &ipv4, 4);
								uint8_t ipv6len = services[i].ipv6.length();
								servicesBody += (char) ipv6len;
								servicesBody += services[i].ipv6;
							}
							
							uint16_t hlen = services[i].hostname.length();
							hlen = bb.toGlobal(hlen, he);
							servicesBody += std::string((char*) &hlen, 2);
							servicesBody += services[i].hostname;
							
							uint16_t port = bb.toGlobal(services[i].port, he);
							servicesBody += std::string((char*) &port, 2);
							servicesBody += (char) (services[i].protocol);
							
							uint16_t snlen = services[i].service.length();
							snlen = bb.toGlobal(snlen, he);
							servicesBody += std::string((char*) &snlen, 2);
							servicesBody += services[i].service;
							
							scount++;
						}
					}
					servicesMutex.unlock();
				}
				else {
					// Create response body containing all local data.
					servicesMutex.lock();
					for (int i = 0; i < services.size(); ++i) {
						servicesBody += "S";
						if (services[i].ipv4 == 0) {
							// Fill in the IP address of the interface we are listening on.
							uint32_t ipv4;
							std::string ipv6;
							if (!remoteToLocalIP(sender, ipv4, ipv6)) {
								std::cerr << "Failed to convert remote IP to local." << std::endl;
								continue;
							}
							
							if (ipv6.length() > 39) {
								std::cerr << "Got wrong ipv6 string length: " << ipv6.length() 
											<< std::endl;
								continue;
							}
							
							ipv4 = bb.toGlobal(ipv4, he);
							servicesBody += std::string((char*) &ipv4, 4);
							uint8_t ipv6len = ipv6.length();
							servicesBody += (char) ipv6len;
							servicesBody += ipv6;
						}
						else {
							uint32_t ipv4 = bb.toGlobal(services[i].ipv4, he);
							servicesBody += std::string((char*) &ipv4, 4);
							uint8_t ipv6len = services[i].ipv6.length();
							servicesBody += (char) ipv6len;
							servicesBody += services[i].ipv6;
						}
						
						uint16_t hlen = services[i].hostname.length();
						hlen = bb.toGlobal(hlen, he);
						servicesBody += std::string((char*) &hlen, 2);
						servicesBody += services[i].hostname;
							
						uint16_t port = bb.toGlobal(services[i].port, he);
						servicesBody += std::string((char*) &port, 2);
						servicesBody += (char) (services[i].protocol);
						
						uint16_t snlen = services[i].service.length();
						snlen = bb.toGlobal(snlen, he);
						servicesBody += std::string((char*) &snlen, 2);
						servicesBody += services[i].service;
						
						scount++;
					}
					
					servicesMutex.unlock();
				}
				
				std::cout <<"Services body generated of size: " << servicesBody.length() << std::endl;
				
				// Assemble the full response message.
				std::string msg = "NYANSD";
				uint16_t msglen = 1;
				uint8_t type = (uint8_t) NYSD_MESSAGE_TYPE_RESPONSE;

				msglen += servicesBody.length() + 1;	// Add 1 for service section counter.
				msglen = bb.toGlobal(msglen, he);
				msg += std::string((char*) &msglen, 2);
				msg += (char) type;
				msg += std::string((char*) &scount, 1);
				msg += servicesBody;
				
				std::cout << "Sending response with size: " << msg.length() << std::endl;
				
				int n = udpsocket.sendTo(msg.data(), msg.length(), sender);
			}
		}
	}
	
	// Clean up resources and return.
	
}
