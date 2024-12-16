#ifndef _MBS_TF_OPEN5GS_FSM_HH_
#define _MBS_TF_OPEN5GS_FSM_HH_
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

class Open5GSFSM {
public:
    Open5GSFSM(ogs_fsm_t *fsm) :m_ogsFsm(fsm) {};
    Open5GSFSM() = delete;
    Open5GSFSM(Open5GSFSM &&other) = delete;
    Open5GSFSM(const Open5GSFSM &other) = delete;
    Open5GSFSM &operator=(Open5GSFSM &&other) = delete;
    Open5GSFSM &operator=(const Open5GSFSM &other) = delete;
    virtual ~Open5GSFSM() {};

private:
    ogs_fsm_t *m_ogsFsm;
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_OPEN5GS_FSM_HH_ */
