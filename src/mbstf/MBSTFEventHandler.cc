/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSTF EventHandler
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include "ogs-proto.h"

#include "common.hh"
#include "Open5GSEvent.hh"
#include "Open5GSFSM.hh"

#include "MBSTFEventHandler.hh"

MBSTF_NAMESPACE_START

void MBSTFEventHandler::dispatch(Open5GSFSM &fsm, Open5GSEvent &event)
{
    // Handle Open5GS FSM events here
    ogs_debug("MBSTF Event: %s", ogs_event_get_name(event.ogsEvent()));
}
    
MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
