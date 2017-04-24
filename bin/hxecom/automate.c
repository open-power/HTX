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

#include <string.h>

#include "automate.h" 
#define STRERROR(E) (E >= 0 && E <= sys_nerr)? sys_errlist[E] : ""

/* Global Data Structures */ 
struct thread_context t_ctx[MAX_INTERFACE];

/* thread specific variables, applicable to worker threads */
pthread_mutex_t create_thread_mutex;
pthread_attr_t thread_attrs;
pthread_cond_t create_thread_cond;
pthread_cond_t start_thread_cond;


int 
main(int argc, char * argv[]) { 

	int i, rc, j , k, l ; 
	int num_devices = 0; 
	struct dev_details * dev_d; 
	int tid; 
	struct thread_context * tctx; 
	void * join_status;
	struct nw_setup nw_topology[MAX_INTERFACE];

	if(argc < 3) { 
		#ifdef __HTX_LINUX__
			printf("Usage %s : eth0 eth1 \n", argv[0] );
		#else 
			printf("Usage %s : en0 en1 \n", argv[0] );
		#endif
		return(-1); 
	} 
	num_devices = argc - 1;
	if(num_devices > MAX_INTERFACE) 
		num_devices = MAX_INTERFACE; 

	/* Allocate space for each device details structure */ 	
	dev_d = (struct dev_details *)malloc(sizeof(struct dev_details) * num_devices); 
	if(dev_d == (struct dev_details *)NULL) { 
		printf("Unable to allocate space for device structure, errno = %d \n", errno); 
		return(-1); 
	} 

	for(i = 0; i < num_devices; i++) { 	
		 strcpy(&dev_d[i].name, argv[i + 1]); 
		/* Find the mac-address of each interfaces */ 
		rc = get_hwaddr(&dev_d[i].name, dev_d[i].mac_addr);   
		if(rc == -1) { 
			printf(" Unable to get mac address for interface = %s \n", dev_d->name); 
			return(-1);
        	}
	} 
	/* 	  
	for(i = 0; i < num_devices; i++) {
		printf("dev_name=%s, ",dev_d[i].name ); 
		pit("mac_address=",dev_d[i].mac_addr, 6); 
		printf("\n");
	}
	*/  
	/* Now create pthreads for each interface
     * One thread will send and other will recv. So 2
     * threads per interface
     */
    pthread_attr_init(&thread_attrs);
    pthread_attr_setdetachstate(&thread_attrs, PTHREAD_CREATE_JOINABLE);
    pthread_attr_setscope(&thread_attrs, PTHREAD_SCOPE_PROCESS);
    pthread_mutex_init(&create_thread_mutex, NULL);
    pthread_cond_init(&create_thread_cond, NULL);
    pthread_cond_init(&start_thread_cond, NULL);

    rc = pthread_mutex_lock(&create_thread_mutex);
    if (rc) {
        printf( "pthread_mutex_lock failed with rc = %d, errno = %d \n", rc, errno);
        return (rc);
    }

    for(tid = 0; tid < num_devices; tid++) {
        tctx = &t_ctx[tid];
        tctx->tid = tid;
        tctx->retries = NUM_RETRIES;
        memcpy(&tctx->source.name[0], &dev_d[tid].name[0], 32);
        memcpy(&tctx->source.mac_addr[0], &dev_d[tid].mac_addr[0], 6);
        memset(&tctx->dest, 0, sizeof(struct dev_details));
        memcpy(&tctx->dev_list, dev_d, (sizeof(struct dev_details) * num_devices));
        tctx->num_devices = num_devices;

        /* Create the worker thread */
        rc = pthread_create(&tctx->worker, &thread_attrs, tc_worker_thread, (void *)tctx);
        if(rc ) {
            printf(" failed to create worker thread with rc = %d, errnp = %d \n", rc, errno);
            break;
        }
        rc = pthread_cond_wait(&create_thread_cond, &create_thread_mutex);
        if(rc) {
            printf("pthread_cond_wait failed with rc = %d, errno = %d \n", rc, errno);
            break;
        }
        if(rc) {
            break;
        }
    }
    /* check if we are here due to error in creating threads */
    if(rc) {
        printf("No of thread actually created = %d \n", tid);
        for(i = 0; i < tid; i++) {
            printf(" Cancelling thread tid = %d, id = %d icoz of error \n", tid, tctx->worker);
            pthread_cancel(tctx->worker);
        }

        return(rc);
    } else {
        /* All the worker thread created successfully, start them */
        rc = pthread_cond_broadcast(&start_thread_cond);
        if(rc) {
            printf("pthread_cond_broadcast failed with rc = %d, errno = %d \n", rc, errno);
            return(rc);
        }
        rc = pthread_mutex_unlock(&create_thread_mutex);
        if (rc) {
            printf(" pthread_mutex_unlock failed with rc = %d, errno =  %d \n", rc, errno);
            return(rc);
        }
        /* Worker thread started, wait for completion */
        for(i = 0; i < tid; i++) {
            rc = pthread_join(t_ctx[i].worker, &join_status);
            if(rc) {
            	t_ctx[i].worker_join_rc = *((int *)join_status);
                printf("tid = %d, rc = %d, join status = %x \n", i, rc, t_ctx[i].worker_join_rc);
                break;
            }
        }
    }

    memset(nw_topology, 0, sizeof(struct nw_setup) * MAX_INTERFACE);
    int count = 0, found_unique = 0;
    for(i = 0; i < tid; i ++) {
        if(t_ctx[i].rcvd_reply) {
            for(j = 0; j < t_ctx[i].num_reply; j++) {
                found_unique = 0;
                for(k = 0; k < count; k++) {
                    if(strcmp(&nw_topology[k].src[0], &t_ctx[i].source.name[0]) && strcmp(&nw_topology[k].dest[0], &t_ctx[i].dest[j].name[0]) &&
                       strcmp(&nw_topology[k].src[0], &t_ctx[i].dest[j].name[0])&& strcmp(&nw_topology[k].dest[0], &t_ctx[i].source.name[0])) {
                        found_unique++;
                    }
                }
                if(found_unique == count ) {
                    strcpy(&nw_topology[count].src[0], &t_ctx[i].source.name[0]);
                    strcpy(&nw_topology[count].dest[0], &t_ctx[i].dest[j].name[0]);
                    printf("%s -> %s \n", &nw_topology[count].src[0], &nw_topology[count].dest[0]);
                    fflush(stdout);
                    count++;
                    found_unique = 0 ;
                    break;
                }

            }
        }
    }
    fflush(stdout);
    /* free thread storage area */
    pthread_attr_destroy(&thread_attrs);
    pthread_cond_destroy(&create_thread_cond);
    pthread_cond_destroy(&start_thread_cond);
    pthread_mutex_destroy(&create_thread_mutex);

    return(0);
}

#ifdef __HTX_LINUX__ 
int
get_hwaddr( char * device, char hw_addr[]) {
    struct ifreq ifr;
    int fd;
    /*
     *  Create dummy socket to perform an ioctl upon.
     */
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        printf("Unable to create a dummy socket \n");
        return(-1);
    }
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, device, sizeof(ifr.ifr_name) - 1);
    ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';
    if (ioctl(fd, SIOCGIFHWADDR, (int8_t *)&ifr) < 0) {
        close(fd);
        printf("Error to get Hardware address \n");
        return(-1);
    }
	/* 
    printf("Device %s -> Ethernet %02x:%02x:%02x:%02x:%02x:%02x\n", ifr.ifr_name,
      (int) ((unsigned char *) &ifr.ifr_hwaddr.sa_data)[0],
      (int) ((unsigned char *) &ifr.ifr_hwaddr.sa_data)[1],
      (int) ((unsigned char *) &ifr.ifr_hwaddr.sa_data)[2],
      (int) ((unsigned char *) &ifr.ifr_hwaddr.sa_data)[3],
      (int) ((unsigned char *) &ifr.ifr_hwaddr.sa_data)[4],
      (int) ((unsigned char *) &ifr.ifr_hwaddr.sa_data)[5]);
	*/ 
    hw_addr[0] = (int) ((unsigned char *) &ifr.ifr_hwaddr.sa_data)[0];
    hw_addr[1] = (int) ((unsigned char *) &ifr.ifr_hwaddr.sa_data)[1];
    hw_addr[2] = (int) ((unsigned char *) &ifr.ifr_hwaddr.sa_data)[2];
    hw_addr[3] = (int) ((unsigned char *) &ifr.ifr_hwaddr.sa_data)[3];
    hw_addr[4] = (int) ((unsigned char *) &ifr.ifr_hwaddr.sa_data)[4];
    hw_addr[5] = (int) ((unsigned char *) &ifr.ifr_hwaddr.sa_data)[5];


    return(0);
}
#else 
/*
 * Get the MAC address of the ethernet adapter we're using...
 */
int
get_hwaddr(char *device, char *addr) {  

    int size;
    struct kinfo_ndd *nddp;
    void *end;
    int found = 0;
    size = getkerninfo(KINFO_NDD, 0, 0, 0);
    if (size == 0) {
         printf( "%s : No ndds.\n", __FUNCTION__);
         return(-1);
     }
          
    if (size < 0) {
        printf("%s : getkerninfo 1", __FUNCTION__);
        return(-1);
     }
     nddp = (struct kinfo_ndd *)malloc(size);
     if (!nddp) {
         printf("%s : malloc failed \n", __FUNCTION__);
       	 return(-1);  
     }
     if (getkerninfo(KINFO_NDD, nddp, &size, 0) < 0) {
         printf("%s : getkerninfo failed 2 ", __FUNCTION__);
         return(2);
     }
     end = (void *)nddp + size;
     while (((void *)nddp < end) && !found) {
     if (!strcmp(nddp->ndd_alias, device) ||
         !strcmp(nddp->ndd_name, device)) {
          found++;
          bcopy(nddp->ndd_addr, addr, 6);
     } else
          nddp++;
     }
	if(found)
		return(0);
	else 
		return(-1); 
}
#endif

int
get_ipaddress(char * device, char * ip_addr) {

    struct ifreq ifr;
    register struct sockaddr_in *sin;
    int fd;

    /* create dummy socket to perform an ioctl upon */
    fd = socket(PF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        printf("Unable to create a dummy socket \n");
        return(-1);
    }
    sin = (struct sockaddr_in *)&ifr.ifr_addr;
    strncpy(ifr.ifr_name, device, sizeof(ifr.ifr_name) -1);
    ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';
    ifr.ifr_addr.sa_family = AF_INET;

    if (ioctl(fd, SIOCGIFADDR, (int8_t*) &ifr) < 0) {
        printf("failed to get ip address for device %s \n", device);
		close(fd);
		return(-1);
    }
    close(fd);
	memcpy(ip_addr, &sin->sin_addr, sizeof(struct in_addr)); 
    return(0);
}

void *
tc_worker_thread(void *t_ctx) {

    int rc = 0, i , j ;
    struct thread_context *tctx = (struct thread_context *)t_ctx;

    rc = pthread_mutex_lock(&create_thread_mutex);
    if (rc) {
        printf("%s : pthread_mutex_lock failed with rc = %d, errno = %d \n", __FUNCTION__, rc, errno);
        pthread_exit(&rc);
    }

    /* notify main thread to proceed creating other master thread */
    rc = pthread_cond_broadcast(&create_thread_cond);
    if (rc) {
        printf("%s :  pthread_cond_broadcast failed with rc = %d, errno = %d \n", __FUNCTION__, rc, errno);
        pthread_exit(&rc);
    }
    /* wait for main thread start notification */
    rc = pthread_cond_wait(&start_thread_cond, &create_thread_mutex);
    if (rc) {
        printf(" %s : pthread_cond_wait failed with rc = %d, errno = %d \n", __FUNCTION__, rc , errno);
        pthread_exit(&rc);
    }

    rc = pthread_mutex_unlock(&create_thread_mutex);
    if (rc) {
        printf("%s : pthread_mutex_unlock failed with rc = %d, errno = %d \n", __FUNCTION__, rc , errno);
        pthread_exit(&rc);
    }


    /***************************start helper thread****************************************/

    /* Initialize helper thread attributes */
    pthread_attr_init(&tctx->helper_attrs);
    pthread_attr_setdetachstate(&tctx->helper_attrs, PTHREAD_CREATE_JOINABLE);
    pthread_attr_setscope(&tctx->helper_attrs, PTHREAD_SCOPE_PROCESS);
    pthread_mutex_init(&tctx->helper_mutex, NULL);
    pthread_cond_init(&tctx->helper_cond, NULL);

    tctx->exit_helper = 0; /* false */
    tctx->rcvd_reply = 0;
    tctx->num_reply = 0;
    rc = pthread_mutex_lock(&tctx->helper_mutex);
    if(rc) {
        printf("%s : pthread_mutex_lock failed with rc = %d, errno = %d \n",  __FUNCTION__, rc , errno);
        pthread_exit(&rc);
    }
    /* create a receive thread to handle all receive */
    rc = pthread_create(&tctx->helper, &tctx->helper_attrs, tc_helper_thread, (void *)tctx);
    if (rc) {
        printf(" %s : pthread_create failed with rc = %d, errno = %d \n",  __FUNCTION__, rc , errno);
        pthread_exit(&rc);
    }
    rc = pthread_cond_wait(&tctx->helper_cond, &tctx->helper_mutex);
    if(rc) {
        pthread_cancel(tctx->helper);
        printf(" %s : pthread_cond_wait failed with rc = %d, errno = %d \n", __FUNCTION__, rc , errno);
        pthread_exit(&rc);
    }
    rc = pthread_mutex_unlock(&tctx->helper_mutex);
    if(rc) {
        pthread_cancel(tctx->helper);
        printf(" %s : pthread_mutex_unlock failed with rc = %d, errno = %d \n", __FUNCTION__, rc , errno);
        pthread_exit(&rc);
    }

    for(i = 0; i < NUM_RETRIES; i++) {
        for(j = 0; j < tctx->num_devices; j++) {
            if(strcmp(tctx->source.name, tctx->dev_list[j].name)) {
                rc = send_arp_packet(tctx->source, tctx->dev_list[j], MAX_COUNT);
                if(rc) {
                    printf("Error while sending Arp packet from source %s to dest %s \n", tctx->source.name, tctx->dev_list[j].name);
                    printf("Continuing .... \n");
                }
            }
        }
        if(tctx->rcvd_reply) {
            break;
        }
    }
    tctx->exit_helper = 1;
    pthread_exit(NULL);
}

void *
tc_helper_thread(void *t_context) {

    int rc = 0;
    struct thread_context *tctx = (struct thread_context *)t_context;
    int old_state, old_type;

    rc = pthread_mutex_lock(&tctx->helper_mutex);
    if (rc) {
        printf("%s: pthread_mutex_lock failed with rc = %d, errno = %d \n", rc, errno);
        pthread_exit(&rc);
    }

    /* notify main thread to proceed creating other master thread */
    rc = pthread_cond_broadcast(&tctx->helper_cond);
    if (rc) {
        printf(" %s: pthread_cond_broadcast failed with rc = %d, errno = %d \n", __FUNCTION__, rc, errno);
        pthread_exit(&rc);
    }

    rc = pthread_mutex_unlock(&tctx->helper_mutex);
    if (rc) {
        printf(" %s: pthread_mutex_unlock failed failed with rc = %d, errno = %d \n", __FUNCTION__, rc, errno);
        pthread_exit(&rc);
    }
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old_state);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old_type);
    /* creates a cancellation point in the calling thread */
    pthread_testcancel();

    rc = recv_arp_reply(tctx);
    if(rc) {
        printf("Error recv packets on interface %s, errno = %d \n", &tctx->source.name, errno);
    }

    pthread_exit(&rc);
}

int
send_arp_packet(struct dev_details source, struct dev_details dest, u_int count) {

    char hwaddr[6], bcast_addr[6];
    u_int  size;
    xmit buf;
    int s;
    int last;
    int rc;

	#ifndef __HTX_LINUX__
		struct sockaddr_ndd_8022 sa;
	#else 
		struct sockaddr_ll sa;
	#endif 

    memcpy(hwaddr, &source.mac_addr[0], 6);
#ifdef __HTX_LINUX__ 
	memcpy(bcast_addr, dest.mac_addr, 6); 
#else 
    hwaddr_aton("FF:FF:FF:FF:FF:FF", bcast_addr);
#endif
	count = NUM_RETRIES;
	/* Building Ethernet Header */ 
	memcpy(buf.dst, bcast_addr, sizeof(buf.dst));
	memcpy(buf.src, hwaddr, sizeof(buf.src));
	bcopy(hwaddr, buf.src, sizeof(buf.src));
	buf.ethertype = ETHERTYPE_ARP; 

    /* ARP Header */
 	buf.arp_pkt.ea_hdr.ar_hrd = ARPHRD_ETHER;
    buf.arp_pkt.ea_hdr.ar_pro = SNAP_TYPE_IP;
    buf.arp_pkt.ea_hdr.ar_hln = 6;
    buf.arp_pkt.ea_hdr.ar_pln = 4;
    buf.arp_pkt.ea_hdr.ar_op = ARPOP_REQUEST;

    /* Grat ARP Packet */
    bcopy(hwaddr, buf.arp_pkt.arp_sha, sizeof(buf.arp_pkt.arp_sha));
    bcopy(bcast_addr, buf.arp_pkt.arp_tha, sizeof(buf.arp_pkt.arp_tha));

	#ifdef __HTX_LINUX__	
	s = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	#else 
	s = socket(AF_NDD, SOCK_DGRAM, 0);
	#endif 
	if (s < 0) {
        printf("Unable to open socket s = %d, errno = %d \n", s, errno);
        return(s);
    }
	 
	/* Now Connect */ 
	#ifndef __HTX_LINUX__
	memset(&sa, 0, sizeof (sa));
    sa.sndd_8022_family = AF_NDD;
    sa.sndd_8022_len = sizeof(sa);
    sa.sndd_8022_filtertype = NS_ETHERTYPE;
    sa.sndd_8022_ethertype = ETHERTYPE_ARP;
    sa.sndd_8022_filterlen = sizeof(struct ns_8022);
	memcpy(sa.sndd_8022_nddname, &source.name, sizeof(sa.sndd_8022_nddname));
    if ((rc = connect(s, (struct sockaddr *)&sa, sizeof(sa))) < 0) {
        printf("failed to connect socket with source,rc = %d, errno = %s \n", rc, STRERROR(errno));
        return(errno);
    }
	#else 
	sa.sll_family    = AF_PACKET;
	sa.sll_ifindex   = get_iface_index(source.name); 
	if (sa.sll_ifindex == -1) { 
		printf("Unable to get device index, device = %s \n", source.name);
        return (-1);
    }
    sa.sll_protocol  = htons(ETH_P_ALL);
	if((rc = bind(s, (struct sockaddr *)&sa, sizeof(sa))) < 0) { 
		printf("failed to bind socket with source,rc = %d, errno = %s \n", rc, STRERROR(errno));
		return(-1); 
	}
	struct packet_mreq pkt_mreq;	
   	memset(&pkt_mreq, 0, sizeof(struct packet_mreq));
   	pkt_mreq.mr_ifindex = get_iface_index(source.name);
   	pkt_mreq.mr_type = PACKET_MR_PROMISC;
   	if (setsockopt(s, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
   					&pkt_mreq, sizeof(struct packet_mreq))) {
   		printf("Failed to set adapter %s in PROMISC mode \n", source.name );
   		return -1;
   }
	#endif	

    
#if 0
	#ifndef __HTX_LINUX__
    printf("%s:%s ", sa.sndd_8022_nddname, inet_ntoa(*(struct in_addr *)buf.arp_pkt.arp_spa));
    #else 
	printf("%s:%s ", source.name, inet_ntoa(*(struct in_addr *)buf.arp_pkt.arp_spa));
	#endif
    printf("----------> ");
    printf("%s:",dest.name);
    printf("%s ", inet_ntoa(*(struct in_addr *)buf.arp_pkt.arp_tpa));
    printf("\n");
#endif
    while (count-- > 0) {
		#ifndef __HTX_LINUX__
        if (((rc = send(s, &buf, sizeof(buf), 0)) < 0) && (errno != ENOBUFS) ) {
		#else 
		if(((rc = write(s, &buf, sizeof(buf))) != sizeof(buf)) && (errno != ENOBUFS) ) { 
		#endif
            printf("Failed to send a packet, rc = %d, device = %s, err = %s \n", rc, source.name, STRERROR(errno));
            return(-1);
        }
        usleep(5);
    }
    close(s);


	return(0); 
}

int
get_iface_index(const int8_t *device) { 

    int sock;
	struct ifreq ifr;
	/* Open a generic IP socket for querying the stack */
	sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sock < 0) {
    	printf("Unable to open socket sock = %d, errno = %d \n", sock, errno);
    	return(-1);
     }
	/* Get interface index */
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_addr.sa_family = AF_PACKET;
    strncpy (ifr.ifr_name, device, sizeof(ifr.ifr_name) - 1);
    ifr.ifr_name[sizeof(ifr.ifr_name)-1] = '\0';
    if (ioctl(sock, SIOCGIFINDEX, &ifr) == -1) { 
		close(sock);
        return (-1);
    }
	close(sock);
    return ifr.ifr_ifindex;
}

int
recv_arp_reply(struct thread_context * tctx) {

    char device[10];
    xmit buf;
    int s;
    int cc, j;
    char rcvd_src[6];
    char rcvd_dest[6];
	#ifndef __HTX_LINUX__
    	struct sockaddr_ndd_8022 sa;
	#else 
		struct sockaddr_ll sa;
	#endif 

    strcpy(device, &tctx->source.name);

	memset(&sa, 0, sizeof (sa));
	#ifndef __HTX_LINUX__	
   	 	sa.sndd_8022_family = AF_NDD;
    	sa.sndd_8022_len = sizeof(sa);
    	sa.sndd_8022_filtertype = NS_TAP;
    	sa.sndd_8022_ethertype = (u_short)0x0806;
    	sa.sndd_8022_filterlen = sizeof(struct ns_8022);
    	bcopy(device, sa.sndd_8022_nddname, sizeof(sa.sndd_8022_nddname));
	#else 
		sa.sll_family    = PF_PACKET;
		sa.sll_ifindex   = get_iface_index(device);
		if (sa.sll_ifindex == -1) {
    	    printf("Unable to get device index, device = %s \n", device);
            return (-1);
        }
        sa.sll_protocol  = htons(ETH_P_ALL);
	#endif 
	
	#ifndef __HTX_LINUX__
	s = socket(AF_NDD, SOCK_DGRAM, NDD_PROT_ETHER);
	#else
	s = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	#endif 
    if (s < 0) {
        printf("%s : error opening socket errno = %d \n", __FUNCTION__, errno);
        return(s);
    }
    if (bind(s, (struct sockaddr *)&sa, sizeof(sa))) {
        printf("%s error binding to socket errno = %d \n ",__FUNCTION__, errno);
        return(errno);
    }
	#ifndef __HTX_LINUX__
    if (connect(s, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        printf("%s error connecting to socket errno = %d \n ",__FUNCTION__, errno);
        return(errno);
    }
	#else 
	struct packet_mreq pkt_mreq;
    memset(&pkt_mreq, 0, sizeof(struct packet_mreq));
    pkt_mreq.mr_ifindex = get_iface_index(device);
    pkt_mreq.mr_type = PACKET_MR_PROMISC;
    if (setsockopt(s, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
                   &pkt_mreq, sizeof(struct packet_mreq))) {
    	printf("Cannot set adapter %s in Promisc mode \n",device );
      	return(errno); 
    }
	#endif 

    do {
        if ((cc = read(s, &buf, sizeof(buf))) < 0) {
            printf("read failed with rc = %d, errno = %d \n", cc, errno);
            return(cc);
        }
        if(cc) {
			if(buf.arp_pkt.ea_hdr.ar_hrd != ARPHRD_ETHER)  
				continue; 
			/* 
			 * print_rcv_buffer(buf, tctx); 
			 */ 
            memcpy(rcvd_src, buf.src, (sizeof(char)*6));
            memcpy(rcvd_dest, buf.dst, (sizeof(char)*6));
            /* Check whether my worker thread is the source for this packet */
            if(!memcmp(rcvd_dest, &tctx->source.mac_addr, (sizeof(char)*6))) {
                /* Now copy the receivers details */
                for(j = 0; j < tctx->num_devices; j++) {
                    if(!memcmp(rcvd_src, &tctx->dev_list[j].mac_addr, (sizeof(char)*6))) {
                        memcpy(&tctx->dest[tctx->num_reply], &tctx->dev_list[j], sizeof(struct dev_details));
                        tctx->rcvd_reply = 1;
                        tctx->num_reply++;
                        break;
                    }
                }
            }
        }
    } while(cc > 0 && !tctx->exit_helper);

    close(s);
    return(0);
} 

void
print_rcv_buffer(xmit buf, struct thread_context tctx) { 

	printf("src %s <---- ", tctx.source.name);
	pit("dest=",buf.dst, 6); 
	printf(", "); 
	pit("src=",buf.src, 6);
	printf(", type=%#2x \n", buf.ethertype); 
} 
