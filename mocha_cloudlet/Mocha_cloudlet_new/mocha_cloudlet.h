//Description: header file for mocha cloudlet class

#ifndef MOCHA_CLOUDLET_H
#define MOCHA_CLOUDLET_H

#include <iostream>
#include <vector>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <conio.h>
#include "mocha.h"

//#define SAVE_LOCAL_IMAGE
#define FACE_CROP//this flag enables face detection
#define DEFAULT_CLOUDLET_PORT 1600
#define TESLA1_IP "172.30.165.4"
#define TESLA2_IP "172.30.165.11"
#define TESLA3_IP "172.30.165.10" 
#define AMAZON_IP "54.214.100.239"
//#define PRE_PCA
//#define CUT_TO_FLOAT

extern double PCFreq;

//thread argument struct
struct thread_arg_t{
	SOCKET cloud_sock;
	SOCKET mobile_sock;
	mocha_packet_c *mocha_packet;
};

class cloud_server_c {
  
  public:
	//server name
	std::string name;
	//server IP address
	std::string ip;
	//server port number
	int port_num;
	//socket to the server
	SOCKET sock;
	//server address struct
	sockaddr_in addr;

	//constructor
	cloud_server_c(std::string name, std::string ip, int port_num) : name(name), ip(ip), port_num(port_num) {} 
};

class mobile_client_c {
  
  public:
	//socket to the mobile
	SOCKET sock;
	//device id
	int id;

	mobile_client_c(SOCKET sock) : sock(sock), id((int) sock) {}
};

class cloudlet_c {

  public:
	//cloud_servers
	std::vector<cloud_server_c> cloud_servers;
	//mobile clients
	std::vector<mobile_client_c> mobile_clients;
	//local server listen socket
	SOCKET listend;

	//constructor
	cloudlet_c(int my_port=DEFAULT_CLOUDLET_PORT) : my_port(my_port) {}
	//add server
	void add_server(std::string, std::string ip, int port_num);
	//local server creation
	void cloudlet_server_create();
	//connect to cloud
	void connect_cloud();
	//add mobile device
	void add_mobile();


  private: 
	  //local server port number
	  int my_port;	 
	  //local server address
	  sockaddr_in cloudlet_addr;
};

//thread funciton handles mobile packet
DWORD WINAPI mobile_packet_thread(LPVOID lpParam);
//thread funciton handles cloud packet
DWORD WINAPI cloud_packet_thread(LPVOID lpParam);
#endif
