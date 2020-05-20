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
#define SERVER_PORT 7002



typedef struct {
Socket *server_socket;
int client_id;
int thread_id; 
}thread_data_t;


void* run(void* data) { 

	char* msg_from_client = (char*)calloc(512, sizeof(char));  

	thread_data_t* thread_data = (thread_data_t*)data;

	if(-1 == thread_data->server_socket->Read(msg_from_client,512,thread_data->client_id)) {
		perror("there was an error");
	}
	
	char* filename = get_file_name(msg_from_client);

	char* response_header = make_response_header(filename, 0); 		//ese 0, deberian ser la cantidad de bytes del archivo. 
	
	char* buffer_to_read_file = (char*)calloc(1024, sizeof(char)); 
	
	strcpy(buffer_to_read_file, response_header); 
	int len_header = strlen(buffer_to_read_file);  
	int read_status = 0;  
	int bytes_read = 0; 
	bool end_read = false; 
	
	int file_id = open(filename, O_RDONLY); 

	
	if (-1 != file_id) {		
		while (!end_read) {
			
			if (bytes_read != 0) {
				if((read_status = read(file_id, buffer_to_read_file, 1024)) != 0) {
					
					thread_data->server_socket->Write(buffer_to_read_file, thread_data->client_id);
					memset(buffer_to_read_file, 0, 1024 * sizeof(char)); 	
					bytes_read+= read_status; 		
				
				}
				else {
					end_read = true; 	
				} 		
			}
			else {
				read_status = read(file_id, buffer_to_read_file+len_header, 1024-len_header);  
				thread_data->server_socket->Write(buffer_to_read_file, thread_data->client_id);
				memset(buffer_to_read_file, 0, 1024 * sizeof(char)); 	
				bytes_read+= read_status; 
			}	
		}	
			
	}
	else {
		//el archivo no existe, y hay que responderle al cliente, que lo que pide no existe. 
	}

	free(msg_from_client);
	free(buffer_to_read_file); 
	thread_data->server_socket->Shutdown(thread_data->client_id);
}



int main(int argc, char* argv[]) {
	
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


return 0;
}
