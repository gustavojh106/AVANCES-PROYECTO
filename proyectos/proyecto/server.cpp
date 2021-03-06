#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include "socket.h"
#include <pthread.h>
#include <queue>
#include <arpa/inet.h>
#include "http_parser.h"
#include <list>
int SERVER_PORT = 7000;
#define B_PORT 65000
#define TEST 0


#define RED true

//#define LOCAL true


char * MY_IP;

typedef struct {
	std::list<ip_port_t*> * balancers; 
	Socket * r_socket; 
	Socket * s_socket; 
} listener_data_t; 

typedef struct {
	Socket *server_socket;
	int client_id;
	int thread_id; 
} thread_data_t;



bool its_a_server(char* msg) {
	return (msg[0] == 'S'); 
}

void* listen_balancers(void* data) {
	listener_data_t* listener_data = (listener_data_t*)data; 
	Socket* r_socket = listener_data->r_socket; 
	Socket* s_socket = listener_data->s_socket; 
	std::list<ip_port_t*> * balancers = listener_data->balancers; 
	
	sockaddr_in s_in;
	char buffer[120];

	#ifdef RED
	std::string ips(MY_IP);
	std::string balancer_seek_msg = "S/C/" + ips + "/" + std::to_string(SERVER_PORT);
		char * msgp = (char*)balancer_seek_msg.c_str();
	#endif
	
	#ifdef LOCAL
		char * msgp = (char *)"S/C/127.0.0.1/7000";	//cuando me hable por stream, hableme por el 7000. 
	#endif
	//int n = s_socket->SendTo((char *)"172.16.123.111", &s_in, B_PORT, msgp, strlen(msgp));
	int n = s_socket->SendTo((char *)"172.16.123.31", &s_in, B_PORT, msgp, strlen(msgp));
	printf("Me acabo de levantar, broadcast enviado.\n");

	//r_socket->Bind(B_PORT + 1, 0);
	
	while(true) {
		memset(buffer, 0, 120);
		memset(&s_in, 0, sizeof(sockaddr_in)); 
		n = r_socket->ReceiveFrom(&s_in, buffer, 120);

		if (n != -1 && !its_a_server(buffer)) {	
			ip_port_t* balancer = build_ip_port(buffer); 
			printf("New Balancer IP : [%s] - Port : [%d] \n", balancer->ip_address, balancer->port); 
			balancers->push_back(balancer);  			
			// send	
			char * addr = inet_ntoa(s_in.sin_addr);
			printf("Sending response to : [%s] to Port : [%d] \n", addr, B_PORT);
			Socket socket('d', false);
			
			// 1. old way
			//n = socket.SendTo(&s_in, true,B_PORT, msgp, strlen(msgp));
			// 2 . new way
			sockaddr_in socka;
			socket.SendTo(addr, &socka, B_PORT, msgp, strlen(msgp));

			printf("response sent \n"); 
			socket.Shutdown();
		}	
		else {
			//printf("ERROR: no se pudo encontrar balanceador en la red \n");
		}		
	}
}


void* run(void* data) { 

	char* msg_from_client = (char*)calloc(512, sizeof(char));  

	thread_data_t* thread_data = (thread_data_t*)data;

	if(-1 == thread_data->server_socket->Read(msg_from_client,512,thread_data->client_id)) {
		perror("there was an error");
	}
	printf("Message from client: %s\n", msg_from_client);
	
	int head_request = 0; 
	
	char* filename = get_file_name(msg_from_client, &head_request);

	if (head_request) {
		printf("el file name es : %s \n", filename); 
	}
	
	char* response_header; 
	
	int option = -1; 
	
	if (!strcmp(filename, "400 Bad Request")) {
		response_header = make_response_header(filename, 0, 400);
		option = 400;  
	}
	else {
		if (!strcmp(filename, "505 HTTP Version Not Supported")) {
			response_header = make_response_header(filename, 0, 505); 
			option = 505; 
		}
		else {
			if (!strcmp(filename, "501 Not Implemented")) {
				response_header = make_response_header(filename, 0, 501); 
				option = 501; 
			}
			else {
				response_header = make_response_header(filename, 0, 200); 		//ese 0, deberian ser la cantidad de bytes del archivo. 
				option = 200; 
			}
		}
	}
	
	if (option == 200) {
		
		
			char* buffer_to_read_file = (char*)calloc(1024, sizeof(char)); 
			
			strcpy(buffer_to_read_file, response_header); 
			
			
			int len_header = strlen(buffer_to_read_file);  
			int read_status = 0;  
			int bytes_read = 0; 
			bool end_read = false;
			int file_id = -1;
			bool directorio = es_directorio(filename);
			if(directorio){
				char * temp = new char [200];
				temp[0] = '/';
				strcpy(temp+1,filename);
				file_id = open(temp, O_RDONLY);
				//delete temp;
			}
			else{
				file_id = open(filename, O_RDONLY);
			}
			if (-1 != file_id && !head_request) {
				
				
				while(end_read == false) {
					if (bytes_read != 0) {
						if((read_status = read(file_id, buffer_to_read_file, 1024)) != 0) {
							
							thread_data->server_socket->Write(buffer_to_read_file, thread_data->client_id, read_status);
							memset(buffer_to_read_file, 0, 1024 * sizeof(char)); 	
							bytes_read+= read_status; 
							
						}
						else {
							end_read = true; 	
						} 		
					}
					else {
						read_status = read(file_id, buffer_to_read_file+len_header, 1024-len_header); 
						thread_data->server_socket->Write(buffer_to_read_file, thread_data->client_id, 1024);
						memset(buffer_to_read_file, 0, 1024 * sizeof(char)); 	
						bytes_read+= read_status;
						
					}	
				}
				
				
						
			}
			else {
				if (-1 == file_id) {
					memset(buffer_to_read_file, 0, 1024 * sizeof(char)); 
					free(response_header);
					response_header = make_response_header("", 0, 404);
					thread_data->server_socket->Write(response_header, thread_data->client_id, strlen(response_header)); 
				}
				else {
					thread_data->server_socket->Write(response_header, thread_data->client_id, strlen(response_header)); 
				}
				//aqui sería hacer el 404 not found. 
			}
			
			
			
	}
	else {
			thread_data->server_socket->Write(response_header, thread_data->client_id, strlen(response_header)); 
	}
	
	//free(filename); 
	// free(response_header); 
	// free(msg_from_client);
	// free(buffer_to_read_file); 
	thread_data->server_socket->Shutdown(thread_data->client_id);
	//free(thread_data); 
	void * asd;
	pthread_exit(asd);
}



int main(int argc, char* argv[]) {
	
	pthread_t listener_balancers; 
	listener_data_t listener_data; 
	memset(&listener_data, 0, sizeof(listener_data_t));
	Socket r_socket('d', false);
	r_socket.Bind(B_PORT+TEST); 
	Socket s_socket('d', false);
	s_socket.EnableBroadcast();
	std::list<ip_port_t*> balancers;	 
	listener_data.balancers = &balancers; 
	listener_data.r_socket = &r_socket;
	listener_data.s_socket = &s_socket;

	#ifdef RED
		if (argc != 3) {
			printf("parametros invalidos \n");
			return 0;  
		}
		MY_IP = argv[1];
		SERVER_PORT = atoi(argv[2]);	
	#endif



	pthread_create(&listener_balancers, NULL, listen_balancers, (void*)&listener_data); 
	
	Socket server_socket('s', false);

	if(-1 == server_socket.Bind(SERVER_PORT, 0)){ // 0 = ipv4.
	perror("there was an error");
	}

	if(-1 == server_socket.Listen(10)) { 
	perror("there was an error");
	}

	int contador_threads = 0; 

	while(1) {

		struct sockaddr_in socket_client;

		int client_id = server_socket.Accept(&socket_client);
		
		if (-1 == client_id) {
			perror("something went wrong");
		}
		else {
			thread_data_t* thread_data = (thread_data_t*)calloc(1,sizeof(thread_data_t)); 		//hay que darle free a esto. 
			thread_data->server_socket = &server_socket;
			thread_data->client_id = client_id;  
			thread_data->thread_id = ++contador_threads; 
			pthread_t* thread = (pthread_t*)malloc(1*sizeof(pthread_t)); 

			pthread_create(thread, NULL, run, thread_data); 
		}
	}

	

	//server_socket.Shutdown(client_id); 

pthread_join(listener_balancers, NULL); 

return 0;
}
