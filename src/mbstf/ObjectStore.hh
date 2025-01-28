#ifndef _MBS_TF_MBSTF_OBJECT_STORE_HH_
#define _MBS_TF_MBSTF_OBJECT_STORE_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSTF Network Function class
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
#include <vector>
#include <map>
#include <mutex>

#include "common.hh"

MBSTF_NAMESPACE_START

class ObjectStore {
public:
    struct Metadata {
        std::string mediaType;
	std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> created;
	std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> modified;
    };

    using ObjectData = std::vector<unsigned char>;
    using Object = std::pair<ObjectData, Metadata>;

    ObjectStore();
    ~ObjectStore();
    void addObject(const std::string& object_id, ObjectData &&object,const std::string& mediaType);
    const ObjectData& getObject(const std::string& object_id) const;
    const Metadata& getMetadata(const std::string& object_id) const;
    void deleteObject(const std::string& object_id);
	
private:
    Metadata populateMetadata(const std::string& mediaType);
    mutable std::recursive_mutex m_mutex;
    std::map<std::string, Object> m_store;
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_MBSTF_OBJECT_STORE_HH_ */
