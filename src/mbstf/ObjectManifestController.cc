/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: ObjectManifestController class
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
 * Author(s): Dev Audsin <dev.audsin@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <exception>
#include <iostream>
#include <list>
#include <memory>
#include <optional>
#include <string>

#include <netinet/in.h>

#include "ogs-app.h"

#include "common.hh"
#include "DistributionSession.hh"
#include "Event.hh"
#include "ObjectController.hh"
#include "ObjectStore.hh"
#include "PushObjectIngester.hh"
#include "SubscriptionService.hh"

#include "ObjectManifestController.hh"


MBSTF_NAMESPACE_START

static bool validate_push_url(DistributionSession &distributionSession, const std::string &url);
static void validate_pull_acquisition_method(DistributionSession &distributionSession);
static bool validate_push_acquisition_method(DistributionSession &distributionSession);

ObjectManifestController::ObjectManifestController(DistributionSession &dist_session)
        :ObjectController(dist_session)
	,Subscriber()
        ,m_manifestHandler(nullptr)
{

    if (dist_session.getObjectAcquisitionMethod() == "PULL") validate_pull_acquisition_method(dist_session);
    if (dist_session.getObjectAcquisitionMethod() == "PUSH") {
        if(validate_push_acquisition_method(dist_session)){
            subscribeTo({"ObjectPushStart"}, objectStore());

	}

    }

};


void ObjectManifestController::processEvent(Event &event, SubscriptionService &event_service) {
    if (event.eventName() == "ObjectPushStart") {
        PushObjectIngester::ObjectPushEvent &obj_push_event = dynamic_cast<PushObjectIngester::ObjectPushEvent&>(event);
        const PushObjectIngester::Request &request(obj_push_event.request());
        const std::optional<std::string> &content_type(request.contentType());
        const std::string &url(request.urlPath());
        ogs_debug("Check mime_type [%s] is ok?", content_type?content_type.value().c_str():"<none>");
        if(!validate_push_url(distributionSession(), url)) {
            const_cast<PushObjectIngester::Request&>(request).setError(400, "Bad Request");
            event.preventDefault();
        }
        // event.preventDefault() if checks fail
    }
}


static void validate_pull_acquisition_method(DistributionSession &distributionSession) {
    if (distributionSession.getObjectAcquisitionMethod() == "PULL") {
        auto &pull_urls = distributionSession.getObjectAcquisitionPullUrls();
        if (pull_urls.has_value() && pull_urls->size() != 1) {
            throw std::runtime_error("objAcquisitionIdsPull must contain exactly one item when objDistributionOperatingMode is set to " + distributionSession.getObjectDistributionOperatingMode());
        }
        std::optional<std::string> object_acquisition_push_id = distributionSession.getObjectAcquisitionPushId();
        if (object_acquisition_push_id.has_value()) {

            throw std::runtime_error("objAcquisitionIdPush must not be present when objAcquisitionMethod is set to " + distributionSession.getObjectAcquisitionMethod());
        }
    }
}

static bool validate_push_acquisition_method(DistributionSession &distributionSession) {
    if (distributionSession.getObjectAcquisitionMethod() == "PUSH") {
        auto &pull_urls = distributionSession.getObjectAcquisitionPullUrls();
        if (pull_urls.has_value()) {
            throw std::runtime_error("objAcquisitionIdsPull must not be present when objAcquisitionMethod is set to " + distributionSession.getObjectAcquisitionMethod());
        } else {
            std::optional<std::string> object_acquisition_push_id = distributionSession.getObjectAcquisitionPushId();
            if (!object_acquisition_push_id.has_value()) {
                std::optional<std::string> id = "manifest";
                distributionSession.setObjectAcquisitionIdPush(id);
            }
	    return true;
        }
    }
    return false;

}

static bool validate_push_url(DistributionSession &distributionSession, const std::string &url)
{
    std::optional<std::string> object_acquisition_push_id = distributionSession.getObjectAcquisitionPushId();
    if (object_acquisition_push_id.has_value()) {
	std::string push_id = object_acquisition_push_id.value();
	std::string url_path(url);
	if ((push_id.front() == '/' && url_path.front() != '/')) {
	    url_path = '/' + url_path;
	} else if ((url_path.front() == '/' && push_id.front() != '/')) {
	    push_id = '/' + push_id;
	}
	if(push_id == url_path) {
	    return true;
	} else {
	    return false;
	}
    }

    return true;
}

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
