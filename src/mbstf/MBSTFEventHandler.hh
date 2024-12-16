#ifndef _MBS_TF_MBSTF_EVENT_HANDLER_HH_
#define _MBS_TF_MBSTF_EVENT_HANDLER_HH_
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
#include "EventHandler.hh"
#include "Open5GSEvent.hh"
#include "Open5GSFSM.hh"

MBSTF_NAMESPACE_START

class MBSTFEventHandler : public EventHandler {
public:
    MBSTFEventHandler() :EventHandler() {};
    MBSTFEventHandler(MBSTFEventHandler &&other) = delete;
    MBSTFEventHandler(const MBSTFEventHandler &other) = delete;
    MBSTFEventHandler &operator=(MBSTFEventHandler &&other) = delete;
    MBSTFEventHandler &operator=(const MBSTFEventHandler &other) = delete;
    virtual ~MBSTFEventHandler() = default;

    virtual void dispatch(Open5GSFSM &fsm, Open5GSEvent &event);
    
private:
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_MBSTF_EVENT_HANDLER_HH_ */
