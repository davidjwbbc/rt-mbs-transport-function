/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS SBI Message interface
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

#include <stdexcept>
#include <memory>

#include "common.hh"
#include "Open5GSSBIRequest.hh"
#include "Open5GSSBIResponse.hh"

#include "Open5GSSBIMessage.hh"

MBSTF_NAMESPACE_START

void Open5GSSBIMessage::parseHeader(Open5GSSBIRequest &request)
{
    if (!m_message) {
        m_message = new ogs_sbi_message_t;
        m_owner = true;
    }
    int rv = ogs_sbi_parse_header(m_message, &request.ogsSBIRequest()->h);
    if (rv != OGS_OK) throw std::runtime_error("ogs_sbi_parse_header() failed");
}

void Open5GSSBIMessage::parseHeader(Open5GSSBIResponse &response)
{
    if (!m_message) {
        m_message = new ogs_sbi_message_t;
        m_owner = true;
    }
    int rv = ogs_sbi_parse_header(m_message, &response.ogsSBIResponse()->h);
    if (rv != OGS_OK) throw std::runtime_error("ogs_sbi_parse_header() failed");
}

const char *Open5GSSBIMessage::resourceComponent(size_t idx) const
{
    if (!m_message) return nullptr;
    if (idx >= OGS_SBI_MAX_NUM_OF_RESOURCE_COMPONENT) return nullptr;
    return m_message->h.resource.component[idx];
}

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
