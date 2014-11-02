//header files for mocha

#ifndef MOCHA_H
#define MOCHA_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <conio.h>
#include <iostream>
#include <cstdio>
#include <string>
#include "facerec.h"


#define MOCHA_HEADER_LENGTH 22
#define MOCHA_GREETING_BYTES_NUM 6 
#define MOCHA_GREETING_MSG "MOCHA"
#define MOCHA_RAW_IMAGE 4
#define MOCHA_VECTOR_TYPE 3
#define RESULT_TYPE_NUM 2
#define MOCHA_DATA_FACE 1

#define MEM_ASSERT(P) if(P==NULL){ \
	std::cout<<"memory allocation fail at line number "<<__LINE__<<"in file "<<__FILE__<<std::endl;\
			exit(1);}

class mocha_packet_c {

  public:
	//default constructor, used for thread arg, you should never explicitly call this constructor
	mocha_packet_c(){}
	//constructor, construct mocha packet from data packet, header will initialized 
	mocha_packet_c (char *header_stream,  int device_id=-1);
	//constructor, construct mocha packet from result and the request packet 
	mocha_packet_c (face_t *result, mocha_packet_c *request); 
	//if the packet has already got its header, use this method to get its payload
	int get_payload_from_sock(SOCKET sock);
	//send the packet out, input is the target socket
	int send_out(SOCKET sock);
	//print out the content
	void dump();

	//packet type, all set to 1 at the moment
	int type;
	//packet index
	int index;
	//whole packet length
	int length;
	//the device send this packet, we will use sock number in cloudlet
	int device_id;
	//payload
	char *payload;

  private:
	//return the output stream for this packet
	//input is an char pointer, points to memory space of length
	void out_stream(char *);
};

//query mocha packet, check for greeting message
int mocha_query(SOCKET sock);
#endif
