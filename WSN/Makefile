all: normal_mote anchor_mote sink_mote
APPS=servreg-hack
CONTIKI=/home/user/contiki-2.7

WITH_UIP6=1
UIP_CONF_IPV6=1

CFLAGS+= -DUIP_CONF_IPV6_RPL

ifdef SERVER_REPLY
CFLAGS+=-DSERVER_REPLY=$(SERVER_REPLY)
endif
ifdef PERIOD
CFLAGS+=-DPERIOD=$(PERIOD)
endif

include $(CONTIKI)/Makefile.include
