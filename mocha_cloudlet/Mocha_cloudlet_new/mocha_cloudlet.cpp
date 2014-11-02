//Description: class file for mocha cloudlet
#include "facerec.h"
#include <opencv\cv.h>
#include "mocha_cloudlet.h"
#include "mocha.h"
#include <opencv\highgui.h>

//put this after all the #include, or you will get a link error
#pragma comment(lib,"WS2_32")

using namespace std;
using namespace cv;

//add server to cloud server
void cloudlet_c::add_server(string name, string ip, int port_num)
{
	cloud_server_c cloud_server(name, ip, port_num);	
	cloud_servers.push_back(cloud_server);
}

//create local cloudlet server
void cloudlet_c::cloudlet_server_create()
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
	cloudlet_addr.sin_family = AF_INET;
	cloudlet_addr.sin_port = htons(my_port);
	cloudlet_addr.sin_addr.s_addr = INADDR_ANY;

    	//bind server and socket
    	iResult = bind(listend, (SOCKADDR *)&cloudlet_addr, sizeof(cloudlet_addr));
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
	cout<<"cloudlet server established"<<endl;

	return;
}

//connect cloud server
void cloudlet_c::connect_cloud()
{
	cout<<"trying to connect"<<endl;
	int iResult=0;
	WSADATA wsaData;

	//check socket version
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
	  cout<<"WSAStartup failed"<<endl;
	  exit(1);
	}	

	//try to connect all the server
	for(vector<cloud_server_c>::iterator itr=cloud_servers.begin(); itr!=cloud_servers.end(); ++itr){
		//create socket
		itr->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	 	if (itr->sock== INVALID_SOCKET){
 	    		cout<<"socket function failed with error"<<WSAGetLastError()<<endl;
	    		WSACleanup();
	    		exit(1);
	 	}
		//address configuration
		itr->addr.sin_family = AF_INET;
		itr->addr.sin_addr.s_addr = inet_addr(itr->ip.c_str());
		itr->addr.sin_port = htons(itr->port_num);

		//connect
		iResult = connect(itr->sock, (SOCKADDR *) &(itr->addr), sizeof (itr->addr));
		if (iResult == SOCKET_ERROR) {
	    		cout<<"connect function with "<<itr->name<<" failed with error"<<WSAGetLastError<<endl;
	    		iResult = closesocket(itr->sock);
	    		WSACleanup();
	    		exit(1);
		}
		
		cout<<"connect to "<<itr->name<<" success"<<endl;

	}	

	return;
}	

//add mobile connections to cloudlet
void cloudlet_c::add_mobile()
{
	SOCKET sock = accept(listend, NULL, NULL);

	if(sock == INVALID_SOCKET){	
	    cout<<"accept failed with error "<<WSAGetLastError()<<endl;
	    closesocket(listend);
	    WSACleanup();
	    exit(1);
	 }
		
	 mobile_clients.push_back(mobile_client_c(sock)); 
	 cout<<"New mobile cloudlet connection estabilshed, device ID: "<<mobile_clients.back().id<<endl;

	 return;
}

//handle packet comes from mobile side
DWORD WINAPI mobile_packet_thread(LPVOID lpParam)
{
  	thread_arg_t *thread_arg = (thread_arg_t*)lpParam;
	static HANDLE init_mutex = CreateMutex(NULL, FALSE, NULL);


	std::vector<char> data(thread_arg->mocha_packet->payload, thread_arg->mocha_packet->payload+thread_arg->mocha_packet->length-MOCHA_HEADER_LENGTH);
	Mat source_frame = imdecode(Mat(data), /*CV_LOAD_IMAGE_GRAYSCALE*/1);
#ifdef FACE_CROP
	vector<Mat> faces;	

	faceDetect_GPU(source_frame, faces);

	cout<<"face size "<<faces.size()<<endl;
	if(!faces.size()) return 0;
#endif

#ifdef PRE_PCA
	Mat projection;
#ifdef FACE_CROP
	faceRecCloudlet(faces[0], projection);
#else
	faceRecCloudlet(source_frame, projection);
#endif

	//cout<<"The mat is: "<<projection<<endl;
	//cout<<"rows: "<<projection.rows<<"\tcols: "<<projection.cols<<"\ttype"<<projection.type()<<endl;
	delete[] thread_arg->mocha_packet->payload;
#ifdef CUT_TO_FLOAT
	thread_arg->mocha_packet->length = sizeof(int)*3+(projection.dataend-projection.datastart)/2+MOCHA_HEADER_LENGTH;
	thread_arg->mocha_packet->payload = new char[sizeof(int)*3+(projection.dataend-projection.datastart)/2];
#else
	thread_arg->mocha_packet->length = sizeof(int)*3+projection.dataend-projection.datastart+MOCHA_HEADER_LENGTH;
	thread_arg->mocha_packet->payload = new char[sizeof(int)*3+projection.dataend-projection.datastart];
#endif
	thread_arg->mocha_packet->type = MOCHA_VECTOR_TYPE;

	int p=0;
	int temp=0;

	temp=htonl(projection.rows);
	memcpy(thread_arg->mocha_packet->payload, &temp, sizeof(int));
	p+=sizeof(int);

	temp=htonl(projection.cols);
	memcpy(thread_arg->mocha_packet->payload+p, &temp, sizeof(int));
	p+=sizeof(int);

	temp=htonl(projection.type());
	memcpy(thread_arg->mocha_packet->payload+p, &temp, sizeof(int));
	p+=sizeof(int);

#ifdef CUT_TO_FLOAT
	MatConstIterator_<double> itr=projection.begin<double>(), itr_end=projection.end<double>();
	for(float temp=0.0;itr!=itr_end;++itr){
		temp = (float)(*itr);
		memcpy(thread_arg->mocha_packet->payload+p, &temp, sizeof(float));
		p+=sizeof(float);
	}
#else
	memcpy(thread_arg->mocha_packet->payload+p, projection.datastart, projection.dataend-projection.datastart); 
#endif
#endif

	//send to cloud
	thread_arg->mocha_packet->send_out(thread_arg->cloud_sock);

#ifdef PRE_PCA
		delete[] thread_arg->mocha_packet->payload;
#else
		;
#endif
	delete[] thread_arg->mocha_packet;
  	delete thread_arg;

  return 0;	
}

//handle packet comes from cloud side
DWORD WINAPI cloud_packet_thread(LPVOID lpParam)
{
  	thread_arg_t *thread_arg = (thread_arg_t*)lpParam;

	//You can add cloudlet processing here

	//send to mobile 
	thread_arg->mocha_packet->send_out(thread_arg->mobile_sock);

#ifdef DEBUG
	cout<<"send to mobible success"<<endl;
#endif

	delete[] thread_arg->mocha_packet->payload;
	delete[] thread_arg->mocha_packet;
  	delete thread_arg;

  return 0;	
}

