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
/* @(#)68	1.1  src/htx/usr/lpp/htx/bin/htxd/htxd_socket.h, htxd, htxubuntu 7/17/13 09:12:06 */



#ifndef HTXD__SOCKET__HEADER
#define HTXD__SOCKET__HEADER


extern int htxd_create_socket(void);
extern int htxd_set_socket_option(int);
extern int htxd_bind_socket(int, struct sockaddr_in *, int);
extern int htxd_listen_socket(int);
extern int htxd_select(int);
extern int htxd_select_timeout(int, int);
extern int htxd_accept_connection(int, struct sockaddr_in *, socklen_t *);
extern int htxd_receive_command_length(int);
extern char * htxd_receive_command(int);
extern int htxd_send_ack_command_length(int);
extern int htxd_send_response_length(int, int);
extern int htxd_send_response(int, char *, int, int);
extern int htxd_receive_bytes(int, char *, int);
extern int htxd_send_heart_beat_response(int , char *);


#endif
