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
#include "ManifestHandlerFactory.hh"
#include "ObjectController.hh"
#include "ObjectStore.hh"
#include "PullObjectIngester.hh"
#include "PushObjectIngester.hh"
#include "SubscriptionService.hh"
#include "ObjectListPackager.hh"
#include "DASHManifestHandler.hh"

#include "ObjectStreamingController.hh"

MBSTF_NAMESPACE_START

static void validate_distribution_session(DistributionSession &distributionSession);
static bool check_if_object_added_is_manifest(std::string &objectId, ObjectStore &objectStore, std::string &manifest_url);

ObjectStreamingController::ObjectStreamingController(DistributionSession &distributionSession)
    :ObjectManifestController(distributionSession)
{
    validate_distribution_session(distributionSession);
    subscribeToService(objectStore());
    setObjectListPackager();
    subscribeToService(*getObjectListPackager());
    startWorker();
}

ObjectStreamingController::~ObjectStreamingController()
{
    abort();
}

std::shared_ptr<ObjectListPackager> &ObjectStreamingController::setObjectListPackager() {
    std::optional<std::string> dest_ip_addr = distributionSession().getDestIpAddr();
    std::optional<std::string> tunnel_addr = distributionSession().getTunnelAddr();
    uint32_t rate_limit = distributionSession().getRateLimit();
    in_port_t port = distributionSession().getPortNumber();
    in_port_t tunnel_port = distributionSession().getTunnelPortNumber();
    //TODO: get the MTU for the dest_ip_addr
    unsigned short mtu = 1500;
    m_objectListPackager.reset(new ObjectListPackager(objectStore(), *this, dest_ip_addr, rate_limit, mtu, port, tunnel_addr, tunnel_port));
    return m_objectListPackager;
}


void ObjectStreamingController::processEvent(Event &event, SubscriptionService &event_service) {
    if (event.eventName() == "ObjectAdded") {
        ObjectStore::ObjectAddedEvent &objAddedEvent = dynamic_cast<ObjectStore::ObjectAddedEvent&>(event);
        std::string objectId = objAddedEvent.objectId();
        ogs_info("Object added with ID: %s", objectId.c_str());
	if(check_if_object_added_is_manifest(objectId, objectStore(), getManifestUrl())) {
	    const ObjectStore::Object &object = objectStore()[objectId];
	    if(manifestHandler()) {
	        try {
	            if(!manifestHandler()->update(object)) {
		        ogs_error("Failed to update Manifest");
			unsetObjectListPackager();
			event.stopProcessing();
			return;
		    }
		    startWorker();

                    if (!m_objectListPackager) {
                        setObjectListPackager();
                    }

		    ObjectListPackager::PackageItem item(objectId);
		    m_objectListPackager->add(item);
	        } catch (std::exception &ex) {
                    ogs_error("Invalid Manifest update: %s", ex.what());
		    unsetObjectListPackager();
		    event.stopProcessing();
		    return;
                }

	    } else {
		std::unique_ptr<ManifestHandler> manifest_handler(ManifestHandlerFactory::makeManifestHandler(object, distributionSession().getObjectAcquisitionMethod() == "PULL"));
                manifestHandler(std::move(manifest_handler));
                /*
                const ObjectStore::Metadata &metadata = objectStore().getMetadata(objectId);
                try {
                    manifestHandler()->validateManifest(object, object_data, metadata);
                }  catch (std::exception &ex) {
                    ogs_info("InVALID Manifest Validation: %s", ex.what());
                    unsetObjectListPackager();
                    return;
                }
                */
                if (!m_objectListPackager) {
                    setObjectListPackager();
                }

		ObjectListPackager::PackageItem item(objectId);
		m_objectListPackager->add(item);
            }
	} else {
            if (!m_objectListPackager) {
                    setObjectListPackager();
            }

	    ObjectListPackager::PackageItem item(objectId);
            m_objectListPackager->add(item);
        }
    }
    ObjectManifestController::processEvent(event, event_service);
}
/*
std::string ObjectStreamingController::generateUUID() {
    uuid_t uuid;
    uuid_generate_random(uuid);
    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);
    return std::string(uuid_str);
}

std::string ObjectStreamingController::nextObjectId()
{
    return generateUUID();
}
*/

const std::optional<std::string> &ObjectStreamingController::getObjectDistributionBaseUrl() const {
    return distributionSession().objectDistributionBaseUrl();
}

namespace {
static const struct init { init() {ControllerFactory::registerController(new ControllerConstructor<ObjectStreamingController>);};} g_init;
}

static void validate_distribution_session(DistributionSession &distributionSession)
{
    if (distributionSession.getObjectDistributionOperatingMode() != "STREAMING") {
        throw std::logic_error("Expected objDistributionOperatingMode to be set to STREAMING.");
    }
}

static bool check_if_object_added_is_manifest(std::string &objectId, ObjectStore &objectStore, std::string &manifest_url) {
    ObjectStore::Metadata &metadata = objectStore.getMetadata(objectId);
    if(metadata.getOriginalUrl() == manifest_url || metadata.getFetchedUrl() == manifest_url) {
        metadata.keepAfterSend(true);
        return true;
    }
    /*
    if (metadata.mediaType() == "application/dash+xml" ){
	 return true;

    } */
    return false;

}

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
