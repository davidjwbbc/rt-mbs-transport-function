/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSTF ObjectListController
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
#include "DistributionSession.hh"
#include "Event.hh"
#include "ObjectController.hh"
#include "ObjectListPackager.hh"
#include "ObjectStore.hh"
#include "openapi/model/CreateReqData.h"
#include "openapi/model/DistSession.h"
#include "openapi/model/IpAddr.h"
#include "openapi/model/ObjDistributionData.h"
#include "openapi/model/UpTrafficFlowInfo.h"
#include "PullObjectIngester.hh"
#include "SubscriptionService.hh"

#include "ObjectListController.hh"

using reftools::mbstf::CreateReqData;
using reftools::mbstf::DistSession;
using reftools::mbstf::IpAddr;
using reftools::mbstf::ObjDistributionData;
using reftools::mbstf::UpTrafficFlowInfo;

MBSTF_NAMESPACE_START

static const ObjDistributionData::ObjAcquisitionIdsPullType &get_object_acquisition_pull_urls(
                                                                                        DistributionSession &distributionSession);
static const std::optional<std::string> &get_dest_ip_addr(DistributionSession &distributionSession);
static in_port_t get_port_number(DistributionSession &distributionSession);
static uint32_t get_rate_limit(DistributionSession &distributionSession);
static const std::optional<std::string > getObjectIngestBaseUrl(DistributionSession &distributionSession);
static const std::optional<std::string > getObjectDistributionBaseUrl(DistributionSession &distributionSession);


ObjectListController::ObjectListController(DistributionSession &distributionSession)
    :ObjectController(distributionSession)
    ,Subscriber()	
{
    subscribeToService(objectStore());
    initPullObjectIngester();
    setObjectListPackager();
}

ObjectListController::~ObjectListController()
{
}

std::shared_ptr<ObjectListPackager> &ObjectListController::setObjectListPackager() {
    std::optional<std::string> dest_ip_addr = get_dest_ip_addr(distributionSession());
    uint32_t rate_limit = get_rate_limit(distributionSession());
    in_port_t port = get_port_number(distributionSession());
    //TODO: get the MTU for the dest_ip_addr
    unsigned short mtu = 1500;
    m_objectListPackager.reset(new ObjectListPackager(objectStore(), *this, dest_ip_addr, rate_limit, mtu, port));
    return m_objectListPackager;
}


void ObjectListController::processEvent(Event &event, SubscriptionService &event_service) {
    if (event.eventName() == "ObjectAdded") {
        ObjectStore::ObjectAddedEvent &objAddedEvent = dynamic_cast<ObjectStore::ObjectAddedEvent&>(event);
        std::string objectId = objAddedEvent.objectId();
	ogs_info("Object added with ID: %s", objectId.c_str());

        ObjectListPackager::PackageItem item(objectId);
        if (m_objectListPackager) {
            m_objectListPackager->add(item);
        } else {
	    ogs_error("ObjectListPackager is not initialized.");
        }
    }
}

std::string ObjectListController::generateUUID() {
    uuid_t uuid;
    uuid_generate_random(uuid);
    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);
    return std::string(uuid_str);
}

void ObjectListController::initPullObjectIngester()
{
    std::string objectIngestBaseUrl;
    std::string objectDistributionBaseUrl;
    const std::optional<std::string>  objIngestBaseUrl = getObjectIngestBaseUrl(distributionSession());
    const std::optional<std::string>  objDistributionBaseUrl = getObjectDistributionBaseUrl(distributionSession());
    
    if (objIngestBaseUrl.has_value()) {
        objectIngestBaseUrl = objIngestBaseUrl.value();

    }

    if (objDistributionBaseUrl.has_value()) {
        objectDistributionBaseUrl = objDistributionBaseUrl.value();
    }


    auto &pull_urls = get_object_acquisition_pull_urls(distributionSession());
    if (pull_urls.has_value()) {
        std::list<PullObjectIngester::IngestItem> urls;

        for(auto &url : pull_urls.value()) {
	    std::shared_ptr<std::string> objIngestUrl;
            std::shared_ptr<std::string> objDistributionUrl;

            if (url.has_value()) {
		if(!objectIngestBaseUrl.empty()) {
                    if (url.value().substr(0, 4) == "http" || url.value().substr(0, 2) == "//"){
                        ogs_error("Invalid ObjectAcquisitionPullUrl");
                        continue;
                    } else {
                        objIngestUrl.reset(new std::string(trimSlashes(objectIngestBaseUrl) + "/" + trimSlashes(url.value())));
                    }

                }

                if(!objectDistributionBaseUrl.empty()) {
                    if (url.value().substr(0, 4) == "http"){
                        objDistributionUrl.reset(new std::string(trimSlashes(objectDistributionBaseUrl) + "/" + removeBaseURL(url.value())));
                    } else {

                        objDistributionUrl.reset(new std::string(trimSlashes(objectDistributionBaseUrl) + "/" + trimSlashes(url.value())));
                    }

                }
		urls.emplace_back(std::move(PullObjectIngester::IngestItem(generateUUID(), url.value(), objIngestUrl, objDistributionUrl, objIngestBaseUrl, objDistributionBaseUrl)));
            }
        }

        addPullObjectIngester(new PullObjectIngester(objectStore(), *this, urls));
    }
}

static const ObjDistributionData::ObjAcquisitionIdsPullType &get_object_acquisition_pull_urls(
                                                                                        DistributionSession &distributionSession)
{
    std::shared_ptr<CreateReqData> createReqData = distributionSession.distributionSessionReqData();
    std::shared_ptr<DistSession> distSession = createReqData->getDistSession();
    const std::optional<std::shared_ptr<ObjDistributionData> > &object_distribution_data = distSession->getObjDistributionData();
    if (object_distribution_data.has_value()) {
        std::shared_ptr<ObjDistributionData> objectDistributionDataPtr = object_distribution_data.value();
        return objectDistributionDataPtr->getObjAcquisitionIdsPull();
    } else {
        ogs_error("ObjectDistributionData is not available");
	static const ObjDistributionData::ObjAcquisitionIdsPullType empty_result;
        return empty_result;
    }
}

static const std::optional<std::string > getObjectIngestBaseUrl(DistributionSession &distributionSession)
{
    const std::shared_ptr<CreateReqData> createReqData = distributionSession.distributionSessionReqData();
    std::shared_ptr<DistSession> distSession = createReqData->getDistSession();
    const std::optional<std::shared_ptr<ObjDistributionData> > &object_distribution_data = distSession->getObjDistributionData();
    if (object_distribution_data.has_value()) {
        std::shared_ptr<ObjDistributionData> objectDistributionDataPtr = object_distribution_data.value();
        return objectDistributionDataPtr->getObjIngestBaseUrl();
    } else {
        ogs_error("ObjectDistributionData is not available");
        static const std::optional<std::string> nullValue = std::nullopt;
        return nullValue;
    }
}

static const std::optional<std::string > getObjectDistributionBaseUrl(DistributionSession &distributionSession)
{
    const std::shared_ptr<CreateReqData> createReqData = distributionSession.distributionSessionReqData();
    std::shared_ptr<DistSession> distSession = createReqData->getDistSession();
    const std::optional<std::shared_ptr<ObjDistributionData> > &object_distribution_data = distSession->getObjDistributionData();
    if (object_distribution_data.has_value()) {
        std::shared_ptr<ObjDistributionData> objectDistributionDataPtr = object_distribution_data.value();
        return objectDistributionDataPtr->getObjDistributionBaseUrl();
    } else {
        ogs_error("ObjectDistributionData is not available");
        static const std::optional<std::string> nullValue = std::nullopt;
        return nullValue;
    }
}

static const std::optional<std::string> &get_dest_ip_addr(DistributionSession &distributionSession)
{
    std::shared_ptr<CreateReqData> createReqData = distributionSession.distributionSessionReqData();
    std::shared_ptr<DistSession> distSession = createReqData->getDistSession();
    const std::optional<std::shared_ptr< UpTrafficFlowInfo > > &upTrafficFlowInfo = distSession->getUpTrafficFlowInfo();
    if (upTrafficFlowInfo.has_value()) {
        std::shared_ptr<UpTrafficFlowInfo> upTrafficFlow = upTrafficFlowInfo.value();
        const std::shared_ptr<IpAddr> ipAddr = upTrafficFlow->getDestIpAddr();
        if (ipAddr) {
            return ipAddr->getIpv4Addr();
        }
    }

    static const std::optional<std::string> empty = std::nullopt;
    return empty;
}

static in_port_t get_port_number(DistributionSession &distributionSession)
{
    in_port_t portNumber = 0;
    std::shared_ptr<CreateReqData> createReqData = distributionSession.distributionSessionReqData();
    std::shared_ptr<DistSession> distSession = createReqData->getDistSession();
    std::optional<std::shared_ptr<UpTrafficFlowInfo> > upTrafficFlowInfo = distSession->getUpTrafficFlowInfo();
    if (upTrafficFlowInfo.has_value()) {
        std::shared_ptr<UpTrafficFlowInfo> upTrafficFlow = upTrafficFlowInfo.value();
        portNumber = static_cast<in_port_t>(upTrafficFlow->getPortNumber());
    }
    return portNumber;
}


static uint32_t get_rate_limit(DistributionSession &distributionSession)
{
    std::shared_ptr<CreateReqData> createReqData = distributionSession.distributionSessionReqData();
    std::shared_ptr<DistSession> distSession = createReqData->getDistSession();
    const std::optional<std::string> &mbr = distSession->getMbr();

    if (mbr) {
        try {
            return static_cast<uint32_t>(std::stoul(mbr.value()));
        } catch (const std::invalid_argument &e) {
            throw std::runtime_error("Invalid MBR value");
        } catch (const std::out_of_range &e) {
            throw std::runtime_error("MBR value out of range");
        }
    }

    return 0;
}

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
