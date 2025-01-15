#ifndef _MBS_TF_OPEN5GS_SBI_SOCKADDR_HH_
#define _MBS_TF_OPEN5GS_SBI_SOCKADDR_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS SBI SockAddr interface
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
#include "ogs-core.h"

#include <memory>

#include "common.hh"

MBSTF_NAMESPACE_START

class Open5GSSockAddr {
public:
    Open5GSSockAddr(ogs_sockaddr_t *sockaddr) :m_sockaddr(sockaddr) {};
    Open5GSSockAddr() = delete;
    Open5GSSockAddr(Open5GSSockAddr &&other) = delete;
    Open5GSSockAddr(const Open5GSSockAddr &other) = delete;
    Open5GSSockAddr &operator=(Open5GSSockAddr &&other) = delete;
    Open5GSSockAddr &operator=(const Open5GSSockAddr &other) = delete;
    virtual ~Open5GSSockAddr() {};

    ogs_sockaddr_t *ogsSockAddr() { return m_sockaddr; };
    const ogs_sockaddr_t *ogsSockAddr() const { return m_sockaddr; };

private:
    ogs_sockaddr_t *m_sockaddr;
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_OPEN5GS_SBI_SOCKADDR_HH_ */
