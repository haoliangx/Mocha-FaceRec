//Description:main function for the MOCHA cloud server
#include "mocha_cloud.h"
#include "mocha.h"
#include "facerec.h"
#include <random>
#include <iostream>

using namespace std;
using namespace cv;
using namespace gpu;

#define DELAY_LINE_SIZE 100
#define LOCATION_SIZE 30

double PCFreq = 0.0;
HANDLE total_time_mutex;
HANDLE total_packet_mutex;
double total_time = 0;
long total_packet = 0;

#ifdef DELAY_SIMULATE
struct server_profile_t remote_server;
#endif

int main(int argc, char **argv)
{
	//prepare for performance measurment, sample performance measure code: http://www.netperf.org/svn/netperf2/trunk/src/netcpu_ntperf.c
	LARGE_INTEGER li;
	if(!QueryPerformanceFrequency(&li))
		cout << "QueryPerformanceFrequency failed!\n";
	PCFreq = double(li.QuadPart)/1000.0;

	//initializing mutex for performance measurment purpose
	total_time_mutex = CreateMutex(NULL, FALSE, NULL);
	total_packet_mutex = CreateMutex(NULL, FALSE, NULL); 
	if(total_time_mutex==NULL || total_packet_mutex==NULL){
	     cout<<"create mutex error"<<endl;
	     exit(1);
	}

#ifdef DELAY_SIMULATE //delay simulation flag, use the delay.txt file to simulate server delay
	char line[DELAY_LINE_SIZE];
	char location_in[LOCATION_SIZE];
	FILE *fp = fopen("delay.txt", "r");
	if(fp==NULL){
		cout<<"open delay.txt fail!"<<endl;
		exit(1);
	}
	vector<server_profile_t> server_profile;
	int counter = 0;
	while(fgets(line, 100, fp)){
		struct server_profile_t new_server;
		sscanf(line, "%lf %lf %lf %lf %s", &new_server.mean_a, &new_server.mean_b, &new_server.std_a, &new_server.std_b, location_in);
		new_server.location = string(location_in);
		cout<<counter<<". "<<new_server.location<<endl;
		server_profile.push_back(new_server);
		counter++;
	}
#endif

#ifdef NON_DEFAULT_NON_TEST //enable this flag to enter your own service port, otherwise use default port:3200
	int port_in;
	cout<<"Please enter service port:";
	cin>>port_in;
	cloud_c my_cloud(port_in);
#else
	cloud_c my_cloud;
#endif

  	int iteration=1;

	//face rec initialization
	Mat temp;
	vector<Mat> temp_faces;
	vector<Rect> temp_rect;
	face_t temp_result;
	//for any face* functions you are gonna to use, you need to do initialization
	//faceDetect_GPU(temp,temp_rect, temp_faces, 1);
	//faceRecCloud(temp, temp_result, 1);
	faceRecLocal(temp, temp_result, 1);
	
	cout<<"Vector rec initialization complete"<<endl;

	//create cloud server
	my_cloud.server_run();
	cout<<"wait cloudlet to connect"<<endl;

	fd_set s_rd;
	FD_ZERO(&s_rd);

	//register all the sockets, so that we can do non-blocking I/O
	for(;;){
		//register cloudlet servers
		for(vector<SOCKET>::iterator itr=my_cloud.cloudlet_socks.begin();itr!=my_cloud.cloudlet_socks.end();itr++){
			FD_SET(*itr, &s_rd);
		}

		//register local server
		FD_SET(my_cloud.listend, &s_rd);
		select(0, &s_rd, NULL, NULL, NULL);

		//check new cloudlet connection
		if(FD_ISSET(my_cloud.listend, &s_rd)){
  			SOCKET new_sock = accept(my_cloud.listend, NULL, NULL);
   			if(new_sock == INVALID_SOCKET){
				cout<<"accept failed with error: "<<WSAGetLastError()<<endl;
				closesocket(my_cloud.listend);
				WSACleanup();
				exit(1);
   			}
			else{
				my_cloud.cloudlet_socks.push_back(new_sock);
				cout<<"New cloud cloudlet connection estabilshed"<<endl;
#ifdef DELAY_SIMULATE
				cout<<"What server do you want to choose?"<<endl;
				scanf("%d", &counter);
				cout<<"The server will be at "<<server_profile[counter].location<<endl;
				remote_server = server_profile[counter];
#endif
   			}
		}
		//finish register all the servers

		//check cloudlet sockets
		for(vector<SOCKET>::iterator itr=my_cloud.cloudlet_socks.begin();itr!=my_cloud.cloudlet_socks.end();itr++){
			//if there is something in the socket
			if(FD_ISSET(*itr, &s_rd)){			

				int iResult = 0;
				//query if there is mocha packets in the socket
				iResult = mocha_query(*itr);	
				if(iResult==1){
		    			//there is mocha packet at socket
		    			cout<<"mocha query success"<<endl;
		    			//get header
		    			char *header = new char[MOCHA_HEADER_LENGTH];
		    			MEM_ASSERT(header);
		    			iResult = recv(*itr, header, MOCHA_HEADER_LENGTH, MSG_PEEK); 
	            			if(iResult != MOCHA_HEADER_LENGTH){
	   		  			cout<<"get header fail!"<<endl;
			  			exit(1);
		     			}	
						//create mocha packet from the header, this mocha packet will be handed to a thread
		     			mocha_packet_c *mocha_packet = new mocha_packet_c(header);
		     			MEM_ASSERT(mocha_packet);

		    			cout<<"iteration "<<iteration++<<endl;

	              		 //initialize the thread args
		      			 struct thread_arg_t *thread_arg = new thread_arg_t;
		      			 MEM_ASSERT(thread_arg);
		      			 thread_arg->mocha_packet = mocha_packet;
		      			 thread_arg->cloudlet_sock = *itr;
		      			 //get payload
		      			 thread_arg->mocha_packet->get_payload_from_sock(*itr);				
					 
		      			 //create thread
		      			 HANDLE thrdResult = CreateThread(NULL, 0, cloudlet_packet_thread, thread_arg, 0, NULL);

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
	}	
	
	return 0;
}


