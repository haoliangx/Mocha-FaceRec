#include "facerec.h"
#include <cv.h>
#include "mocha_cloud.h"
#include <algorithm>    // std::sort
#include <cvaux.h>
#include <vector>
#include <highgui.h>
#include <random>
#include "mocha.h"
#include <iostream>
#include <fstream>

//put this after all the #include, or you will get a link error
#pragma comment(lib,"WS2_32")

using namespace std;
using namespace cv;

void cloud_c::server_run()
{
	int iResult=0;
	WSADATA wsaData;
	
	//check socket version
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
	  cout<<"WSAStartup failed"<<endl;
	  exit(1);
	}

	//create server socket
	listend = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listend == INVALID_SOCKET){
		cout<<"socket function failed with error "<<WSAGetLastError()<<endl;
		WSACleanup();
		exit(1);
	}

	//initialize server address
	cloud_addr.sin_family = AF_INET;
	cloud_addr.sin_port = htons(my_port);
	cloud_addr.sin_addr.s_addr = INADDR_ANY;	

    	iResult = bind(listend, (SOCKADDR *)&cloud_addr, sizeof(cloud_addr));
	if(iResult == SOCKET_ERROR){
	    cout<<"Server bind fail!"<<endl;
	    closesocket(listend);
	    WSACleanup();
	    exit(1);
	}

	//listen
	if (listen(listend, SOMAXCONN) == SOCKET_ERROR){
	    cout<<"listen function failed with error"<<WSAGetLastError()<<endl;
	    exit(1);
	}
	cout<<"cloud server established"<<endl;

	return;
}

//ATTENTION: ALL THE PACKET TYPE IS DEFINED IN THE MOBILE SIDE, WHEN THE MOCHA PACKET IS FORMED, IT IS TAGGED WITH A PACKET TYPE
//THE DATA SEND FROM THE MOBILE IS EITHER RAW IMAGE OR FACE IMAGE, HOWEVER, WHEN YOU SET THE DATA TYPE YOU SHOULD CONSIDER WHEN THE DATA HITS THE CLOUD, IF THE DATA HITS THE CLOUD IS VECTOR, YOU SHOULD SET THE TYPE TO VECTOR

//the thread function for to handle the cloudlet packet
DWORD WINAPI cloudlet_packet_thread(LPVOID lpParam)
{
	//cast the params
  	thread_arg_t *thread_arg = (thread_arg_t*)lpParam;
 	
	 //do face rec
	 face_t result;

	if(thread_arg->mocha_packet->type == MOCHA_DATA_FACE){//process face images
	    //load images from memory
	    std::vector<char> data(thread_arg->mocha_packet->payload, thread_arg->mocha_packet->payload+thread_arg->mocha_packet->length-MOCHA_HEADER_LENGTH);
	    Mat source_frame = imdecode(Mat(data), CV_LOAD_IMAGE_GRAYSCALE);

	    faceRecLocal(source_frame, result);
	  }
	  else if(thread_arg->mocha_packet->type == MOCHA_VECTOR_TYPE){//process vector packet

	    int temp;  
	    int p=0;  

	    memcpy(&temp, thread_arg->mocha_packet->payload, sizeof(int));
	    int row = ntohl(temp);
	    p+=sizeof(int);

	    memcpy(&temp, thread_arg->mocha_packet->payload+p, sizeof(int));
	    int col = ntohl(temp);
	    p+=sizeof(int);

	    memcpy(&temp, thread_arg->mocha_packet->payload+p, sizeof(int));
	    int type = ntohl(temp);
	    p+=sizeof(int);

#ifdef CUT_TO_FLOAT//if this flag is enable on the cloudlet, you should also enable here
	    Mat face_vector = Mat(row, col, type);

	    MatIterator_<double> itr=face_vector.begin<double>(), itr_end=face_vector.end<double>();
	    for(float temp=0;itr!=itr_end;++itr){
			memcpy(&temp, thread_arg->mocha_packet->payload+p, sizeof(float));
			*itr = (double)temp;
			p+=sizeof(float);
	    }	    
#else
	    Mat face_vector = Mat(row, col, type, thread_arg->mocha_packet->payload+p);
#endif
	    //cout<<"The mat is: "<<face_vector<<endl;
	    faceRecCloud(face_vector, result);
	  }
	  else if(thread_arg->mocha_packet->type == MOCHA_RAW_IMAGE){//process raw image
	    std::vector<char> data(thread_arg->mocha_packet->payload, thread_arg->mocha_packet->payload+thread_arg->mocha_packet->length-MOCHA_HEADER_LENGTH);
	    Mat source_frame = imdecode(Mat(data), /*CV_LOAD_IMAGE_GRAYSCALE*/1);
	    vector<Mat> faces;

	    faceDetect_GPU(source_frame, faces);

	    //cout<<"face size "<<faces.size()<<endl;
	    if(!faces.size()) return 0;
	    //face recognition
		faceRecLocal(source_frame, result);
	  }
	  else{
		cout<<"Wrong data type "<<thread_arg->mocha_packet->type<<endl;
		exit(1);
	  }

	    cout<<"name: "<<result.name<<endl;
	    cout<<"confidence: "<<result.confidence<<endl;

#ifdef DELAY_SIMULATE
	double mean = remote_server.mean_a*thread_arg->mocha_packet->length/1024+remote_server.mean_b;
	double stddev = remote_server.std_a*thread_arg->mocha_packet->length/1024+remote_server.std_b;
	default_random_engine generator;
	normal_distribution<double> distribution(mean/1000000, stddev/1000000);
	double delay = distribution(generator);
	cout<<"delay time "<<delay/1000<<"s"<<endl;
	Sleep(delay);
#endif

	//create response packet from the result
	mocha_packet_c*	response_pakcet = new mocha_packet_c(&result, thread_arg->mocha_packet);

	//send to cloudlet
	response_pakcet->send_out(thread_arg->cloudlet_sock);


	delete[] thread_arg->mocha_packet->payload;
	delete[] thread_arg->mocha_packet;
  	delete thread_arg;

  return 0;	
}