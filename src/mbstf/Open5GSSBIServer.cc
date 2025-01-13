/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS Application interface
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include "ogs-app.h"
#include "ogs-sbi.h"

#include <memory>
#include <stdexcept>

#include "common.hh"
#include "App.hh"
#include "TimerFunc.hh"
#include "Open5GSEvent.hh"
#include "Open5GSTimer.hh"
#include "Open5GSYamlDocument.hh"

#include "Open5GSSBIServer.hh"

MBSTF_NAMESPACE_START

Open5GSSBIServer::Open5GSSBIServer()
{
    
}

Open5GSSBIServer::Open5GSSBIServer(ogs_socknode_t *node,  ogs_sockopt_t *option)
{
    m_ogsServer = ogs_sbi_server_add(node->addr, option);
    ogs_assert(m_ogsServer);
}

Open5GSSBIServer::~Open5GSSBIServer()
{

}

void Open5GSSBIServer::ogsSBIServerAdvertise(ogs_sockaddr_t *addr) {
    if (addr && ogs_app()->parameter.no_ipv4 == 0) {
        ogs_sbi_server_set_advertise(this->ogsSBIServer(), AF_INET, addr);
    }
}



MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
