/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS Application interface
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
#include "MBSTFObjectStore.hh"

MBSTF_NAMESPACE_START

MBSTFObjectStore::MBSTFObjectStore() {}

MBSTFObjectStore::~MBSTFObjectStore() {}

void MBSTFObjectStore::addObject(const std::string& object_id, ObjectData&& object, const std::string& type) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    Metadata metadata = populateMetadata(type);
    m_store[object_id] = std::make_pair(std::move(object), metadata);
}

const MBSTFObjectStore::ObjectData& MBSTFObjectStore::getObject(const std::string& object_id) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_store.at(object_id).first;
}

const MBSTFObjectStore::Metadata& MBSTFObjectStore::getMetadata(const std::string& object_id) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_store.at(object_id).second;
}

void MBSTFObjectStore::deleteObject(const std::string& object_id) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_store.erase(object_id);
}

MBSTFObjectStore::Metadata MBSTFObjectStore::populateMetadata(const std::string& type) {
    //auto now = std::chrono::system_clock::now();
    auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());

    return Metadata{ type, now, now };
}


MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
