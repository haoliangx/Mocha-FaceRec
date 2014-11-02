#ifndef MOCHA_CLOUD_H
#define MOCHA_CLOUD_H

#include <iostream>
#include <vector>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <conio.h>
#include "mocha.h"

#define DEFAULT_CLOUD_PORT 3200 
//#define DEBUG
//#define DELAY_SIMULATE
//#define CUT_TO_FLOAT
//#define EMPTY_RESULT //send empty result back
//#define SAVE_LOCAL_IMAGE	//save images locally
//#define NON_DEFAULT_PORT_TEST  //enter the port number by user

extern double PCFreq;
#ifdef DELAY_SIMULATE
extern struct server_profile_t remote_server;

struct server_profile_t{
	std::string location;
	double mean_a;
	double mean_b;
	double std_a;
	double std_b;
};
#endif

struct thread_arg_t{
	SOCKET cloudlet_sock;
	mocha_packet_c *mocha_packet;
};

class cloud_c {
	
  public:
	SOCKET listend;
	int last_pack_id;
	std::vector<SOCKET> cloudlet_socks;

	cloud_c(int cloud_port=DEFAULT_CLOUD_PORT):my_port(cloud_port){}
	void server_run();

  private:
	int my_port;
	sockaddr_in cloud_addr;	
};

DWORD WINAPI cloudlet_packet_thread(LPVOID lpParam);
#endif
