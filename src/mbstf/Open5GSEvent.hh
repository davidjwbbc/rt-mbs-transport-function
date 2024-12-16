#ifndef _MBS_TF_OPEN5GS_EVENT_HH_
#define _MBS_TF_OPEN5GS_EVENT_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS SBI Server interface
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

#include <memory>

#include "common.hh"

MBSTF_NAMESPACE_START

class Open5GSEvent {
public:
    Open5GSEvent(ogs_event_t *event) :m_event(event) {};
    Open5GSEvent() = delete;
    Open5GSEvent(Open5GSEvent &&other) = delete;
    Open5GSEvent(const Open5GSEvent &other) = delete;
    Open5GSEvent &operator=(Open5GSEvent &&other) = delete;
    Open5GSEvent &operator=(const Open5GSEvent &other) = delete;
    virtual ~Open5GSEvent() {};

    ogs_event_t *ogsEvent() { return m_event; };
    const ogs_event_t *ogsEvent() const { return m_event; };

private:
    ogs_event_t *m_event;
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_OPEN5GS_EVENT_HH_ */
