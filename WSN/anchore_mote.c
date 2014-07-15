/*
 * Developed by Mehdi Mohammadi
 * Western Michigan University
 * Computer Science Department 
 * July 15, 2014
 */
 
#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "net/uip.h"
#include "net/uip-ds6.h"
#include "net/uip-udp-packet.h"
#include "net/uip-debug.h"
#include "sys/etimer.h"
#include "node-id.h"

#include "simple-udp.h"
#include "servreg-hack.h"
#include "anchor.h"
#include <stdio.h>
#include <string.h>

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

#define UDP_EXAMPLE_ID  190

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"

#ifndef PERIOD
#define PERIOD 60
#endif

#define START_INTERVAL		(15 * CLOCK_SECOND)
#define SEND_INTERVAL		(PERIOD * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))
#define MAX_PAYLOAD_LEN		40

#define UDP_PORT 1234
#define SERVICE_ID 190

static struct simple_udp_connection unicast_connection;

static struct uip_udp_conn *client_conn;
static uip_ipaddr_t server_ipaddr;

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "Anchor Routing Process");
PROCESS(unicast_sender_process, "Unicast Measurement Process");
AUTOSTART_PROCESSES(&udp_client_process, &unicast_sender_process);
/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{
  char *str;

  if(uip_newdata()) {
    str = uip_appdata;
    str[uip_datalen()] = '\0';
    printf("DATA recv '%s'\n", str);
  }
}
/*---------------------------------------------------------------------------*/
static void
send_packet(void *ptr)
{
  static int seq_id;
  char buf[MAX_PAYLOAD_LEN];

  seq_id++;
clock_time_t start_time = clock_time();
  PRINTF("DATA send to %d 'Msg %d'\n",
         server_ipaddr.u8[sizeof(server_ipaddr.u8) - 1], seq_id);

  sprintf(buf, "%lu", (unsigned long)start_time);
  uip_udp_packet_sendto(client_conn, buf, strlen(buf),
                        &server_ipaddr, UIP_HTONS(UDP_SERVER_PORT));
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
   uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 1);
#elif 1
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
  clock_time_t recv_time = clock_time();

  struct broadcast_message *msg;
  msg = (struct broadcast_message *)data;
  printf("Anchor: Data received with length %d : st %lu\n",
         datalen, msg->sent_time );


  unsigned long ll = (unsigned long)recv_time;
  msg->recv_time = ll;
  msg->anchorid = node_id;
  simple_udp_sendto(&unicast_connection, msg, sizeof(struct broadcast_message), receiver_addr);
  printf("Anchor: Msg Back to %d: %lu\n", msg->nodeid, msg->recv_time);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(unicast_sender_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer send_timer;
  uip_ipaddr_t *addr;

  PROCESS_BEGIN();

  set_global_address();

  simple_udp_register(&unicast_connection, UDP_PORT,
                      NULL, UDP_PORT, receiver);

  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
    etimer_set(&send_timer, SEND_TIME);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
    addr = NULL;
    if(addr != NULL) {
      static unsigned int message_number;
      char buf[20];

      printf("Sending unicast to ");
      uip_debug_ipaddr_print(addr);
      printf("\n");
      sprintf(buf, "Message %d", message_number);
      message_number++;
      simple_udp_sendto(&unicast_connection, buf, strlen(buf) + 1, addr);
    } 
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
