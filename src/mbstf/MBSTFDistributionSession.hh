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

#include <chrono>
#include <memory>

#include "common.hh"

namespace fiveg_mag_reftools {
    class CJson;
}

namespace reftools::mbstf {
    class CreateReqData;
}

using fiveg_mag_reftools::CJson; 
using reftools::mbstf::CreateReqData;

MBSTF_NAMESPACE_START

class Open5GSEvent;

class MBSTFDistributionSession {
public:
    using SysTimeMS = std::chrono::system_clock::time_point;
    MBSTFDistributionSession(CJson &json, bool as_request);
    MBSTFDistributionSession(const std::shared_ptr<CreateReqData> &create_req_data);
    MBSTFDistributionSession() = delete;
    MBSTFDistributionSession(MBSTFDistributionSession &&other) = delete;
    MBSTFDistributionSession(const MBSTFDistributionSession &other) = delete;
    MBSTFDistributionSession &operator=(MBSTFDistributionSession &&other) = delete;
    MBSTFDistributionSession &operator=(const MBSTFDistributionSession &other) = delete;

    virtual ~MBSTFDistributionSession();

    CJson json(bool as_request) const;

    static const std::shared_ptr<MBSTFDistributionSession> &find(const std::string &id); // throws std::out_of_range if id does not exist
    const std::string &distributionSessionId() const { return m_distributionSessionId; };
    const std::shared_ptr<CreateReqData> &distributionSessionReqData() const {return m_createReqData;};
    const SysTimeMS &generated() const {return m_generated;};
    const std::string &hash() const {return m_hash;};

    static bool processEvent(Open5GSEvent &event);

private:
    std::shared_ptr<CreateReqData> m_createReqData;
    SysTimeMS m_generated;
    SysTimeMS m_lastUsed;
    std::string m_hash;
    std::string m_distributionSessionId;
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_MBSTF_DISTRIBUTION_SESSION_HH_ */
