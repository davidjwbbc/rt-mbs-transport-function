#ifndef _MBS_TF_MBSTF_CONTROLLER_HH_
#define _MBS_TF_MBSTF_CONTROLLER_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSTF Object store class
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#pragma once

#include "ogs-app.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include <memory>
#include <utility>
#include <chrono>
#include <vector>
#include <map>
#include <list>

#include "common.hh"
//#include "MBSTFDistributionSession.hh"
//#include "ObjectStore.hh"


MBSTF_NAMESPACE_START


class MBSTFDistributionSession;
class ObjectStore;
class PullObjectIngester;

class Controller {
public:
    Controller() = delete;
    ~Controller() {};
    Controller(MBSTFDistributionSession &distributionSession, ObjectStore &objectStore);
    ObjectStore& objectStore(MBSTFDistributionSession &distributionSession);

protected:
    std::shared_ptr<PullObjectIngester> &addPullObjectIngester(PullObjectIngester&&);
	
private:
    MBSTFDistributionSession &m_distributionSession;
    ObjectStore &m_objectStore;
    std::list<std::shared_ptr<PullObjectIngester>> m_pullIngesters;

};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_MBSTF_CONTROLLER_HH_ */
