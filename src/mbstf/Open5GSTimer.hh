#ifndef _MBS_TF_OPEN5GS_TIMER_HH_
#define _MBS_TF_OPEN5GS_TIMER_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS ogs_timer_t wrapper
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

class Open5GSTimer {
public:
    Open5GSTimer(ogs_timer_t *timer) :m_ogsTimer(timer) {};
    Open5GSTimer() = delete;
    Open5GSTimer(Open5GSTimer &&other) = delete;
    Open5GSTimer(const Open5GSTimer &other) = delete;
    Open5GSTimer &operator=(Open5GSTimer &&other) = delete;
    Open5GSTimer &operator=(const Open5GSTimer &other) = delete;
    virtual ~Open5GSTimer() {};

    ogs_timer_t *ogsTimer() { return m_ogsTimer; };
    const ogs_timer_t *ogsTimer() const { return m_ogsTimer; };

private:
    ogs_timer_t *m_ogsTimer;
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_OPEN5GS_TIMER_HH_ */
