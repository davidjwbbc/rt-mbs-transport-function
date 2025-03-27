/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Pull Object Ingester
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
 * Author(s): Dev Audsin <dev.audsin@bbc.co.uk>
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
#include <iostream>
#include <string>

#include "common.hh"
#include "App.hh"
#include "PullObjectIngester.hh"
#include "hash.hh"
#include "Curl.hh"
#include "ObjectStore.hh"

MBSTF_NAMESPACE_START

PullObjectIngester::IngestItem::IngestItem(const std::string &object_id, const std::string &url, const std::string &acquisition_id, const std::optional<std::string> &obj_ingest_base_url,  const std::optional<std::string> &obj_distribution_base_url, const std::optional<time_type> &download_deadline)
    :m_objectId(object_id)
    ,m_url(url)
    ,m_acquisitionId(acquisition_id)
    ,m_objIngestBaseUrl(obj_ingest_base_url)
    ,m_objDistributionBaseUrl(obj_distribution_base_url)
    ,m_deadline(download_deadline)
{
}

PullObjectIngester::IngestItem::IngestItem(const IngestItem &other)
    :m_objectId(other.m_objectId)
    ,m_url(other.m_url)
    ,m_acquisitionId(other.m_acquisitionId)
    ,m_objIngestBaseUrl(other.m_objIngestBaseUrl)
    ,m_objDistributionBaseUrl(other.m_objDistributionBaseUrl)
    ,m_deadline(other.m_deadline)
{
}

PullObjectIngester::IngestItem::IngestItem(IngestItem &&other)
    :m_objectId(std::move(other.m_objectId))
    ,m_url(std::move(other.m_url))
    ,m_acquisitionId(std::move(other.m_acquisitionId))
    ,m_objIngestBaseUrl(std::move(other.m_objIngestBaseUrl))
    ,m_objDistributionBaseUrl(std::move(other.m_objDistributionBaseUrl))
    ,m_deadline(std::move(other.m_deadline)) 
{
}

PullObjectIngester::~PullObjectIngester() {abort();}

bool PullObjectIngester::fetch(const std::string &object_id, const std::optional<time_type> &download_deadline)
{
    for (auto it = m_fetchList.begin(); it != m_fetchList.end(); ++it) {
        if (it->objectId() == object_id) {
            if (download_deadline.has_value()) {
                it->deadline(download_deadline.value());
            }

            sortListByPolicy();
            return true;
        }
    }

    return false;
}

bool PullObjectIngester::fetch(const IngestItem &item) {
    m_fetchList.push_back(item);
    sortListByPolicy();
    return true;
}

bool PullObjectIngester::fetch(IngestItem &&item) {
    m_fetchList.push_back(std::move(item));
    sortListByPolicy();
    return true;
}

void PullObjectIngester::sortListByPolicy() {
    m_fetchList.sort([](const IngestItem &a, const IngestItem &b) {
        if (a.deadline().has_value() && b.deadline().has_value()) {
            return a.deadline() < b.deadline();
        }
        return a.deadline().has_value();
    });
}

void PullObjectIngester::doObjectIngest() {
    if(!m_curl) m_curl = std::make_shared<Curl>();
    std::chrono::seconds timeout(10); // 10 seconds timeout
    if (!m_fetchList.empty()) {
        // Make the GET request and get the number of bytes received
        auto item = m_fetchList.front();
	m_fetchList.pop_front();
        ogs_debug("Fetching %s...", item.url().c_str());
	long bytesReceived = m_curl->get(item.url(), timeout);

        // Check the result
        if (bytesReceived >= 0) {
            ogs_debug("Received %ld bytes of data", bytesReceived);
	    auto lastModified = std::chrono::system_clock::now();
            std::string fetched_url = m_curl->getPermanentRedirectUrl();
            if (fetched_url.empty()) fetched_url = item.url();
	    ObjectStore::Metadata metadata(m_curl->getContentType(), item.url(), fetched_url, item.acquisitionId(), lastModified, item.objIngestBaseUrl(), item.objDistributionBaseUrl());
            metadata.cacheExpires(std::chrono::system_clock::now() + std::chrono::minutes(ObjectStore::Metadata::cacheExpiry()));
            const std::string& etag = m_curl->getEtag();
	    if (!etag.empty()) {
                metadata.entityTag(etag);
	    }
	    this->objectStore().addObject(item.objectId(), std::move(m_curl->getData()), std::move(metadata));
	    
        } else if (bytesReceived == -1) {
            ogs_error("Request timed out.");
            // emitObjectIngestFailedEvent();
        } else {
            ogs_error("An error occurred while fetching the data."); 	     
            // emitObjectIngestFailedEvent();
        }
    }
}

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
