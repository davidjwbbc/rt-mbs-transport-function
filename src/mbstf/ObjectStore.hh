#ifndef _MBS_TF_MBSTF_OBJECT_STORE_HH_
#define _MBS_TF_MBSTF_OBJECT_STORE_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSTF Object store class
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#pragma once

#include <memory>
#include <utility>
#include <chrono>
#include <optional>
#include <vector>
#include <map>
#include <mutex>
#include <list>
#include <functional>
#include <thread>
#include "common.hh"

MBSTF_NAMESPACE_START

class ObjectController;

#define CACHE_EXPIRES 10
#define CHECK_EXPIRY_INTERVAL 10

class ObjectStore {
public:
    class Metadata {
    public:
        Metadata();
        Metadata(const std::string &media_type, const std::string &url, const std::string &fetched_url,
                 const std::chrono::system_clock::time_point last_modified,
                 const std::optional<std::chrono::system_clock::time_point> &cache_expires = std::nullopt);
        Metadata(const Metadata &other);
        Metadata(Metadata &&other);
	Metadata& operator=(Metadata&& other);
        virtual ~Metadata() {};

        const std::string &mediaType() const {return m_mediaType;};
        Metadata &mediaType(const std::string &media_type) {m_mediaType = media_type; return *this;};
        Metadata &mediaType(std::string &&media_type) {m_mediaType = std::move(media_type); return *this;};
	const std::optional<std::chrono::system_clock::time_point>& cacheExpires() const { return m_cacheExpires;};
	std::optional<std::chrono::system_clock::time_point>& cacheExpires(std::chrono::system_clock::time_point cacheExpires) { m_cacheExpires = cacheExpires; return m_cacheExpires;};
        static int cacheExpiry()  {return CACHE_EXPIRES;};
        static int cacheExpiryInterval()  {return CHECK_EXPIRY_INTERVAL;};

	const std::optional<std::string> &entityTag() const { return m_entityTag;};

	bool hasEntityTag() {return m_entityTag.has_value();};

	Metadata &entityTag(const std::optional<std::string>& entityTag) {m_entityTag = entityTag; return *this;};

    private:
        std::string m_mediaType;
        std::string m_originalUrl;
        std::string m_fetchedUrl;
	std::optional<std::string> m_entityTag;
        std::optional<std::chrono::system_clock::time_point> m_cacheExpires;
	std::chrono::system_clock::time_point m_created;
	std::chrono::system_clock::time_point m_modified;
    };

    using ObjectData = std::vector<unsigned char>;
    using Object = std::pair<ObjectData, Metadata>;

    ObjectStore() = delete;
    ObjectStore(ObjectController &controller);
    ~ObjectStore();
    void addObject(const std::string& object_id, ObjectData &&object, Metadata &&metadata);
    const ObjectData& getObjectData(const std::string& object_id) const;
    const Metadata& getMetadata(const std::string& object_id) const;
    void deleteObject(const std::string& object_id);
    bool removeObject(const std::string& objectId);
    bool removeObjects(const std::list<std::string>& objectIds);
    std::list<std::pair<const std::string*, const std::pair<std::vector<unsigned char>, ObjectStore::Metadata>*>> getExpired();
    const Object &operator[](const std::string& object_id) const;
    
    bool isStale(const std::string& object_id) const;
    std::map<std::string, const Object&> getStale() const;

	
private:
    void checkExpiredObjects();	
    mutable std::recursive_mutex m_mutex;
    ObjectController &m_controller;
    std::map<std::string, Object> m_store;
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_MBSTF_OBJECT_STORE_HH_ */
