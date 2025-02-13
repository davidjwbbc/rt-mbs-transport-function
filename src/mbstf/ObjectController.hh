#ifndef _MBS_TF_MBSTF_CONTROLLER_HH_
#define _MBS_TF_MBSTF_CONTROLLER_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSTF Object store class
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <memory>
#include <list>

#include "common.hh"
#include "Controller.hh"
#include "ObjectStore.hh"

MBSTF_NAMESPACE_START

class DistributionSession;
class PullObjectIngester;

class ObjectController : public Controller {
public:
    ObjectController() = delete;
    ObjectController(DistributionSession &distributionSession)
        :Controller(distributionSession)
        ,m_objectStore()
        ,m_pullIngesters()
    {};
    ObjectController(const ObjectController &) = delete;
    ObjectController(ObjectController &&) = delete;

    virtual ~ObjectController() {};

    ObjectController &operator=(const ObjectController &) = delete;
    ObjectController &operator=(ObjectController &&) = delete;

    const ObjectStore &objectStore() const { return m_objectStore; };

protected:
    std::shared_ptr<PullObjectIngester> &addPullObjectIngester(PullObjectIngester&&);
	
private:
    ObjectStore m_objectStore;
    std::list<std::shared_ptr<PullObjectIngester>> m_pullIngesters;
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_MBSTF_CONTROLLER_HH_ */
