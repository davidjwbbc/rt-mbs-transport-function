#ifndef _MBS_TF_OPEN5GS_SBI_STREAM_HH_
#define _MBS_TF_OPEN5GS_SBI_STREAM_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS SBI Stream interface
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
#include "ogs-sbi.h"

#include <memory>

#include "common.hh"

#include "Open5GSSBIServer.hh"

MBSTF_NAMESPACE_START

class Open5GSSBIStream {
public:
    Open5GSSBIStream(ogs_sbi_stream_t *stream) :m_stream(stream) {};
    Open5GSSBIStream() = delete;
    Open5GSSBIStream(Open5GSSBIStream &&other) = delete;
    Open5GSSBIStream(const Open5GSSBIStream &other) = delete;
    Open5GSSBIStream &operator=(Open5GSSBIStream &&other) = delete;
    Open5GSSBIStream &operator=(const Open5GSSBIStream &other) = delete;
    virtual ~Open5GSSBIStream() {};

    ogs_sbi_stream_t *ogsSBIStream() { return m_stream; };
    const ogs_sbi_stream_t *ogsSBIStream() const { return m_stream; };

    operator bool() const { return !!m_stream; };

    Open5GSSBIServer server() const { return Open5GSSBIServer(m_stream?ogs_sbi_server_from_stream(m_stream):nullptr); };

private:
    ogs_sbi_stream_t *m_stream;
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_OPEN5GS_SBI_STREAM_HH_ */
