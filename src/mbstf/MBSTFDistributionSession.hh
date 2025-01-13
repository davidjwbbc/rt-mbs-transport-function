#ifndef _MBS_TF_MBSTF_DISTRIBUTION_SESSION_HH_
#define _MBS_TF_MBSTF_DISTRIBUTION_SESSION_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSTF Network Function class
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include "ogs-app.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include <memory>

#include "common.hh"

#include "openapi/model/CJson.hh"
#include "openapi/model/CreateReqData.h"

using fiveg_mag_reftools::CJson; 
using reftools::mbstf::CreateReqData;

MBSTF_NAMESPACE_START

class MBSTFDistributionSession {
public:
    MBSTFDistributionSession(CJson *json, bool as_request);
    MBSTFDistributionSession(const std::shared_ptr<CreateReqData> &create_req_data);
    MBSTFDistributionSession() = delete;
    MBSTFDistributionSession(MBSTFDistributionSession &&other) = delete;
    MBSTFDistributionSession(const MBSTFDistributionSession &other) = delete;
    MBSTFDistributionSession &operator=(MBSTFDistributionSession &&other) = delete;
    MBSTFDistributionSession &operator=(const MBSTFDistributionSession &other) = delete;

    virtual ~MBSTFDistributionSession();

    CJson *json(bool as_request) const;
    const std::string &id() const;

    static const std::shared_ptr<MBSTFDistributionSession> &find(const std::string &id); // throws std::out_of_range if id does not exist
    char *calculate_DistributionSessionHash(CJson *json);
    std::string distributionSessionId() { return m_distributionSessionId; };
    std::shared_ptr<CreateReqData> distributionSessionReqData() {return m_createReqData;};
    ogs_time_t generated() {return m_generated;};
    std::string hash() {return m_hash;};

    static bool processEvent(ogs_event_t *e);

    

private:
    std::shared_ptr<CreateReqData> m_createReqData;
    ogs_time_t m_generated;
    ogs_time_t m_lastUsed;
    std::string m_hash;
    std::string m_distributionSessionId;

};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_MBSTF_DISTRIBUTION_SESSION_HH_ */
