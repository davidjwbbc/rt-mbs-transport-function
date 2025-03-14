/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: ObjectListController class
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
#include "openapi/model/ObjAcquisitionMethod.h"
#include "PullObjectIngester.hh"
#include "PushObjectIngester.hh"
#include "SubscriptionService.hh"

#include "ObjectListController.hh"

using reftools::mbstf::CreateReqData;
using reftools::mbstf::DistSession;
using reftools::mbstf::IpAddr;
using reftools::mbstf::ObjDistributionData;
using reftools::mbstf::UpTrafficFlowInfo;
using reftools::mbstf::ObjAcquisitionMethod;

MBSTF_NAMESPACE_START

static std::string trim_slashes(const std::string &path);
static const ObjDistributionData::ObjAcquisitionIdsPullType &get_object_acquisition_pull_urls(
                                                                                        DistributionSession &distributionSession);
static const std::optional<std::string> &get_dest_ip_addr(DistributionSession &distributionSession);
static in_port_t get_port_number(DistributionSession &distributionSession);
static uint32_t get_rate_limit(DistributionSession &distributionSession);
static const std::optional<std::string> &get_object_ingest_base_url(DistributionSession &distributionSession);
static const std::string &get_object_acquisition_method(DistributionSession &distributionSession);
static void set_object_ingest_base_url(DistributionSession &distributionSession, std::string ingestBaseUrl);
ObjectListController::ObjectListController(DistributionSession &distributionSession)
    :ObjectController(distributionSession)
    ,Subscriber()
{
    subscribeToService(objectStore());
    initObjectIngester();
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
    std::optional<std::string> object_ingest_base_url = get_object_ingest_base_url(distributionSession());
    std::optional<std::string> object_distribution_base_url = objectDistributionBaseUrl();

    auto &pull_urls = get_object_acquisition_pull_urls(distributionSession());
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

                urls.emplace_back(std::move(PullObjectIngester::IngestItem(generateUUID(), obj_ingest_url, url_str,
                                                                           object_ingest_base_url, object_distribution_base_url)));
            }
        }

        addPullObjectIngester(new PullObjectIngester(objectStore(), *this, urls));
    }
}


void ObjectListController::initPushObjectIngester()
{
    const std::string objIngestBaseUrl;

    PushObjectIngester *pushIngester = new PushObjectIngester(objectStore(), *this);

    set_object_ingest_base_url(distributionSession(), pushIngester->getIngestServerPrefix());
    setPushIngester(pushIngester);
}

void ObjectListController::initObjectIngester()
{
    if(get_object_acquisition_method(distributionSession()) == "PULL")
    {
        initPullObjectIngester();

    } else
    if(get_object_acquisition_method(distributionSession()) == "PUSH")
    {
        initPushObjectIngester();

    } else {
        ogs_error("Invalid Acq. method");
    }
}

const std::optional<std::string> &ObjectListController::objectDistributionBaseUrl() const
{
    std::shared_ptr<CreateReqData> createReqData = distributionSession().distributionSessionReqData();
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

static std::string trim_slashes(const std::string &path)
{
    size_t start = path.starts_with('/') ? 1 : 0;
    size_t end = path.ends_with('/') ? path.size() - 1 : path.size();

    return path.substr(start, end - start);
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

static const std::string &get_object_acquisition_method(DistributionSession &distributionSession)
{
    std::shared_ptr<CreateReqData> createReqData = distributionSession.distributionSessionReqData();
    std::shared_ptr<DistSession> distSession = createReqData->getDistSession();
    const std::optional<std::shared_ptr<ObjDistributionData> > &object_distribution_data = distSession->getObjDistributionData();
    if (object_distribution_data.has_value()) {
        std::shared_ptr<ObjDistributionData> objectDistributionDataPtr = object_distribution_data.value();
        std::shared_ptr< ObjAcquisitionMethod > objAcquisitionMethod = objectDistributionDataPtr->getObjAcquisitionMethod();
        return objAcquisitionMethod->getString();
    } else {
        ogs_error("ObjectDistributionData is not available");
        static std::string emptyObjAcquisitionMethod = std::string();
        return emptyObjAcquisitionMethod;
    }
}


static const std::optional<std::string> &get_object_ingest_base_url(DistributionSession &distributionSession)
{
    std::shared_ptr<CreateReqData> createReqData = distributionSession.distributionSessionReqData();
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


static void set_object_ingest_base_url(DistributionSession &distributionSession, std::string ingestBaseUrl)
{
    const std::shared_ptr<CreateReqData> createReqData = distributionSession.distributionSessionReqData();
    std::shared_ptr<DistSession> distSession = createReqData->getDistSession();
    const std::optional<std::shared_ptr<ObjDistributionData> > &object_distribution_data = distSession->getObjDistributionData();
    if (object_distribution_data.has_value()) {
        std::shared_ptr<ObjDistributionData> objectDistributionDataPtr = object_distribution_data.value();
        const std::optional<std::string> &baseUrl = ingestBaseUrl.empty() ? std::nullopt : std::optional<std::string>(ingestBaseUrl);
        objectDistributionDataPtr->setObjIngestBaseUrl(baseUrl);
    } else {
        ogs_error("ObjectDistributionData is not available");
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
