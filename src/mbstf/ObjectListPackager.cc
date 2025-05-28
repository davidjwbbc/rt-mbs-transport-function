/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: ObjectListPackager class
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
 * Author(s): Dev Audsin <dev.audsin@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <chrono>
#include <exception>
#include <iostream>
#include <list>
#include <memory>
#include <optional>
#include <string>

#include <netinet/in.h>

#include "ogs-app.h" // ogs_error(), ogs_info()
#include "Transmitter.h" // LibFlute

#include "common.hh"
#include "ObjectController.hh"
#include "ObjectListController.hh"
#include "ObjectPackager.hh"
#include "ObjectStore.hh"

#include "ObjectListPackager.hh"

MBSTF_NAMESPACE_START

// ObjectListPackager::PackageItem

ObjectListPackager::PackageItem::PackageItem(const std::string &object_id, const std::optional<time_type> &deadline)
    :m_objectId(object_id)
    ,m_deadline(deadline)
{
}

ObjectListPackager::PackageItem::PackageItem(const PackageItem &other)
    :m_objectId(other.m_objectId)
    ,m_deadline(other.m_deadline)
{
}

ObjectListPackager::PackageItem::PackageItem(PackageItem &&other)
    :m_objectId(std::move(other.m_objectId))
    ,m_deadline(std::move(other.m_deadline))
{
}

// ObjectListPackager

ObjectListPackager::ObjectListPackager(ObjectStore &object_store, ObjectController &controller,
                                       const std::list<PackageItem> &object_to_package,
                                       const std::optional<std::string> &address,
                                       uint32_t rateLimit, unsigned short mtu, in_port_t port, const std::optional<std::string> &tunnel_address, in_port_t tunnel_port)
    :ObjectPackager(object_store, controller, address, rateLimit, mtu, port, tunnel_address, tunnel_port)
    ,m_packageItems(object_to_package)
    ,m_tunnelEndpoint()
    ,m_packageItemsMutex (new std::recursive_mutex)
{
    sortListByPolicy();
    if (tunnel_address) {
        m_tunnelEndpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(tunnel_address.value()), tunnel_port);
    }
    startWorker();
}

ObjectListPackager::ObjectListPackager(ObjectStore &object_store, ObjectController &controller,
                                       std::list<PackageItem> &&object_to_package, const std::optional<std::string> &address,
                                       uint32_t rateLimit, unsigned short mtu, in_port_t port, const std::optional<std::string> &tunnel_address, in_port_t tunnel_port)
    :ObjectPackager(object_store, controller, address, rateLimit, mtu, port, tunnel_address, tunnel_port)
    ,m_packageItems(std::move(object_to_package))
    ,m_tunnelEndpoint()
    ,m_packageItemsMutex (new std::recursive_mutex)
{
    sortListByPolicy();
    if (tunnel_address) {
        m_tunnelEndpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(tunnel_address.value()), tunnel_port);
    }
    startWorker();
}

ObjectListPackager::ObjectListPackager(ObjectStore &object_store, ObjectController &controller,
                                       const std::optional<std::string> &address, uint32_t rateLimit, unsigned short mtu,
                                       in_port_t port, const std::optional<std::string> &tunnel_address, in_port_t tunnel_port)
    :ObjectPackager(object_store, controller, address, rateLimit, mtu, port, tunnel_address, tunnel_port)
    ,m_packageItems()
    ,m_tunnelEndpoint()
    ,m_packageItemsMutex (new std::recursive_mutex)
{
    if (tunnel_address) {
        m_tunnelEndpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(tunnel_address.value()), tunnel_port);
    }
    startWorker();
}

ObjectListPackager::~ObjectListPackager() {
    abort();
    if (m_transmitter) delete m_transmitter;
}

bool ObjectListPackager::add(const PackageItem &item) {
    std::lock_guard<std::recursive_mutex> lock(*m_packageItemsMutex);
    m_packageItems.push_back(item);
    sortListByPolicy();
    return true;
}

bool ObjectListPackager::add(PackageItem &&item) {
    std::lock_guard<std::recursive_mutex> lock(*m_packageItemsMutex);
    m_packageItems.push_back(std::move(item));
    sortListByPolicy();
    return true;
}

void ObjectListPackager::doObjectPackage() {
    try {
        std::optional<std::string> destAddr = destIpAddr();

        if (destAddr)
        {
            if (!m_transmitter) {
                m_transmitter = new LibFlute::Transmitter(
                    destAddr.value(),
                    static_cast<short>(port()),
                    0,
                    mtu(),
                    rateLimit(),
                    m_io,
                    m_tunnelEndpoint);
                m_transmitter->register_completion_callback(
                    [this](uint32_t toi) {
                        if (m_queuedToi == toi) {
			    
                            m_queued = false;
			    objectSendCompletion(m_queuedObjectId);
                            /*
			    std::shared_ptr<Event> event(new ObjectListPackager::ObjectSendCompleted(m_queuedObjectId));
                            sendEventAsynchronous(event);
                            */ 
			    //objectStore().deleteObject(m_queuedObjectId);
                            ogs_info("Transmitted: Object with TOI: %d", toi);
                        } else {
                            ogs_error("Unscheduled completion of Object with TOI: %d", toi);
                        }

                    }


                );

                // emitFluteSessionStartedEvent();
            }
            {
                std::lock_guard<std::recursive_mutex> lock(*m_packageItemsMutex);

                if (!m_packageItems.empty() && !m_queued) {
		    
                    auto &item = m_packageItems.front();
	            m_packageItemsMutex->unlock();
                    std::string location;
		    m_queuedObjectId = item.objectId();
                    std::vector<unsigned char> &objData = objectStore().getObjectData(item.objectId());
                    const ObjectStore::Metadata &metadata = objectStore().getMetadata(item.objectId());
                    std::string obj_ingest_base_url = metadata.objIngestBaseUrl().value_or(std::string());
                    std::string obj_distribution_base_url = metadata.objDistributionBaseUrl().value_or(std::string());

                    // If we need to substitute objIngestBaseUrl for objDistributionBaseUrl then do so
                    if (!obj_ingest_base_url.empty() && !obj_distribution_base_url.empty() &&
                        metadata.getFetchedUrl().starts_with(obj_ingest_base_url)) {
                        location = obj_distribution_base_url + metadata.getFetchedUrl().substr(obj_ingest_base_url.size());
                    } else {
                        // Just use the fetched URL
                        location = metadata.getFetchedUrl();
                    }

                    m_queued = true;
                    uint64_t expires_in;
                    const auto &cache_expires = metadata.cacheExpires();
                    if (cache_expires) {
                        expires_in = std::chrono::duration_cast<std::chrono::seconds>(cache_expires.value().time_since_epoch()).count() + 2208988800;
                    } else {
                        expires_in = m_transmitter->seconds_since_epoch() + 60;
                    }
                    m_queuedToi = m_transmitter->send(location, metadata.mediaType(),
                            expires_in,
                            reinterpret_cast<char*>(objData.data()),
                            objData.size()
                    );
                    m_packageItemsMutex->lock();
                    m_packageItems.pop_front();
                }
	    }

            m_io.run_one();
        }
    } catch (std::exception &ex) {
        ogs_error("Exiting on unhandled exception: %s", ex.what());
        // emitFluteSessionFailedEvent();
    }
}

void ObjectListPackager::sortListByPolicy() {
    m_packageItems.sort([](const PackageItem &a, const PackageItem &b) {
        if (a.deadline().has_value() && b.deadline().has_value()) {
            return a.deadline() < b.deadline();
        }
        return a.deadline().has_value();
    });
}

void ObjectListPackager::objectSendCompletion(std::string &object_id)
{
    std::shared_ptr<Event> event(new ObjectListPackager::ObjectSendCompleted(object_id));
    sendEventAsynchronous(event);
}

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
