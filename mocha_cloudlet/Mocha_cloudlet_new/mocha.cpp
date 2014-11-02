#include <cstring>
#include <cstdio>
#include <cassert>
#include <ctime>
#include "mocha.h"
//#include "mocha_cloudlet.h"

using namespace std;

mocha_packet_c::mocha_packet_c(char *header_stream, int device_id)
{
	//confirm mocha greeting message
	if(strcmp(MOCHA_GREETING_MSG, header_stream)){
		cout<<"wrong mocha message "<<header_stream<<endl;
		exit(1);
	}
	int p=MOCHA_GREETING_BYTES_NUM;
	int temp;

	//write type
	memcpy(&temp, header_stream+p, sizeof(int));
	type = ntohl(temp);
	p+=sizeof(int);

	//write index
	memcpy(&temp, header_stream+p, sizeof(int));
	index=ntohl(temp);
	p+=sizeof(int);

	//write length
	memcpy(&temp, header_stream+p, sizeof(int));
	length=ntohl(temp);
	p+=sizeof(int);
	
	//write device id
	if(device_id!=-1){
		this->device_id = device_id;
	}
	else{
		memcpy(&temp, header_stream+p, sizeof(int));
		this->device_id=ntohl(temp);
	}

	payload = NULL;
}
	
mocha_packet_c::mocha_packet_c(face_t *result, mocha_packet_c *request)
{	
	type=RESULT_TYPE_NUM;
	index = request->index;
	device_id = request->device_id;
	int payload_length = result->name.length()+result->confidence.length();
	length = MOCHA_HEADER_LENGTH+payload_length;
	
	payload = new char[payload_length];
	MEM_ASSERT(payload);
	
	int p=0;
	//write confidence
	memcpy(payload, result->confidence.c_str(), result->confidence.length());
	p+=result->confidence.length();
	//write name
	memcpy(payload+p, result->name.c_str(), result->name.length());
	//p+=result->name.length()+1;
}


int mocha_packet_c::get_payload_from_sock(SOCKET sock)
{
  //function should be called after header got initialized
  if(length==0){
	  cout<<"packet has no header"<<endl;
	  exit(1);
  }

  //check if payload is empty
  if(payload!=NULL){
	  cout<<"Payload is not empty, possible re-get?"<<endl;
	  exit(1);
  }

  int iResult=0;
  if(sock==INVALID_SOCKET){
  	cout<<"invalid socket in getting payload"<<endl;
	exit(1);
  }

  //discard the header
  char *header = new char[MOCHA_HEADER_LENGTH];
  MEM_ASSERT(header);

  iResult = recv(sock, header, MOCHA_HEADER_LENGTH, MSG_WAITALL);
  if(iResult!=MOCHA_HEADER_LENGTH){
	  cout<<"recv header fail in getingg payload"<<endl;
	  exit(1);
  }
  delete[] header;

  //get payload
  payload = new char[length-MOCHA_HEADER_LENGTH];
  MEM_ASSERT(payload);

  iResult = recv(sock, payload, length-MOCHA_HEADER_LENGTH, MSG_WAITALL);
  if(iResult!=length-MOCHA_HEADER_LENGTH){
	  cout<<"recv payload fail in getting payload"<<endl;
	  exit(1);
  }

#ifdef SAVE_LOCAL_IMAGE 
  //save the images locally
   char *filename = (char*)malloc(100);
   srand((unsigned int)time(NULL));

   sprintf(filename, "image%d.jpg", rand()%1000);
   FILE *fp = fopen(filename, "wb");
   assert(fp!=NULL);
   fwrite(payload,1, length-MOCHA_HEADER_LENGTH, fp);
   fflush(fp);
   fclose(fp);
#endif

  return 0;
} 

int mocha_packet_c::send_out(SOCKET sock)
{
	int iResult = 0;
	char *out = new char[length]; 
	MEM_ASSERT(out);
	out_stream(out);

	iResult = send(sock, out, length, 0);
	if(iResult!=length){
		cout<<"send to cloud fail"<<endl;
		exit(1);
	}

	delete[] out;
	return 0;
}

void mocha_packet_c::out_stream(char *out)
{ 
	int p=0;
	int temp;

	memcpy(out, MOCHA_GREETING_MSG, MOCHA_GREETING_BYTES_NUM);
	p+=MOCHA_GREETING_BYTES_NUM;

	temp = htonl(type);
	memcpy(out+p, &temp, sizeof(int));
	p+=sizeof(int);
	
	temp = htonl(index);
	memcpy(out+p, &temp, sizeof(int));
	p+=sizeof(int);

	temp = htonl(length);
	memcpy(out+p, &temp, sizeof(int));
	p+=sizeof(int);

	temp = htonl(device_id);
	memcpy(out+p, &temp, sizeof(int));
	p+=sizeof(int);

	if(payload==NULL){
		cout<<"payload is emtpy"<<endl;
		exit(1);
	}
	memcpy(out+p, payload, length-MOCHA_HEADER_LENGTH);

	return;
}

void mocha_packet_c::dump()
{
	cout<<"type "<<type<<endl;
	cout<<"index "<<index<<endl;
	cout<<"length "<<length<<endl;
	cout<<"device id "<<device_id<<endl;

	return;
}

int mocha_query(SOCKET sock)
{
	char *greeting = new char[MOCHA_GREETING_BYTES_NUM];
	MEM_ASSERT(greeting);
	recv(sock, greeting, MOCHA_GREETING_BYTES_NUM, MSG_PEEK);
	if(strcmp(greeting, MOCHA_GREETING_MSG)){
		delete[] greeting;
		return 0;
	}
	
	delete[] greeting;
	return 1;
}

