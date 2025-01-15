#ifndef _MBS_TF_OPEN5GS_SBI_REQUEST_HH_
#define _MBS_TF_OPEN5GS_SBI_REQUEST_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS SBI Request interface
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

#include <string>

#include "common.hh"

MBSTF_NAMESPACE_START

class Open5GSSBIRequest {
public:
    Open5GSSBIRequest(ogs_sbi_request_t *request) :m_request(request) {};
    Open5GSSBIRequest() = delete;
    Open5GSSBIRequest(Open5GSSBIRequest &&other) = delete;
    Open5GSSBIRequest(const Open5GSSBIRequest &other) = delete;
    Open5GSSBIRequest &operator=(Open5GSSBIRequest &&other) = delete;
    Open5GSSBIRequest &operator=(const Open5GSSBIRequest &other) = delete;
    virtual ~Open5GSSBIRequest() {};

    ogs_sbi_request_t *ogsSBIRequest() { return m_request; };
    const ogs_sbi_request_t *ogsSBIRequest() const { return m_request; };

    operator bool() const { return !!m_request; };

    std::string headerValue(const std::string &field, const std::string &defval = std::string()) const;

    const char *content() const { return m_request?m_request->http.content:nullptr; };
    const char *uri() const { return m_request?m_request->h.uri:nullptr; };

private:
    ogs_sbi_request_t *m_request;
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_OPEN5GS_SBI_REQUEST_HH_ */
