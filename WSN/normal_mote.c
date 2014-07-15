/*
 * Developed by Mehdi Mohammadi
 * Western Michigan University
 * Computer Science Department 
 * July 15, 2014
 */
 
#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/rime.h"
#include "net/uip.h"
#include "net/uip-ds6.h"
#include "net/uip-udp-packet.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "node-id.h"
#include "anchor.h"

#include <stdio.h>
#include <string.h>

#include "simple-udp.h"

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"

#ifndef PERIOD
#define PERIOD 10
#endif

#define START_INTERVAL		(15 * CLOCK_SECOND)
#define SEND_INTERVAL		(PERIOD * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))
#define MAX_PAYLOAD_LEN		40

#define UDP_PORT 1234

static struct simple_udp_connection broadcast_connection;
static struct uip_udp_conn *client_conn;
static uip_ipaddr_t server_ipaddr;

static unsigned long min_dist = 0xFFFF;
static int last_anchor;
static int closest_anchor;

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "Normal Mote process");
PROCESS(broadcast_example_process, "UDP broadcast process");
AUTOSTART_PROCESSES(&udp_client_process, &broadcast_example_process);
/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{
  char *str;

  if(uip_newdata()) {
    str = uip_appdata;
    str[uip_datalen()] = '\0';
    printf("DATA recv '%lu'\n", str);
  }
}
/*---------------------------------------------------------------------------*/
static void
send_packet(void *ptr)
{
  char buf[MAX_PAYLOAD_LEN];
  struct anchor *anc;

  if(closest_anchor != last_anchor)
{
  anc = malloc(sizeof(struct anchor));
  anc->node = node_id;
  anc->anchor = closest_anchor;

  PRINTF("DATA send to %d \n",
         server_ipaddr.u8[sizeof(server_ipaddr.u8) - 1]);

  uip_udp_packet_sendto(client_conn, anc, sizeof(struct anchor),
                        &server_ipaddr, UIP_HTONS(UDP_SERVER_PORT));
  }
}
/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTF("Client IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n");
      /* hack to make address "final" */
      if (state == ADDR_TENTATIVE) {
	uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
set_global_address(void)
{
  uip_ipaddr_t ipaddr;

  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

#if 0
/* Mode 1 - 64 bits inline */
   uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 1);
#elif 1
/* Mode 2 - 16 bits inline */
  uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0, 0x00ff, 0xfe00, 1);
#else
/* Mode 3 - derived from server link-local (MAC) address */
  uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0x0250, 0xc2ff, 0xfea8, 0xcd1a); //redbee-econotag
#endif
}
/*---------------------------------------------------------------------------*/
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
struct broadcast_message *msg;
  msg = (struct broadcast_message *)data;

  int nid = msg->nodeid;
  if(nid == node_id)
  {       
       printf("Data received from %d %lu - %lu\n",
         msg->anchorid, msg->sent_time, msg->recv_time);
       unsigned long diff = msg->recv_time - msg->sent_time;
       if(diff < min_dist)
	{
	  min_dist = diff;	 
	  last_anchor = closest_anchor; 
	  closest_anchor = msg->anchorid;	  
	}
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(broadcast_example_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer send_timer;
  
  struct broadcast_message *msg;
  msg = malloc(sizeof(struct broadcast_message));
  uip_ipaddr_t addr;

  PROCESS_BEGIN();

  simple_udp_register(&broadcast_connection, UDP_PORT,
                      NULL, UDP_PORT,
                      receiver);

  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
    etimer_set(&send_timer, SEND_TIME);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
    
    clock_time_t start_time = clock_time();
    unsigned long ll = (unsigned long)start_time;
    msg->sent_time = ll;
    msg->nodeid = node_id;
    
    printf("Sending broadcast time=%lu\n", (unsigned long)msg->sent_time);
    uip_create_linklocal_allnodes_mcast(&addr);
    simple_udp_sendto(&broadcast_connection, msg, sizeof(struct broadcast_message), &addr);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic;
  static struct ctimer backoff_timer;

  PROCESS_BEGIN();

  PROCESS_PAUSE();

  set_global_address();
  
  PRINTF("UDP client process started\n");

  print_local_addresses();

  /* new connection with remote host */
  client_conn = udp_new(NULL, UIP_HTONS(UDP_SERVER_PORT), NULL); 
  if(client_conn == NULL) {
    PRINTF("No UDP connection available, exiting the process!\n");
    PROCESS_EXIT();
  }
  udp_bind(client_conn, UIP_HTONS(UDP_CLIENT_PORT)); 

  PRINTF("Created a connection with the server ");
  PRINT6ADDR(&client_conn->ripaddr);
  PRINTF(" local/remote port %u/%u\n",
	UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));

  etimer_set(&periodic, SEND_INTERVAL);
  while(1) {
    PROCESS_YIELD();
    if(ev == tcpip_event) {
      tcpip_handler();
    }
    
    if(etimer_expired(&periodic)) {
      etimer_reset(&periodic);
      ctimer_set(&backoff_timer, SEND_TIME, send_packet, NULL);
    }
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
