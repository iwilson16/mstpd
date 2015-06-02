/*
 * switch_deps.c    Driver-specific code for switch backend (OpenWRT).
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 * Authors: Alexandru Ardelean <ardeleanalex@gmail.com>
 */

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <asm/byteorder.h>
#include <netinet/in.h>
#include <linux/if_bridge.h>

#include "log.h"
#include "mstp.h"

#include <libxsw.h>
static int mstp_refcount = 0;

static const enum STP_PORT_STATE br_to_switch_port_states[] =
{
    [BR_STATE_DISABLED]   = STP_PORT_STATE_DISABLED,
    [BR_STATE_LISTENING]  = STP_PORT_STATE_LISTENING,
    [BR_STATE_LEARNING]   = STP_PORT_STATE_LEARNING,
    [BR_STATE_FORWARDING] = STP_PORT_STATE_FOWARDING,
    [BR_STATE_BLOCKING]   = STP_PORT_STATE_BLOCKING,
};

static const int switch_to_br_port_states[] = 
{
    [STP_PORT_STATE_NO_STP]    = -1,
    [STP_PORT_STATE_DISABLED]  = BR_STATE_DISABLED,
    [STP_PORT_STATE_BLOCKING]  = BR_STATE_BLOCKING,
    [STP_PORT_STATE_LISTENING] = BR_STATE_LISTENING,
    [STP_PORT_STATE_LEARNING]  = BR_STATE_LEARNING,
    [STP_PORT_STATE_FOWARDING] = BR_STATE_FORWARDING,
};

/*------------------------------------------------------------------*/
/* stubs and wrappers to use the existing code for Linux bridges    */
/* without actually using them                                      */
/*------------------------------------------------------------------*/
int init_bridge_ops(void)
{
    return 0;
}

int netsock_init(void)
{
    return 0;
}

int get_hwaddr(char *ifname, __u8 *hwaddr)
{
    return switch_get_hw_address(ifname, hwaddr);
}

int get_flags(char *ifname)
{
    return switch_get_if_flags(ifname);
}

int get_bridge_portno(char *if_name)
{
    return switch_get_port_index(if_name);
}

int if_shutdown(char *ifname)
{
    return switch_set_port_enabled(ifname, false);
}

int ethtool_get_speed_duplex(char *ifname, int *speed, int *duplex)
{
    return switch_get_port_link_info(ifname, speed, duplex, NULL);
}

char *index_to_name(int index, char *name)
{
    return switch_index_to_name(index, name);
}

char *index_to_port_name(int index, char *name)
{
    return switch_index_to_port_name(index, name);
}

/*------------------------------------------------------------------*/
/* mstpd driver deps functions                                      */
/*------------------------------------------------------------------*/

/*
 * Set new state (BR_STATE_xxx) for the given port and MSTI.
 * Return new actual state (BR_STATE_xxx) from driver.
 */

int driver_mstp_init()
{
    return 0;
}

void driver_mstp_fini()
{
}

bool driver_create_bridge(bridge_t *br, __u8 *macaddr)
{
    return (switch_get_stp_enabled(br->sysdeps.name) > 0);
}

bool driver_create_port(port_t *prt, __u16 portno)
{
    return true;
}

void driver_delete_bridge(bridge_t *br)
{
}

/* Driver hook that is called when a port is deleted */
void driver_delete_port(port_t *prt)
{

}

int driver_set_new_state(per_tree_port_t *ptp, int new_state)
{
    const char *swname = ptp->port->bridge->sysdeps.name;
    const char *swport_name = ptp->port->sysdeps.name;
    int rc = switch_set_stp_port_state(swname, swport_name,
                br_to_switch_port_states[new_state]);
    if (rc) {
        new_state = switch_get_stp_port_state(swname, swport_name);
        if (new_state > -1 && new_state <= STP_PORT_STATE_FOWARDING)
            new_state = switch_to_br_port_states[new_state];
    }

    return new_state;
}

bool driver_create_msti(bridge_t *br, __u16 mstid)
{
    /* TODO: send "create msti" command to driver */
    return true;
}

bool driver_delete_msti(bridge_t *br, __u16 mstid)
{
    /* TODO: send "delete msti" command to driver */
    return true;
}

void driver_flush_all_fids(per_tree_port_t *ptp)
{
    /* TODO: do real flushing.
     * Make it asynchronous, with completion function calling
     * MSTP_IN_all_fids_flushed(ptp)
     */
    MSTP_IN_all_fids_flushed(ptp);
}

/*
 * Set new ageing time (in seconds) for the port.
 * Return new actual ageing time from driver (the ageing timer granularity
 *  in the hardware can be more than 1 sec)
 */
unsigned int driver_set_ageing_time(port_t *prt, unsigned int ageingTime)
{
    /* TODO: do set new ageing time */
    return ageingTime;
}
