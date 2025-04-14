#ifndef _MBS_TF_OBJECT_MANIFEST_CONTROLLER_HH_
#define _MBS_TF_OBJECT_MANIFEST_CONTROLLER_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Object Manifest Controller base class
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
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
#include "ObjectController.hh"

MBSTF_NAMESPACE_START

class DistributionSession;
class ManifestHandler;
class Event;
class SubscriptionService;

class ObjectManifestController : public ObjectController {
public:
    ObjectManifestController() = delete;
    ObjectManifestController(DistributionSession &dist_session)
        :ObjectController(dist_session)
        ,m_manifestHandler(nullptr)
        {};
    ObjectManifestController(const ObjectManifestController&) = delete;
    ObjectManifestController(ObjectManifestController&&) = delete;

    virtual ~ObjectManifestController() {};

    ObjectManifestController &operator=(const ObjectManifestController&) = delete;
    ObjectManifestController &operator=(ObjectManifestController&&) = delete;

protected:
    ObjectManifestController &manifestHandler(ManifestHandler *manifest_handler) {
        m_manifestHandler.reset(manifest_handler);
        //m_manifestHandler = manifest_handler;
        return *this;
    };
    ManifestHandler *manifestHandler() const { return m_manifestHandler.get(); };
    void validatePullAcquisitionMethod(DistributionSession &distributionSession);
    void validatePushAcquisitionMethod(DistributionSession &distributionSession);
    void validateObjectPushStart(Event &event, SubscriptionService &event_service);

private:
    std::unique_ptr<ManifestHandler> m_manifestHandler;
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_OBJECT_MANIFEST_CONTROLLER_HH_ */
