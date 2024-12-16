#ifndef _MBS_TF_EVENT_HANDLER_HH_
#define _MBS_TF_EVENT_HANDLER_HH_
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

#include "common.hh"
#include "Open5GSEvent.hh"
#include "Open5GSFSM.hh"

MBSTF_NAMESPACE_START

class EventHandler {
public:
    EventHandler() = default;
    EventHandler(EventHandler &&other) = delete;
    EventHandler(const EventHandler &other) = delete;
    EventHandler &operator=(EventHandler &&other) = delete;
    EventHandler &operator=(const EventHandler &other) = delete;
    virtual ~EventHandler() = default;

    virtual void dispatch(Open5GSFSM &fsm, Open5GSEvent &event) = 0;
    
private:
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_EVENT_HANDLER_HH_ */
