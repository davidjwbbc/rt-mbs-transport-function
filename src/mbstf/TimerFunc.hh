#ifndef _MBS_TF_TIMER_FUNC_HH_
#define _MBS_TF_TIMER_FUNC_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Timer function interface
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

class TimerFunc {
public:
    TimerFunc();
    TimerFunc(TimerFunc &&other) = delete;
    TimerFunc(const TimerFunc &other) = delete;
    TimerFunc &operator=(TimerFunc &&other) = delete;
    TimerFunc &operator=(const TimerFunc &other) = delete;
    virtual ~TimerFunc();

    virtual void trigger() = 0;

private:
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_TIMER_FUNC_HH_ */
