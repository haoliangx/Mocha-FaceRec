//Description: main funciton for the MOCHA cloudlet
#include "mocha_cloudlet.h"
#include "mocha.h"
#include "facerec.h"
#include <random>

using namespace std;
using namespace cv;
using namespace gpu;

double PCFreq = 0.0;

int main(int argc, char **argv)
{  
  //prepare for performance measurment, sample performance measure code: http://www.netperf.org/svn/netperf2/trunk/src/netcpu_ntperf.c
  LARGE_INTEGER li;
  if(!QueryPerformanceFrequency(&li))
		cout << "QueryPerformanceFrequency failed!\n";
  PCFreq = double(li.QuadPart)/1000.0;

#ifdef FACE_CROP//do face detection on the cloudlet
	Mat init_image;
	vector<Mat> init_faces;
	faceDetect_GPU(init_image, init_faces, 1);
	cout<<"Face detection initialization complete"<<endl;
#endif

#ifdef PRE_PCA// do projection on the cloudlet
	Mat testFace, projection;
	faceRecCloudlet(testFace, projection, 1);
	cout<<"PCA initialization complete"<<endl;
#endif

  cloudlet_c my_cloudlet;
  int iteration=1;


  cout<<"begin connection?"<<endl;
  char a;
  scanf("%c", &a);

  //add server
  //if you want multiple server, you can add here
  my_cloudlet.add_server("Tesla3", TESLA3_IP, 3200);
  //my_cloudlet.add_server("Amazon", AMAZON_IP, 3200);
  //create local server
  my_cloudlet.cloudlet_server_create(); 
  //connect to cloud servers
  my_cloudlet.connect_cloud();

  fd_set s_rd;
  FD_ZERO(&s_rd);


  for(;;){
	//register cloud servers
	for(vector<cloud_server_c>::iterator itr=my_cloudlet.cloud_servers.begin();itr!=my_cloudlet.cloud_servers.end();itr++){
    		FD_SET(itr->sock, &s_rd);
	}

	//register mobile cilents
	for(vector<mobile_client_c>::iterator itr=my_cloudlet.mobile_clients.begin();itr!=my_cloudlet.mobile_clients.end();itr++){
		FD_SET(itr->sock, &s_rd);
	}
	//register local server
	FD_SET(my_cloudlet.listend, &s_rd);
	select(0, &s_rd, NULL, NULL, NULL);

	//check if new mobile device connection
	if(FD_ISSET(my_cloudlet.listend, &s_rd)){
		my_cloudlet.add_mobile();
		my_cloudlet.connect_cloud();
	}
	
	//check mobile sockets
	for(vector<mobile_client_c>::iterator itr=my_cloudlet.mobile_clients.begin(); itr!=my_cloudlet.mobile_clients.end();itr++){
		if(FD_ISSET(itr->sock, &s_rd)){
			//check if there is mocha packet in the socket	
			int iResult = mocha_query(itr->sock);

			if(iResult==1){
				//get header
				char *header = new char[MOCHA_HEADER_LENGTH];
				MEM_ASSERT(header);
				iResult = recv(itr->sock, header, MOCHA_HEADER_LENGTH, MSG_PEEK); 
				if(iResult != MOCHA_HEADER_LENGTH){
					cout<<"get header fail!"<<endl;
					exit(1);
				}
				mocha_packet_c *mocha_packet = new mocha_packet_c(header, itr->id);
				MEM_ASSERT(mocha_packet);

				cout<<"iteration "<<iteration++<<endl;

				//initialize the thread args
				struct thread_arg_t *thread_arg = new thread_arg_t;
				MEM_ASSERT(thread_arg);

				thread_arg->mocha_packet= mocha_packet;
				thread_arg->mobile_sock = itr->sock;	
				thread_arg->cloud_sock = my_cloudlet.cloud_servers.back().sock;
				//get payload
				thread_arg->mocha_packet->get_payload_from_sock(itr->sock);

				//create thread
		    	HANDLE thrdResult = CreateThread(NULL, 0, mobile_packet_thread, thread_arg, 0, NULL);

	 			//check creation
	  			if(thrdResult == NULL){
			  		cout<<"Thread creation fail!"<<endl;
			  		exit(1);	
				} 
			} 
			else {
			  continue;
			}
		}	
	}	

	//check cloud sockets
	for(vector<cloud_server_c>::iterator itr=my_cloudlet.cloud_servers.begin(); itr!=my_cloudlet.cloud_servers.end();itr++){
		if(FD_ISSET(itr->sock, &s_rd)){

			//check if there is mocha packet in the socket	
			int iResult = mocha_query(itr->sock);
			if(iResult==1){
				cout<<"mocha cloud query success"<<endl;
				//get header
				char *header = new char[MOCHA_HEADER_LENGTH];
				MEM_ASSERT(header);
				iResult = recv(itr->sock, header, MOCHA_HEADER_LENGTH, MSG_PEEK); 
				if(iResult != MOCHA_HEADER_LENGTH){
					cout<<"get header fail!"<<endl;
					exit(1);
				}
				mocha_packet_c *mocha_packet = new mocha_packet_c(header);
				MEM_ASSERT(mocha_packet);

				//initialize the thread args
				struct thread_arg_t *thread_arg = new thread_arg_t;
				MEM_ASSERT(thread_arg);
				thread_arg->mocha_packet=mocha_packet;
				thread_arg->mobile_sock=(SOCKET)mocha_packet->device_id;
				thread_arg->cloud_sock=itr->sock;
				//get payload
				thread_arg->mocha_packet->get_payload_from_sock(itr->sock);

				//create thread
				HANDLE thrdResult = CreateThread(NULL, 0, cloud_packet_thread, thread_arg, 0, NULL);
				//check creation
	  			if(thrdResult == NULL){
			  		cout<<"Thread creation fail!"<<endl;
			  		exit(1);	
				} 
			}
			else{
				continue;
			}
		}	
	}

  }
    

  return 0;
}
