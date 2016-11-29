/* IBM_PROLOG_BEGIN_TAG */
/* 
 * Copyright 2003,2016 IBM International Business Machines Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 		 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* IBM_PROLOG_END_TAG */

/* @(#)24	1.1  src/htx/usr/lpp/htx/bin/htxd/htxcmd_socket.c, htxd, htxfedora 7/17/13 05:14:19 */



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#include "htxclient.h"
#include "htxd_common.h"

#define BACKLOG					10
#define	SELECT_TIMEOUT_SECONDS			10 
#define	SELECT_TIMEOUT_MICRO_SECONDS		0

#define RESPONSE_STRING_LENGTH			10
#define	RESPONSE_ERR_CODE_LENGTH		10


extern htxclient_parameter global_parameter;
char htxclient_global_error_text[1024];



void htxscreen_exit(char *exit_message, int exit_code)
{
	strcpy(htxclient_global_error_text, exit_message);
	strcat(htxclient_global_error_text,strerror(errno));
	exit(exit_code);
}

int htxclient_create_socket(void)
{
	int socket_fd;

	socket_fd = socket (AF_INET, SOCK_STREAM, 0);
	if(socket_fd == -1) {
		perror("ERROR: while creating socket. Exiting...");
		exit(1);
	}

	return socket_fd;
}



int htxclinet_set_socket_option(int socket_fd, struct timeval* timeout )
{
	int result;
	socklen_t option_length = 0;

	result = setsockopt (socket_fd, SOL_SOCKET, SO_REUSEADDR, (void *) timeout, option_length );
	if(result == -1) {
		perror("ERROR: while setting socket options. Exiting...");
		exit(1);
	}

	return result;
}



/* establishing socket connection */
int htxclient_connect(int socket_fd, char *sut_ip, int htxd_port)
{
	int result;
	struct sockaddr_in sut_address;
	struct hostent *p_sut_entry;
	char error_string[512];

	p_sut_entry = gethostbyname(sut_ip);
	if(p_sut_entry == NULL) {
		sprintf(error_string, "ERROR: while calling gethostbyname with hostname <%s>. Exiting...", sut_ip);
		herror(error_string);
		exit(1);
	} 

	sut_address.sin_family = AF_INET;
	sut_address.sin_port = htons(htxd_port);
	sut_address.sin_addr = *((struct in_addr *)(p_sut_entry->h_addr));
	memset(&(sut_address.sin_zero), '\0', 8);

	result = connect(socket_fd, (struct sockaddr *)&sut_address, sizeof(struct sockaddr));	
	if(result == -1) {
		sprintf(error_string, "ERROR: while connecting hostname <%s> and port <%d>. Exiting...", sut_ip, htxd_port);
		htxscreen_exit(error_string, 1);
		perror(error_string);
		exit(1);
	}
	
	return result;
}



/* send command to daemon */
int htxclient_send_command(int socket_fd, char *command_string)
{
	int result;

	/* send the command to daemon */
	/*printf("Sending command string <%s>\n", command_string);
	fflush(stdout);*/
	result = send(socket_fd, command_string, strlen(command_string), 0);
	if(result == -1) {
		perror("ERROR: while sending command to SUT. Exiting...");
		exit(1);
	}

	return result;
}



int htxd_receive_bytes(int new_fd, char * receive_buffer, int receive_length)
{
    int read_bytes;
    int remaining_bytes;

    remaining_bytes = receive_length;
    while(remaining_bytes > 0) {
        read_bytes = recv(new_fd, receive_buffer, remaining_bytes, MSG_WAITALL);
        if(read_bytes == -1) {
            return -1;
        }

        if(read_bytes == 0) {
            break;
        }

        remaining_bytes -= read_bytes;
        receive_buffer += read_bytes;
    }
    return  (receive_length - remaining_bytes);

}



/* receive response from daemon */
int  htxclient_receive_response(int socket_fd, htxclient_response *response)
{
	int return_code;
	char temp_buffer[20];	
	int response_length;


	memset(temp_buffer, 0, sizeof(temp_buffer) );
	return_code = htxd_receive_bytes(socket_fd, temp_buffer, RESPONSE_ERR_CODE_LENGTH);
	if(return_code == -1) {
		exit(1);
	}

	/* printf("command return_code string <%s>\n", temp_buffer); 
	fflush(stdout); */

	response->return_code = atoi(temp_buffer);
	/* printf("response->return_code <%d>\n", response->return_code); */
	fflush(stdout);

	memset(temp_buffer, 0, sizeof(temp_buffer) );
	return_code = htxd_receive_bytes(socket_fd, temp_buffer, RESPONSE_STRING_LENGTH);
	if(return_code == -1) {
		exit(1);
	}

	/* printf("ressponse length string <%s>\n", temp_buffer); 
	fflush(stdout); */

	response_length = atoi(temp_buffer);
	/* printf("ressponse length <%d>\n", response_length); 
	fflush(stdout); */
	if(response->response_buffer == NULL) {
		response->response_buffer = malloc(response_length + 10);
	}
	memset(response->response_buffer, 0, response_length + 10 );
	return_code = htxd_receive_bytes(socket_fd, response->response_buffer, response_length);
	if(return_code == -1) {
		exit(1);
	}

	/*printf("ressponse string <%s>\n", response->response_buffer);
	fflush(stdout);*/

	return return_code;
}



/* prepare string for sending to daemon, as following format
   temp_string =:<command number(10)>:<ecg name>:<option list>: 
   command_string =<length of temp_string(10)><temp_string>	*/
void htxclient_prepare_command_string(htxclient_command *p_command_object, char *command_string)
{
	char temp_string[COMMAND_STRING_LENGTH];

	sprintf(temp_string, ":%010d:%s:%s:", p_command_object->command_index, p_command_object->ecg_name, p_command_object->option_list);
	sprintf(command_string, "%010d%s", (int) strlen(temp_string), temp_string);		
}



int htxclient_close_socket(int socket_fd)
{
	int return_code = 0;


	return_code = close(socket_fd);

	return return_code;
}


void initialize_command_object(htxclient_command *p_command_object, char *command_name)
{

	int i = 0;
	int option_count = sizeof(option_list)/sizeof(option);

	if(global_parameter.sut_hostname[0] == '\0') {
		strcpy(p_command_object->sut_hostname, DEFAULT_SUT_HOSTNAME);
	} else {
		strcpy(p_command_object->sut_hostname, global_parameter.sut_hostname);
	}
	if(global_parameter.daemon_port_number == 0) {
		p_command_object->daemon_port_number = HTXD_DEFAULT_PORT;
	} else {
		p_command_object->daemon_port_number = global_parameter.daemon_port_number;
	}

	strcpy(p_command_object->command_name, command_name);

	p_command_object->command_index = -1;
	while(i < option_count) {
		//if(strncmp(p_command_object->command_name, option_list[i].option_string, strlen(option_list[i].option_string) ) == 0 ) {
		if(strcmp(p_command_object->command_name, option_list[i].option_string) == 0 ) {
			p_command_object->command_index = i;
			break;
		}
		i++;
	}
	p_command_object->ecg_name[0] = '\0';
	p_command_object->option_flag = FALSE;
	p_command_object->option_list[0] = '\0';
	p_command_object->command_argument_vector_position = -1;
}


void initialize_response_object(htxclient_response *p_response_object)
{
	p_response_object->response_buffer      = NULL;
	p_response_object->return_code          = 0;
}
