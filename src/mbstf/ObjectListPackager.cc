/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSTF ObjectListPackager
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

#include <boost/asio/io_service.hpp>

#include "ogs-app.h" // LibFlute
#include "Transmitter.h" // LibFlute

#include "common.hh"
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

ObjectListPackager::ObjectListPackager(ObjectStore &object_store, ObjectListController &controller,
                                       const std::list<PackageItem> &object_to_package,
                                       const std::shared_ptr<std::string> &address,
                                       uint32_t rateLimit, unsigned short mtu, short port)
    :ObjectPackager(object_store, controller, address, rateLimit, mtu, port)
    ,m_packageItems(object_to_package)
{
    sortListByPolicy();
    startWorker();
}

ObjectListPackager::ObjectListPackager(ObjectStore &object_store, ObjectListController &controller,
                                       std::list<PackageItem> &&object_to_package, const std::shared_ptr<std::string> &address,
                                       uint32_t rateLimit, unsigned short mtu, short port)
    :ObjectPackager(object_store, controller, address, rateLimit, mtu, port)
    ,m_packageItems(std::move(object_to_package))
{
    sortListByPolicy();
    startWorker();
}

ObjectListPackager::ObjectListPackager(ObjectStore &object_store, ObjectListController &controller,
                                       const std::shared_ptr<std::string> &address, uint32_t rateLimit, unsigned short mtu,
                                       short port)
    :ObjectPackager(object_store, controller, address, rateLimit, mtu, port)
    ,m_packageItems()
{
    startWorker();
}

ObjectListPackager::~ObjectListPackager() {
    abort();
    if (m_transmitter) delete m_transmitter;
}

bool ObjectListPackager::add(const PackageItem &item) {
    m_packageItems.push_back(item);
    sortListByPolicy();
    return true;
}

bool ObjectListPackager::add(PackageItem &&item) {
    m_packageItems.push_back(std::move(item));
    sortListByPolicy();
    return true;
}

void ObjectListPackager::doObjectPackage() {
    try {
        std::shared_ptr<std::string> destAddr = destIpAddr();

        if (destAddr)
        {
            if (!m_transmitter) {
                m_transmitter = new LibFlute::Transmitter(
                    *destAddr,
                    (short)port(),
                    0,
                    mtu(),
                    rateLimit(),
                    m_io);
                m_transmitter->register_completion_callback(
                    [this](uint32_t toi) {
                        if (m_queuedToi == toi) {
                            m_queued = false;
                            ogs_info("INFO: Object with TOI: %d", toi);
                        }
                    }
                );
            }

            if (!m_packageItems.empty() && !m_queued) {
                auto &item = m_packageItems.front();
                std::vector<unsigned char> &objData = objectStore().getObjectData(item.objectId());
                const ObjectStore::Metadata &metadata = objectStore().getMetadata(item.objectId());
                m_queued = true;
                m_queuedToi = m_transmitter->send( metadata.getOriginalUrl(),
                    "application/octet-stream",
                    m_transmitter->seconds_since_epoch() + 60, // 1 minute from now
                    reinterpret_cast<char*>(objData.data()),
                    objData.size()
                );
                m_packageItems.pop_front();
            }

            m_io.run_one();
        }
    } catch (std::exception &ex) {
	std::ostringstream exceptError;
        exceptError << "Exiting on unhandled exception: " << ex.what();
        ogs_info("%s", exceptError.str().c_str());
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

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
