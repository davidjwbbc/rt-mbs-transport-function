/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSTF Object packager base class
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include "common.hh"
#include "ObjectPackager.hh"

MBSTF_NAMESPACE_START

void ObjectPackager::workerLoop(ObjectPackager *packager)
{
    while(!packager->m_workerCancel){
       packager->doObjectPackage();
    }	    
}

ObjectPackager& ObjectPackager::setDestIpAddr(const std::optional<std::string> &dest_ip_addr) {
    m_destIpAddr = dest_ip_addr;
    return *this;
}

ObjectPackager& ObjectPackager::setPort(in_port_t port) {
    m_port = port;
    return *this;
}

ObjectPackager& ObjectPackager::setMtu(unsigned short mtu) {
    m_mtu = mtu;
    return *this;
}

ObjectPackager& ObjectPackager::setRateLimit(uint32_t rateLimit) {
    m_rateLimit = rateLimit;
    return *this;
}

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
