#ifndef _MBS_TF_OPEN5GS_SBI_SERVER_HH_
#define _MBS_TF_OPEN5GS_SBI_SERVER_HH_
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

#include "ogs-sbi.h"

#include <memory>

#include "common.hh"

MBSTF_NAMESPACE_START

class Open5GSSBIServer {
public:
    Open5GSSBIServer();
    Open5GSSBIServer(ogs_socknode_t *node,  ogs_sockopt_t *option);
    Open5GSSBIServer(Open5GSSBIServer &&other) = delete;
    Open5GSSBIServer(const Open5GSSBIServer &other) = delete;
    Open5GSSBIServer &operator=(Open5GSSBIServer &&other) = delete;
    Open5GSSBIServer &operator=(const Open5GSSBIServer &other) = delete;
    virtual ~Open5GSSBIServer();

    ogs_sockaddr_t *ogsSockaddr(std::shared_ptr<Open5GSSBIServer> &server);
    void ogsSBIServerAdvertise(ogs_sockaddr_t *addr);
    ogs_sbi_server_t *ogsSBIServer() { return m_ogsServer; };

private:
    ogs_sbi_server_t *m_ogsServer;
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_OPEN5GS_SBI_SERVER_HH_ */
