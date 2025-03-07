/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSTF Object store
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <memory>
#include <stdexcept>
#include <utility>
#include <chrono>
#include <thread>
#include <mutex>

#include "common.hh"
#include "SubscriptionService.hh"
#include "Event.hh"
#include "ObjectStore.hh"

MBSTF_NAMESPACE_START

ObjectStore::Metadata::Metadata()
    :m_cacheExpires(std::nullopt)
    ,m_created(std::chrono::system_clock::now()) 
    ,m_modified(std::chrono::system_clock::now())
{

}

ObjectStore::Metadata::Metadata(const std::string &media_type, const std::string &url, const std::string &fetched_url,
                 const std::string &acquisition_id,
                 const std::chrono::system_clock::time_point last_modified,
                 std::optional<std::string> obj_ingest_base_url,
                 std::optional<std::string> obj_distribution_base_url,
                 const std::optional<std::chrono::system_clock::time_point> &cache_expires)
    :m_mediaType(media_type)
    ,m_originalUrl(url)
    ,m_fetchedUrl(fetched_url)
    ,m_acquisitionId(acquisition_id)
    ,m_objIngestBaseUrl(obj_ingest_base_url)
    ,m_objDistributionBaseUrl(obj_distribution_base_url)
    ,m_cacheExpires(cache_expires)
    ,m_created(std::chrono::system_clock::now())
    ,m_modified(last_modified)
{

}

ObjectStore::Metadata::Metadata(const Metadata &other)
    :m_mediaType(other.m_mediaType)
    ,m_originalUrl(other.m_originalUrl)
    ,m_fetchedUrl(other.m_fetchedUrl)
    ,m_acquisitionId(other.m_acquisitionId)
    ,m_objIngestBaseUrl(other.m_objIngestBaseUrl)
    ,m_objDistributionBaseUrl(other.m_objDistributionBaseUrl)
    ,m_cacheExpires(other.m_cacheExpires)
    ,m_created(other.m_created)
    ,m_modified(other.m_modified)
{

}

ObjectStore::Metadata::Metadata(Metadata &&other)
    :m_mediaType(std::move(other.m_mediaType))
    ,m_originalUrl(std::move(other.m_originalUrl))
    ,m_fetchedUrl(std::move(other.m_fetchedUrl))
    ,m_acquisitionId(std::move(other.m_acquisitionId))
    ,m_objIngestBaseUrl(std::move(other.m_objIngestBaseUrl))
    ,m_objDistributionBaseUrl(std::move(other.m_objDistributionBaseUrl))
    ,m_cacheExpires(std::move(other.m_cacheExpires))
    ,m_created(other.m_created)
    ,m_modified(other.m_modified)
{

}


ObjectStore::ObjectStore(ObjectController &controller)
    :SubscriptionService()
    ,m_controller(controller)
{

}

ObjectStore::~ObjectStore() 
{

}

void ObjectStore::addObject(const std::string& object_id, ObjectData &&object, Metadata &&metadata) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_store.emplace(object_id, std::make_pair(std::move(object), std::move(metadata)));
    std::shared_ptr<Event> event(new ObjectStore::ObjectAddedEvent(object_id));
    sendEventAsynchronous(event);
}

const ObjectStore::ObjectData& ObjectStore::getObjectData(const std::string& object_id) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_store.at(object_id).first;
}

ObjectStore::ObjectData& ObjectStore::getObjectData(const std::string& object_id) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_store.at(object_id).first;
}

const ObjectStore::Metadata& ObjectStore::getMetadata(const std::string& object_id) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_store.at(object_id).second;
}

void ObjectStore::deleteObject(const std::string& object_id) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_store.erase(object_id);
    std::shared_ptr<Event> event(new ObjectStore::ObjectDeletedEvent(object_id));
    sendEventAsynchronous(event);
}

const ObjectStore::Object& ObjectStore::operator[](const std::string& object_id) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_store.at(object_id);
}

/*
bool ObjectStore::hasExpired(const std::string& object_id) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const auto& metadata = m_store.at(object_id).second;
    if (metadata.cacheExpires().has_value()) {
        return std::chrono::system_clock::now() > metadata.cacheExpires().value();
    }
    return false;
}
*/

bool ObjectStore::isStale(const std::string& object_id) const {
    auto it = m_store.find(object_id);
    if (it == m_store.end()) {
        return false;
    }

    const Metadata& metadata = it->second.second;
    return metadata.cacheExpires().has_value() && metadata.cacheExpires().value() < std::chrono::system_clock::now();
}

std::map<std::string, const ObjectStore::Object&> ObjectStore::getStale() const {
    std::map<std::string, const ObjectStore::Object&> staleObjects;

    for (const auto& pair : m_store) {
        const std::string& object_id = pair.first;
        if (isStale(object_id)) {
            staleObjects.emplace(object_id, pair.second);
        }
    }

    return staleObjects;
}

bool ObjectStore::removeObject(const std::string& objectId) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    auto it = m_store.find(objectId);
    if (it != m_store.end()) {
        m_store.erase(it);
	return true;
    } else {
        return false;
    }
}

bool ObjectStore::removeObjects(const std::list<std::string>& objectIds) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    for (const auto& objectId : objectIds) {
        auto it = m_store.find(objectId);
        if (it != m_store.end()) {
            m_store.erase(it);
        } /*else {
            return false;		
            // Optionally handle the case where the objectId is not found
        } */
    }
    return true;
}


MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
