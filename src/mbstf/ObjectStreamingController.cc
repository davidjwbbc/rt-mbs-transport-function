/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: ObjectStreamingController class
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

#include <uuid/uuid.h>

#include "ogs-app.h"

#include "common.hh"
#include "ControllerFactory.hh"
#include "DistributionSession.hh"
#include "Event.hh"
#include "ObjectController.hh"
#include "ObjectStore.hh"
#include "PullObjectIngester.hh"
#include "PushObjectIngester.hh"
#include "SubscriptionService.hh"
#include "ObjectManifestController.hh"

#include "ObjectStreamingController.hh"

MBSTF_NAMESPACE_START

static std::string trim_slashes(const std::string &path);

ObjectStreamingController::ObjectStreamingController(DistributionSession &distributionSession)
    :ObjectManifestController(distributionSession)
    ,Subscriber()
{
    validateStreamingDistributionSession(distributionSession);
    subscribeToService(objectStore());
    initObjectIngester();
}

ObjectStreamingController::~ObjectStreamingController()
{
}

void ObjectStreamingController::processEvent(Event &event, SubscriptionService &event_service) {
    if (event.eventName() == "ObjectAdded") {
        ObjectStore::ObjectAddedEvent &objAddedEvent = dynamic_cast<ObjectStore::ObjectAddedEvent&>(event);
        std::string objectId = objAddedEvent.objectId();
        ogs_info("Object added with ID: %s", objectId.c_str());

    } else if (event.eventName() == "ObjectPushStart") {
	validateObjectPushStart(event, event_service);    
    }
}

std::string ObjectStreamingController::generateUUID() {
    uuid_t uuid;
    uuid_generate_random(uuid);
    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);
    return std::string(uuid_str);
}

void ObjectStreamingController::initPullObjectIngester()
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
                        obj_ingest_url += trim_slashes(url_str);
                    }

                }

                urls.emplace_back(std::move(PullObjectIngester::IngestItem(nextObjectId(), obj_ingest_url, url_str,
                                                                           object_ingest_base_url, object_distribution_base_url)));
            }
        }

        addPullObjectIngester(new PullObjectIngester(objectStore(), *this, urls));
    }
}


void ObjectStreamingController::initPushObjectIngester()
{
    const std::string objIngestBaseUrl;

    PushObjectIngester *pushIngester = new PushObjectIngester(objectStore(), *this);

    distributionSession().setObjectIngestBaseUrl(pushIngester->getIngestServerPrefix());
    subscribeTo({"ObjectPushStart"}, *pushIngester);
    setPushIngester(pushIngester);
}

void ObjectStreamingController::initObjectIngester()
{
    if (distributionSession().getObjectAcquisitionMethod() == "PULL") {
        initPullObjectIngester();
    } else if (distributionSession().getObjectAcquisitionMethod() == "PUSH") {
        initPushObjectIngester();
    } else {
        ogs_error("Invalid Acq. method");
    }
}

std::string ObjectStreamingController::nextObjectId()
{
    return generateUUID();
}

const std::optional<std::string> &ObjectStreamingController::getObjectDistributionBaseUrl() const {
    return distributionSession().objectDistributionBaseUrl();
}

void ObjectStreamingController::validateStreamingDistributionSession(DistributionSession &distributionSession)
{
    if (distributionSession.getObjectDistributionOperatingMode() != "STREAMING") {
        throw std::logic_error("Expected objDistributionOperatingMode to be set to STREAMING.");
    }
    if (distributionSession.getObjectAcquisitionMethod() == "PULL") validatePullAcquisitionMethod(distributionSession);
    if (distributionSession.getObjectAcquisitionMethod() == "PUSH") validatePushAcquisitionMethod(distributionSession);
}


namespace {
static const struct init { init() {ControllerFactory::registerController(new ControllerConstructor<ObjectStreamingController>);};} g_init;
}

static std::string trim_slashes(const std::string &path)
{
    size_t start = path.starts_with('/') ? 1 : 0;
    size_t end = path.ends_with('/') ? path.size() - 1 : path.size();

    return path.substr(start, end - start);
}

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
