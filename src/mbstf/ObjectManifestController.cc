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
#include "PullObjectIngester.hh"
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
	,m_scheduledPullCancel(false)

{

    if (dist_session.getObjectAcquisitionMethod() == "PULL") validate_pull_acquisition_method(dist_session);

    if (dist_session.getObjectAcquisitionMethod() == "PUSH") validate_push_acquisition_method(dist_session);
    initObjectIngester();
};


void ObjectManifestController::initPullObjectIngester()
{
    std::optional<std::string> object_ingest_base_url = distributionSession().getObjectIngestBaseUrl();
    std::optional<std::string> object_distribution_base_url = distributionSession().objectDistributionBaseUrl();

    auto &pull_urls = distributionSession().getObjectAcquisitionPullUrls();
    if (pull_urls.has_value()) {
        std::list<PullObjectIngester::IngestItem> urls;

        for (auto &url : pull_urls.value()) {
            std::string obj_ingest_url;

            if (url.has_value()) {
                const std::string &url_str = url.value();
                if (object_ingest_base_url.has_value()) {
                    if (url_str.starts_with("https:") || url_str.starts_with("http:") || url_str.starts_with("//")) {
                        ogs_error("Invalid objectAcquisitionPullUrl when objectIngestBaseUrl is set: %s", url.value().c_str());
                        continue;
                    } else {
                        obj_ingest_url = object_ingest_base_url.value();
                        if (!obj_ingest_url.ends_with("/")) obj_ingest_url += "/";
                        obj_ingest_url += distributionSession().trimSlashes(url_str);
                    }

                }

                urls.emplace_back(std::move(PullObjectIngester::IngestItem(nextObjectId(), obj_ingest_url, url_str,
                                                                           object_ingest_base_url, object_distribution_base_url)));
            }
        }

        addPullObjectIngester(new PullObjectIngester(objectStore(), *this, urls));
    }
}


void ObjectManifestController::initPushObjectIngester()
{
    const std::string objIngestBaseUrl;

    PushObjectIngester *pushIngester = new PushObjectIngester(objectStore(), *this);

    distributionSession().setObjectIngestBaseUrl(pushIngester->getIngestServerPrefix());
    subscribeTo({"ObjectPushStart"}, *pushIngester);
    setPushIngester(pushIngester);
}

void ObjectManifestController::initObjectIngester()
{
    if (distributionSession().getObjectAcquisitionMethod() == "PULL") {
        initPullObjectIngester();
    } else if (distributionSession().getObjectAcquisitionMethod() == "PUSH") {
        initPushObjectIngester();
    } else {
        ogs_error("Invalid Acq. method");
    }
}


void ObjectManifestController::workerLoop(ObjectManifestController *controller) {
    while (!controller->m_scheduledPullCancel) {
        while (true) {
            {
                std::lock_guard<std::recursive_mutex> lock(controller->m_manifestHandlerMutex);

                if ( controller->m_scheduledPullCancel || controller->m_manifestHandler != nullptr) {
                    break; 
                }
            }
            // Avoid a tight busy-loop: yield or sleep for a short time.
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

	if (controller->m_scheduledPullCancel) break;

        // Get the next ingest items
        auto next_ingest_items = controller->manifestHandler()->nextIngestItems();
        auto fetch_time = next_ingest_items.first; // Get fetch_time using .first
        auto &ingest_items = next_ingest_items.second; // Get ingest_items using .second

	std::list<PullObjectIngester::IngestItem> urls;

        std::list<std::shared_ptr<PullObjectIngester>> ingesters = controller->getPullObjectIngesters();
	while(ingesters.size() < ingest_items.size()) {
	    controller->addPullObjectIngester(new PullObjectIngester(controller->objectStore(), *controller, urls));
	}

        // Wait until the fetch_time
        std::this_thread::sleep_until(fetch_time);

        // Add the URLs to the PullObjectIngester instances
        auto ingester_it = ingesters.begin();
	while (!ingest_items.empty() && ingester_it != ingesters.end()) {
            auto ingest_item = ingest_items.front();
            ingest_items.pop_front();  // remove the item immediately
            ingest_item.deadline(std::chrono::system_clock::now() + controller->manifestHandler()->getDefaultDeadline());

            if (!(*ingester_it)->fetch(ingest_item)) {
                 ogs_info("Failed to fetch item: %s", ingest_item.url().c_str());
            }

            ++ingester_it;
	}
	if(ingest_items.empty()) {
	    std::lock_guard<std::recursive_mutex> lock(controller->m_manifestHandlerMutex);
	    controller->m_scheduledPullCancel = true;
	}
    }
}


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

std::string &ObjectManifestController::getManifestUrl() {
    manifestUrl();
    return m_manifestUrl;
}


void ObjectManifestController::manifestUrl()
{
    std::optional<std::string> manifest_base_url = distributionSession().getObjectIngestBaseUrl();
    std::string manifest_url;

    auto &pull_urls = distributionSession().getObjectAcquisitionPullUrls();
    if (pull_urls.has_value()) {
        for (auto &url : pull_urls.value()) {

            if (url.has_value()) {
                const std::string &url_str = url.value();
                if (manifest_base_url.has_value()) {
                    if (url_str.starts_with("https:") || url_str.starts_with("http:") || url_str.starts_with("//")) {
                        ogs_error("Invalid objectAcquisitionPullUrl when objectIngestBaseUrl is set: %s", url.value().c_str());
                        continue;
                    } else {
                        manifest_url = manifest_base_url.value();
                        if (!manifest_url.ends_with("/")) manifest_url += "/";
                        manifest_url += distributionSession().trimSlashes(url_str);
                    }

                }

            }
        }
    }
    m_manifestUrl = manifest_url;
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
