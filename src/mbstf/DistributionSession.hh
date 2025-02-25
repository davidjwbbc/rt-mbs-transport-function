#ifndef _MBS_TF_MBSTF_DISTRIBUTION_SESSION_HH_
#define _MBS_TF_MBSTF_DISTRIBUTION_SESSION_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSTF Network Function class
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 *
 * Licensed under the License terms and conditions for use, reproduction, and
 * distribution of 5G-MAG software (the “License”).  You may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 * https://www.5g-mag.com/reference-tools.  Unless required by applicable law or
 * agreed to in writing, software distributed under the License is distributed on
 * an “AS IS” BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied.
 *
 * See the License for the specific language governing permissions and limitations
 * under the License.
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
class Controller;

class DistributionSession {
public:
    using SysTimeMS = std::chrono::system_clock::time_point;
    DistributionSession(CJson &json, bool as_request);
    DistributionSession(const std::shared_ptr<CreateReqData> &create_req_data);
    DistributionSession() = delete;
    DistributionSession(DistributionSession &&other) = delete;
    DistributionSession(const DistributionSession &other) = delete;
    DistributionSession &operator=(DistributionSession &&other) = delete;
    DistributionSession &operator=(const DistributionSession &other) = delete;

    virtual ~DistributionSession();

    CJson json(bool as_request) const;

    static const std::shared_ptr<DistributionSession> &find(const std::string &id); // throws std::out_of_range if id does not exist
    const std::string &distributionSessionId() const { return m_distributionSessionId; };
    const std::shared_ptr<CreateReqData> &distributionSessionReqData() const {return m_createReqData;};
    const SysTimeMS &generated() const {return m_generated;};
    const std::string &hash() const {return m_hash;};
    void setController(std::shared_ptr<Controller> controller) {m_controller = controller;};

    static bool processEvent(Open5GSEvent &event);

private:
    std::shared_ptr<CreateReqData> m_createReqData;
    SysTimeMS m_generated;
    SysTimeMS m_lastUsed;
    std::string m_hash;
    std::string m_distributionSessionId;
    std::shared_ptr<Controller> m_controller;
    //ObjectStore m_objectStore;
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_MBSTF_DISTRIBUTION_SESSION_HH_ */
