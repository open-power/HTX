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
/* @(#)67	1.2  src/htx/usr/lpp/htx/bin/htxd/htxd_socket.c, htxd, htxubuntu 2/5/15 00:50:58 */



#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>

#include "htxd_option.h"
#include "htxd_trace.h"


#define BACKLOG 10
/* #define	SELECT_TIMEOUT_SECONDS			600 */
#define	SELECT_TIMEOUT_SECONDS			100
#define	SELECT_TIMEOUT_MICRO_SECONDS	0

#define COMMAND_HEADER_LENGTH 16
#define COMMAND_TAILER_LENGTH 16
#define COMMAND_STRING_LENGTH 10



/* creating a socket */
int htxd_create_socket(void)
{
	int	socket_fd;
	char	trace_string[256];


	socket_fd = socket (AF_INET, SOCK_STREAM, 0);
	if(socket_fd == -1)
	{
		sprintf(trace_string, "ERROR: while creating socket. Exiting... errno = %d, return_code = %d", errno, socket_fd); 
		HTXD_TRACE(LOG_ON, trace_string);
		perror(trace_string);
		exit(1);
	}

	if (fcntl(socket_fd, F_SETFD, FD_CLOEXEC) == -1) {
		sprintf(trace_string, "ERROR: while setting FD_CLOEXEC on socket. Exiting... errno = %d, return_code = %d", errno, socket_fd);
		HTXD_TRACE(LOG_ON, trace_string);
		perror(trace_string);
		exit(1);
	}

	return socket_fd;
}



/* setting socket option */
int htxd_set_socket_option(int socket_fd)
{
	int	result;
	int	option_value = 1;
	char	trace_string[256];


	result = setsockopt (socket_fd, SOL_SOCKET, SO_REUSEADDR, (void *) &option_value, sizeof (option_value) );
	if(result == -1)
	{
		sprintf(trace_string, "ERROR: while setting socket options. Exiting... errno = %d, return_code = %d", errno, socket_fd); 
		HTXD_TRACE(LOG_ON, trace_string);
		perror(trace_string);
		exit(1);
	}

	return result;
}



/* bind socket */
int htxd_bind_socket(int socket_fd, struct sockaddr_in *local_address, int port_number)
{
	int	result;
	char	trace_string[256];


	result = bind (socket_fd, (struct sockaddr *) local_address, sizeof (struct sockaddr));
	if(result == -1)
	{
		sprintf(trace_string, "ERROR: while binding connection. Exiting... errno = %d, return_code = %d", errno, socket_fd); 
		HTXD_TRACE(LOG_ON, trace_string);
		perror(trace_string);
		exit(1);
	}

	return result;
}



/* listening connection */
int htxd_listen_socket(int socket_fd)
{
	int	result;
	char	trace_string[256];


	result = listen (socket_fd, BACKLOG);
	if(result == -1)
	{
		sprintf(trace_string, "ERROR: while listening connection. Exiting... errno = %d, return_code = %d", errno, socket_fd); 
		HTXD_TRACE(LOG_ON, trace_string);
		perror(trace_string);
		exit(1);
	}

	return result;
}



/* select for reading */
int htxd_select(int socket_fd)
{
	int		result;
	fd_set		read_fd_set;
	struct timeval	timeout;
	char		trace_string[256];

	FD_ZERO (&read_fd_set);

	timeout.tv_sec	= SELECT_TIMEOUT_SECONDS;
	timeout.tv_usec	= SELECT_TIMEOUT_MICRO_SECONDS;

	FD_SET (socket_fd, &read_fd_set);

	result = select (socket_fd + 1, &read_fd_set, NULL, NULL, &timeout);
	if( (result == -1) && (errno != EINTR) ) {
		sprintf(trace_string, "ERROR: while selecting connection. Exiting... errno = %d, return_code = %d", errno, socket_fd); 
		HTXD_TRACE(LOG_ON, trace_string);
		perror(trace_string);
		exit(1);
	}

	return result;
}




/* select with timeout for reading */
int htxd_select_timeout(int socket_fd, int timeout_seconds)
{
	int		result;
	fd_set		read_fd_set;
	struct timeval	timeout;
	char		trace_string[256];

	FD_ZERO (&read_fd_set);

	timeout.tv_sec	= timeout_seconds;
	timeout.tv_usec	= SELECT_TIMEOUT_MICRO_SECONDS;

	FD_SET (socket_fd, &read_fd_set);

	result = select (socket_fd + 1, &read_fd_set, NULL, NULL, &timeout);
	if( (result == -1) && (errno != EINTR) ) {
		sprintf(trace_string, "ERROR: while selecting connection. Exiting... errno = %d, return_code = %d", errno, socket_fd); 
		HTXD_TRACE(LOG_ON, trace_string)
		perror(trace_string);
		exit(1);
	}

	return result;
}



/* accept a connection */
int htxd_accept_connection(int socket_fd, struct sockaddr_in *p_client_address, socklen_t *p_address_length)
{
	int	new_fd;
	char	trace_string[256];


	memset((void*)p_client_address, 0, sizeof(struct sockaddr));
	*p_address_length = sizeof (struct sockaddr);
	new_fd = accept (socket_fd, (struct sockaddr *)p_client_address, p_address_length);
	if( (new_fd == -1) && (errno != EINTR) ) {
		sprintf(trace_string, "ERROR: while accepting connection. Exiting... errno = %d, return_code = %d", errno, socket_fd); 
		HTXD_TRACE(LOG_ON, trace_string)
		perror(trace_string);
		exit(1);
	}

	return new_fd; 
}



/* length received ack */
int htxd_send_ack_command_length(int new_fd)
{
	int result;
	char ack_string[] = ":length received:";

	
	result = send (new_fd, ack_string, strlen (ack_string), 0);
	if(result == -1)
	{
		return result;
	}

	return result;
}



/* receive bytes */
int htxd_receive_bytes(int new_fd, char * receive_buffer, int receive_length)
{
	int	received_bytes;
	int	remaining_bytes;


	remaining_bytes = receive_length;
	while(remaining_bytes > 0) {
		received_bytes = recv(new_fd, receive_buffer, remaining_bytes, 0);
		if(received_bytes == -1) {
			return -1;
		}

		if(received_bytes == 0) {
			break;
		}

		remaining_bytes -= received_bytes;
		receive_buffer += received_bytes;
	}	
	return 	(receive_length - remaining_bytes);
}



/* incomming command string receives here */
char * htxd_receive_command(int new_fd)  /* note: have to change for error handling */
{
	int	result;
	char	*command_details_buffer = NULL;
	char	temp_buffer[20];
	int	command_length;
	char	trace_string[256];


	memset(temp_buffer, 0, sizeof(temp_buffer) );
	/* receiving command  length from incomming commend */
	result = htxd_receive_bytes(new_fd, temp_buffer, 10);
	if(result == -1)
	{
		sprintf(trace_string, "htxd_receive_bytes returned -1"); 
		HTXD_TRACE(LOG_ON, trace_string);
		return NULL;
	}

	temp_buffer[COMMAND_STRING_LENGTH] = '\0';

	command_length = atoi(temp_buffer);

	/* now we are ready to receive the command string */
	command_details_buffer = malloc(command_length + 10);
	if(command_details_buffer == NULL)
	{
		sprintf(trace_string, "malloc() failed while allocating command_details_buffer"); 
		HTXD_TRACE(LOG_ON, trace_string)
		return NULL;
	} 

	memset(command_details_buffer, 0, command_length + 10);

	/* receiving the command string */
	result = htxd_receive_bytes(new_fd, command_details_buffer, command_length);
	if(result == -1)
	{
		return NULL;
	}

	command_details_buffer[command_length] = '\0';

	return command_details_buffer;
}



/* send response to a client */
int htxd_send_response(int new_fd, char *command_result, int command_type, int command_return_code)
{
	int result = 0;
	char *response_buffer = NULL;
	int buffer_length;
	int number_of_bytes_sent;
	int cumulative_number_of_bytes_sent = 0;
	int number_of_bytes_to_send;


	if(command_type == HTXCMDLINE_COMMAND) {
	buffer_length = strlen(command_result) + 10 + 10 + 10;
		response_buffer = malloc(buffer_length);
		memset(response_buffer, 0, buffer_length);
		sprintf(response_buffer, "%010d%010d%s", command_return_code,strlen(command_result), command_result);
		number_of_bytes_to_send = strlen(response_buffer);

		while(cumulative_number_of_bytes_sent < number_of_bytes_to_send) {
			number_of_bytes_sent = send(new_fd, response_buffer + cumulative_number_of_bytes_sent, number_of_bytes_to_send - cumulative_number_of_bytes_sent, 0);
			if(number_of_bytes_sent == -1)
			{
				printf("[DEBUG] : Error : htxd_send_response() send() returns -1, errno <%d\n", errno);
			}
			cumulative_number_of_bytes_sent += number_of_bytes_sent;
		}
	} else if(command_type == HTXSCREEN_COMMAND) {
		response_buffer = malloc( (10 * 1024) + 30 );
		sprintf(response_buffer, "%010d%010d", command_return_code, (10 * 1024));
		memcpy(response_buffer + 20, command_result, 10 * 1024);
		number_of_bytes_to_send = (10 * 1024) + 20;

		while(cumulative_number_of_bytes_sent < number_of_bytes_to_send) {
			number_of_bytes_sent = send(new_fd, (response_buffer + cumulative_number_of_bytes_sent) , number_of_bytes_to_send - cumulative_number_of_bytes_sent, 0);
			cumulative_number_of_bytes_sent += number_of_bytes_sent;
		}
	}
	
	free(response_buffer);	
	
	return result;
}



int htxd_send_heart_beat_response(int new_fd, char *response_buffer)
{
	int cumulative_number_of_bytes_sent = 0;
	int number_of_bytes_to_send = 1;
	int number_of_bytes_sent;

	while(cumulative_number_of_bytes_sent < number_of_bytes_to_send) {
		number_of_bytes_sent = send(new_fd, (response_buffer + cumulative_number_of_bytes_sent), number_of_bytes_to_send - cumulative_number_of_bytes_sent, 0);
		cumulative_number_of_bytes_sent += number_of_bytes_sent;
	}

	return 0;
}

