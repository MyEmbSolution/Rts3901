#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <pthread.h>
#include "common.h"
#include "rtl.h"


/*
 * fill content to data payload
 * @param data: payload pointer
 * @param len: payload length
 */
void fill_data_content(u8 *data, int len)
{
#ifdef RANDOM_DATA
        int i;

        /* random data */
        srand(time(NULL));
        for (i = 0; i < len; i++) {
                data[i] = (u8)((int)(255.0*rand()/RAND_MAX));
        }
#else
        static u8 pattern = 0x5A;
        /* pattern */
        memset(data, pattern++, len);
#endif
}

void fill_waveform_test_data_content(PMP_ADAPTER Adapter, u8 *data, int len)
{
        if( Adapter->wave_form_test_mode == 0 ) {
                /* pattern */
                memset(data, 0xFF, len);
        } else if (Adapter->wave_form_test_mode == 1) {
                /* pattern */
                memset(data, 0, len);
	 } else if (Adapter->wave_form_test_mode == 2) {
                int i;

                /* random data */
                srand(time(NULL));
                for (i = 0; i < len; i++) {
                        data[i] = (u8)i;
                }
        } else {
#ifdef RANDOM_DATA
                int i;

                /* random data */
                srand(time(NULL));
                for (i = 0; i < len; i++) {
                        data[i] = (u8)((int)(255.0*rand()/RAND_MAX));
                }
#else
                static u8 pattern = 0x5A;
                /* pattern */
                memset(data, pattern++, len);
#endif
        }
}

void* send_waveform_test_packet_thread(void *arg)
{
        PMP_ADAPTER Adapter = (PMP_ADAPTER)arg;
        struct sockaddr_ll socket_address;
        struct ethhdr *eh;
        void* send_buffer = NULL;
        u8 dest_mac[ETH_MAC_LEN] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
        u8* etherhead;
        u8* data; /* payload */
        int send_skfd = -1;
        int pktsize;

        rt_nic_info *nicinfo;

        nicinfo = Adapter->testnicinfo;

        if ((send_skfd = create_raw_socket(ETH_P_LOOPBACK)) < 0)
                return NULL;

        send_buffer = (void*)malloc(MAX_ETH_FRAME_LEN);
        if (send_buffer == NULL) {
                goto loopback_test_exit;
        }

        /*prepare sockaddr_ll*/
        socket_address.sll_family   = PF_PACKET;
        socket_address.sll_protocol = htons(ETH_P_LOOPBACK);
        socket_address.sll_ifindex  = nicinfo->ifindex;
        socket_address.sll_hatype   = ARPHRD_ETHER;
        socket_address.sll_pkttype  = PACKET_OTHERHOST;
        socket_address.sll_halen    = ETH_ALEN;

        memset(socket_address.sll_addr, 0, sizeof(socket_address.sll_addr));
        memcpy(socket_address.sll_addr, dest_mac, sizeof(dest_mac));
        etherhead = send_buffer;
        data = send_buffer + ETH_HEADER_LEN;
        eh = (struct ethhdr *)etherhead;

        do {
                pktsize = MAX_ETH_FRAME_LEN;

                /* fill ethernet header */
                memset(send_buffer, 0, sizeof(MAX_ETH_FRAME_LEN));
                memcpy((void*)send_buffer, (void*)dest_mac, ETH_MAC_LEN);
                memcpy((void*)(send_buffer+ETH_MAC_LEN),
                       (void*)nicinfo->macaddr, ETH_MAC_LEN);
                eh->h_proto = htons(ETH_P_LOOPBACK);

                fill_waveform_test_data_content(Adapter, data, pktsize-ETH_HEADER_LEN);

                /*send packet*/
                if (sendto(send_skfd, send_buffer, pktsize, 0,
                           (struct sockaddr*)&socket_address,
                           sizeof(socket_address)) < 0 ) {

                        dbgperror(MPL_ERR, "sendto()");
                        continue;
                }

                //usleep(100);

                fflush(stdout);
        } while(Adapter->kee_send_packet == TRUE);

loopback_test_exit:
        if (send_buffer != NULL)
                free(send_buffer);

        send_buffer = NULL;
        close(send_skfd);
        return NULL;
}

void send_waveform_test_packet(PMP_ADAPTER Adapter)
{
        pthread_t tid;
        int err;

        Adapter->kee_send_packet = TRUE;

        err = pthread_create(&(tid), NULL, &send_waveform_test_packet_thread, (void*)Adapter);
        if (err != 0)
                printf("\ncan't create thread :[%s]", strerror(err));
}


