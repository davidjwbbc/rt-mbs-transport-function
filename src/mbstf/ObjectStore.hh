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

#include "ogs-app.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include <memory>
#include <utility>
#include <chrono>
#include <optional>
#include <vector>
#include <map>
#include <mutex>

#include "common.hh"

MBSTF_NAMESPACE_START

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
        virtual ~Metadata() {};

        const std::string &mediaType() const {return m_mediaType};
        Metadata &mediaType(const std::string &media_type) {m_mediaType = media_type; return *this;};
        Metadata &mediaType(std::string &&media_type) {m_mediaType = std::move(media_type); return *this;};

    private:
        std::string m_mediaType;
        std::string m_originalUrl;
        std::string m_fetchedUrl;
        std::optional<std::chrono::system_clock::time_point> m_cacheExpires;
	std::chrono::system_clock::time_point m_created;
	std::chrono::system_clock::time_point m_modified;
    };

    using ObjectData = std::vector<unsigned char>;
    using Object = std::pair<ObjectData, Metadata>;

    ObjectStore();
    ~ObjectStore();
    void addObject(const std::string& object_id, ObjectData &&object, Metadata &&metadata);
    const ObjectData& getObjectData(const std::string& object_id) const;
    const Metadata& getMetadata(const std::string& object_id) const;
    void deleteObject(const std::string& object_id);

    const Object &operator[](const std::string& object_id) const;
    using getObject = operator[];

    bool hasExpired(const std::string& object_id) const;
	
private:
    std::recursive_mutex m_mutex;
    std::map<std::string, Object> m_store;
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_MBSTF_OBJECT_STORE_HH_ */
