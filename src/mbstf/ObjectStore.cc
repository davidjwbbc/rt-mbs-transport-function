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

#include "ogs-app.h"
#include "ogs-sbi.h"

#include <memory>
#include <stdexcept>
#include <utility>
#include <chrono>
#include <thread>
#include <mutex>

#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "ObjectStore.hh"
#include "hash.hh"

MBSTF_NAMESPACE_START

ObjectStore::Metadata::Metadata()
    :m_cacheExpires(std::nullopt)
    ,m_created(std::chrono::system_clock::now()) 
    ,m_modified(std::chrono::system_clock::now())
{

}

ObjectStore::Metadata::Metadata(const std::string &media_type, const std::string &url, const std::string &fetched_url,
                                const std::chrono::system_clock::time_point last_modified,
                                const std::optional<std::chrono::system_clock::time_point> &cache_expires)
    :m_mediaType(media_type) 
    ,m_originalUrl(url)
    ,m_fetchedUrl(fetched_url) 
    ,m_cacheExpires(cache_expires)
    ,m_created(std::chrono::system_clock::now())
    ,m_modified(last_modified)
{

}

ObjectStore::Metadata::Metadata(const Metadata &other)
    :m_mediaType(other.m_mediaType)
    ,m_originalUrl(other.m_originalUrl)
    ,m_fetchedUrl(other.m_fetchedUrl)
    ,m_cacheExpires(other.m_cacheExpires)
    ,m_created(other.m_created)
    ,m_modified(other.m_modified)
{

}

ObjectStore::Metadata::Metadata(Metadata &&other)
    :m_mediaType(std::move(other.m_mediaType))
    ,m_originalUrl(std::move(other.m_originalUrl))
    ,m_fetchedUrl(std::move(other.m_fetchedUrl))
    ,m_cacheExpires(std::move(other.m_cacheExpires))
    ,m_created(other.m_created)
    ,m_modified(other.m_modified) 
{

}

//ObjectStore::ObjectStore() {}

ObjectStore::ObjectStore(DistributionSession &distributionSession, ObjectController &controller)
    :m_distributionSession(distributionSession) 
    ,m_controller(controller)
{
    startCheckingExpiredObjects();	

}

ObjectStore::~ObjectStore() 
{
    if (m_checkExpiryThread.joinable()) {
        m_checkExpiryThread.join();
    }

}

void ObjectStore::addObject(const std::string& object_id, ObjectData &&object, Metadata &&metadata) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_store.emplace(object_id, std::make_pair(std::move(object), std::move(metadata)));
    //m_store[object_id] = std::make_pair(std::move(object), std::move(metadata));
}

const ObjectStore::ObjectData& ObjectStore::getObjectData(const std::string& object_id) const {
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
}

const ObjectStore::Object& ObjectStore::operator[](const std::string& object_id) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_store.at(object_id);
}

bool ObjectStore::hasExpired(const std::string& object_id) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const auto& metadata = m_store.at(object_id).second;
    if (metadata.cacheExpires().has_value()) {
        return std::chrono::system_clock::now() > metadata.cacheExpires().value();
    }
    return false;
}

void ObjectStore::setExpiredCallback(ExpiredCallback &&callback) {
    m_expiredCallback = std::move(callback);
}

void ObjectStore::startCheckingExpiredObjects() {
    m_checkExpiryThread = std::thread([this]() {
	checkExpiredObjects();
        std::this_thread::sleep_for(std::chrono::minutes(ObjectStore::Metadata::cacheExpiryInterval())); 
    });
}

std::list<std::pair<const std::string*, const std::pair<std::vector<unsigned char>, ObjectStore::Metadata>*>> ObjectStore::getExpired() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::list<std::pair<const std::string*, const std::pair<std::vector<unsigned char>, ObjectStore::Metadata>*>> expiredList;

    for (auto obj = m_store.begin(); obj != m_store.end(); ++obj) {
        if (hasExpired(obj->first)) {
            // Add pointers to the expired object to the list
            expiredList.emplace_back(&obj->first, &obj->second);
        }
    }

    return expiredList;
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




void ObjectStore::checkExpiredObjects() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    for (auto obj = m_store.begin(); obj != m_store.end();) {
        if (hasExpired(obj->first)) {
            if (m_expiredCallback) {
                m_expiredCallback(obj->first, obj->second);
            }
            //it = m_store.erase(obj); // Remove expired object
        } /*else {
            ++obj;
        }
	*/
    }
}



MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
