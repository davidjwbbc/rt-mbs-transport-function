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

ObjectListController::ObjectListController(DistributionSession &distributionSession)
    :ObjectController(distributionSession)
    ,Subscriber()	
{
    subscribeToService(objectStore());
    initPullObjectIngester(distributionSession);
    setObjectListPackager(distributionSession, objectStore());
}

ObjectListController::~ObjectListController()
{
}

std::shared_ptr<ObjectListPackager> &ObjectListController::setObjectListPackager(DistributionSession &distributionSession, ObjectStore &object_store) {
    std::shared_ptr<std::string>destIpAddr = getdestIpAddr(distributionSession);
    uint32_t rateLimit = getRateLimit(distributionSession);
    short port = getPortNumber(distributionSession);
    //TODO: getMTU() but from where does this value come from??
    unsigned short mtu = 1500;
    m_objectListPackager.reset(new ObjectListPackager(object_store, *this, destIpAddr, rateLimit, mtu, port));
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

void ObjectListController::initPullObjectIngester(DistributionSession &distributionSession)
{

    auto &pull_urls = getObjectAcquisitionPullUrls(distributionSession);
    if (pull_urls.has_value()) {
        std::list<PullObjectIngester::IngestItem> urls;

        for(auto &url : pull_urls.value()) {
            if (url.has_value()) {
                urls.emplace_back(std::move(PullObjectIngester::IngestItem(generateUUID(), url.value())));
            }
        }

        addPullObjectIngester(new PullObjectIngester(objectStore(), *this, urls));
    }
}

const ObjDistributionData::ObjAcquisitionIdsPullType &ObjectListController::getObjectAcquisitionPullUrls(DistributionSession &distributionSession) const
{
    const std::shared_ptr<CreateReqData> createReqData = distributionSession.distributionSessionReqData();
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

std::shared_ptr<std::string> ObjectListController::getdestIpAddr(DistributionSession &distributionSession) const
{
    const std::shared_ptr<CreateReqData> createReqData = distributionSession.distributionSessionReqData();
    std::shared_ptr<DistSession> distSession = createReqData->getDistSession();
    const std::optional<std::shared_ptr< UpTrafficFlowInfo > > &upTrafficFlowInfo = distSession->getUpTrafficFlowInfo();
    if(upTrafficFlowInfo.has_value()) {
          std::shared_ptr<UpTrafficFlowInfo> upTrafficFlow = upTrafficFlowInfo.value();
          const std::shared_ptr<IpAddr> ipAddr = upTrafficFlow->getDestIpAddr();
          if (ipAddr) {
              //std::optional<std::string> addr;
              const auto &addr = ipAddr->getIpv4Addr();
              if (addr.has_value()){
                  std::shared_ptr<std::string> ipAddrPtr(new std::string(addr.value()));
                  return ipAddrPtr;
              }
          }
    }

    return nullptr;
}

short ObjectListController::getPortNumber(DistributionSession &distributionSession) const
{
    int32_t portNumber = 0;
    const std::shared_ptr<CreateReqData> createReqData = distributionSession.distributionSessionReqData();
    std::shared_ptr<DistSession> distSession = createReqData->getDistSession();
    std::optional<std::shared_ptr< UpTrafficFlowInfo > > upTrafficFlowInfo = distSession->getUpTrafficFlowInfo();
    if(upTrafficFlowInfo.has_value()) {
	  std::shared_ptr<UpTrafficFlowInfo> upTrafficFlow = upTrafficFlowInfo.value();
          portNumber = upTrafficFlow->getPortNumber();
    }
    return static_cast<short>(portNumber);
}


uint32_t ObjectListController::getRateLimit(DistributionSession &distributionSession) const
{
    const std::shared_ptr<CreateReqData> createReqData = distributionSession.distributionSessionReqData();
    std::shared_ptr<DistSession> distSession = createReqData->getDistSession();
    const std::optional<std::string> &mbr = distSession->getMbr();

    if (mbr) {
        try {
            return static_cast<uint32_t>(std::stoul(*mbr));
        } catch (const std::invalid_argument &e) {
            throw std::runtime_error("Invalid MBR value");
        } catch (const std::out_of_range &e) {
            throw std::runtime_error("MBR value out of range");
        }
    } else {
        return 0;
    }
}

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
