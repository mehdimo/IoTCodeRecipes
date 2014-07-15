/*
 * Developed by Mehdi Mohammadi
 * Western Michigan University
 * Computer Science Department 
 * July 15, 2014
 */

#include "contiki.h"
#include "net/rime.h"
#include "random.h"
#include "lib/memb.h"
#include "node-id.h"
#include "lib/list.h"
#include "net/rime.h"
#include <stdio.h>

struct service {
  struct service *next;
  rimeaddr_t service_provider;
  char service_name[15];
};

struct broadcast_message {
  char service_name[15];
  uint8_t seqno;
};

#define MAX_SERVICES 32

MEMB(services_memb, struct service, MAX_SERVICES);

LIST(services_list);

static struct broadcast_conn broadcast;
static struct unicast_conn unicast;

/*---------------------------------------------------------------------------*/
PROCESS(service_advertisement_process, "Service Advertisment for Internet of Things");
PROCESS(service_request_process, "Service Request for Internet of Things");
/*----------------------------------------------------------------------------*/
AUTOSTART_PROCESSES(&service_advertisement_process, &service_request_process);

/*---Broadcast functions ------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
  struct broadcast_message *m;
  struct service *e;

  m = (struct broadcast_message *)packetbuf_dataptr();

  /* An service advertisement is received, If we have a record for it we update it, otherwise we should add it to the list */
  for(e = list_head(services_list); e != NULL; e = e->next) {
    if(rimeaddr_cmp(from, &e->service_provider)) {
		break;
    }
  }

  /* The service provider was not found in the list, so we add a new entry by
     allocating memory and fill in the necessary fields, and add it to the list. */
  if(e==NULL)
  {
     e = memb_alloc(&services_memb);
     if(e == NULL)
      return;
	  
     rimeaddr_copy(&e->service_provider, from);
     strcpy(e->service_name, m->service_name);
     list_add(services_list, e);
  }
  printf("Service advertisement received from %d.%d: '%s'\n",
	from->u8[0], from->u8[1], m->service_name);
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};

/***** Unicast function *****************************************/
static void
rec_req_srv(struct unicast_conn *c, const rimeaddr_t *from)
{
    printf("received service request from %d\n", from->u8[0]);
}

static const struct unicast_callbacks unicast_callbacks = {rec_req_srv};

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(service_advertisement_process, ev, data)
{
  static struct etimer et;
  struct broadcast_message msg;
  static char m[15];
  
  int service_types = 3;

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  list_init(services_list);

  broadcast_open(&broadcast, 129, &broadcast_call);

  while(1) {

    /* Delay 2-4 seconds */
    etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    
    int nid = node_id;
    nid = nid%service_types;
    if(nid==0) 
       nid = service_types;
    char m[15] = "Service-";

    char dg[5];
    sprintf(dg, "%d", nid);
    strcat(m, dg);

    strcpy(msg.service_name, m);

    packetbuf_copyfrom(&msg, sizeof(struct broadcast_message));
    broadcast_send(&broadcast);
    printf("advertisement sent for %s\n", msg.service_name);
  }

  PROCESS_END();
}
/*------------------ Service Request---------------------------------------*/
PROCESS_THREAD(service_request_process, ev, data)
{
  char* req_service_name = "Service-2";

  PROCESS_EXITHANDLER(unicast_close(&unicast);)
    
  PROCESS_BEGIN();

  unicast_open(&unicast, 146, &unicast_callbacks);

  while(1) {
    static struct etimer et;
    struct service *n;

    etimer_set(&et, CLOCK_SECOND * 8 + random_rand() % (CLOCK_SECOND * 8));
    
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    if(list_length(services_list) > 0) 
    {   
     struct service *selected_service;
     for(n = list_head(services_list); n != NULL; n = n->next) 
     {
        int cmp = strcmp(req_service_name, n->service_name);
        if(cmp == 0) {
			selected_service = n;
			printf("examined node %d (%s)\n", n->service_provider.u8[0], n->service_name);          
        }
     }
     if(selected_service != NULL)
     {
         printf("Sent request for (%s) to %d.%d\n", selected_service->service_name, selected_service->service_provider.u8[0], selected_service->service_provider.u8[1]);
         unicast_send(&unicast, &selected_service->service_provider);
     }
    } // end if
    else 
      printf("Service list is Empty.\n");
  }
  PROCESS_END();
}
