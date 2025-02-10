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
#include <iostream>
#include <string>

#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "PullObjectIngester.hh"
#include "Open5GSSBIClient.hh"
#include "Open5GSSBIResponse.hh"
#include "Open5GSEvent.hh"
#include "hash.hh"
#include "ObjectStore.hh"

MBSTF_NAMESPACE_START

class ObjectStore;

PullObjectIngester::IngestItem::IngestItem(const std::string &object_id, const std::string &url, const std::optional<time_type> &download_deadline)
    :m_objectId(object_id)
    ,m_url(url)
    ,m_deadline(download_deadline)
{

}

PullObjectIngester::IngestItem::IngestItem(const IngestItem &other)
    :m_objectId(other.m_objectId)
    ,m_url(other.m_url)
    ,m_deadline(other.m_deadline)
{

}

PullObjectIngester::IngestItem::IngestItem(IngestItem &&other)
    :m_objectId(std::move(other.m_objectId))
    ,m_url(std::move(other.m_url))
    ,m_deadline(std::move(other.m_deadline)) 
{

}


PullObjectIngester::~PullObjectIngester() {}

bool PullObjectIngester::add(const std::string &object_id, const std::string &url, const time_type &download_deadline) {
    IngestItem newItem(object_id, url, download_deadline);
    m_fetchList.push_back(std::move(newItem));
    sortListIntoPriorityOrder();
    return true;
}

bool PullObjectIngester::add(const IngestItem &item) {
    m_fetchList.push_back(item);
    sortListIntoPriorityOrder();
    return true;
}

bool PullObjectIngester::add(IngestItem &&item) {
    m_fetchList.push_back(std::move(item));
    sortListIntoPriorityOrder();
    return true;
}

void PullObjectIngester::sortListIntoPriorityOrder() {
    m_fetchList.sort([](const IngestItem &a, const IngestItem &b) {
        if (a.deadline().has_value() && b.deadline().has_value()) {
            return a.deadline() < b.deadline();
        }
        return a.deadline().has_value();
    });
}

void PullObjectIngester::doObjectIngest() {
    const std::string method = "GET";
    const std::string apiVersion = "v1";
	
    for (auto& item : m_fetchList) {
        // Send request to the URL
        std::string url = item.url();
	/*std::shared_ptr<Open5GSSBIClient> */ item.client() = std::make_shared<Open5GSSBIClient>(url);
	std::shared_ptr<Open5GSSBIRequest> request = std::make_shared<Open5GSSBIRequest>(method, url, apiVersion, std::nullopt, std::nullopt);
        
        auto *requestContext = new std::pair<IngestItem&, PullObjectIngester&>(item, *this);
        void *data = reinterpret_cast<void*>(requestContext);
        

        //const void *data = reinterpret_cast<const void*>(&item);
        bool rv = item.client()->sendRequest(client_notify_cb, request, (void *)data);
        if(!rv) ogs_error("Error while sending Request to %s", url.c_str());
	//return rv;


        // Implementation to send request to the URL goes here
        std::cout << "Sending request to URL: " << url << std::endl;
    }
    m_fetchList.clear();
}

/*
int PullObjectIngester::client_notify_cb(int status, ogs_sbi_response_t *response, void *data)
{
    return 0;	
}
*/

int PullObjectIngester::client_notify_cb(int status, ogs_sbi_response_t *response, void *data) 
{
    ogs_info("In client_notify_cb");

    //int rv;
    const char *content;
    const char *contentType;
    size_t contentLength;

    if (status != OGS_OK) {
        ogs_log_message(
                status == OGS_DONE ? OGS_LOG_DEBUG : OGS_LOG_WARN, 0,
                "client_notify_cb() failed [%d]", status);
        if (response) ogs_sbi_response_free(response);
        return OGS_ERROR;
    }

    Open5GSSBIResponse resp(response);
    
    contentLength = resp.contentLength(); 
    if(contentLength) {
        content =  resp.content();
    }

    if (!content) {
        ogs_error("No content in response");
        if (response) ogs_sbi_response_free(response);
        return OGS_ERROR;
    }

    Open5GSSBIMessage message;

    try {
        message.parseHeader(resp);
    } catch (std::exception &ex) {
        ogs_error("Parsing response header failed");
	if (response) ogs_sbi_response_free(response);
        return OGS_ERROR;
    }
    contentType = message.contentType();
    auto* retrievedContext = reinterpret_cast<std::pair<IngestItem&, PullObjectIngester&>*>(data);
    //auto *ingestItem = reinterpret_cast<const IngestItem*>(data);
    
    IngestItem& ingestItem = retrievedContext->first;
    PullObjectIngester& pullObjectIngester = retrievedContext->second;
    
    std::vector<unsigned char> objectData = convertToVector(std::string(content)); 
    
    auto lastModified = std::chrono::system_clock::now();
    /*
    std::optional<std::chrono::system_clock::time_point> cacheExpires = std::chrono::system_clock::now() + std::chrono::minutes(ObjectStore::Metadata::cacheExpiry());
    ObjectStore::Metadata metadata(std::string(contentType), ingestItem.url(), ingestItem.url(), lastModified, cacheExpires);
    */

    ObjectStore::Metadata metadata(std::string(contentType), ingestItem.url(), ingestItem.url(), lastModified);
    metadata.cacheExpires(std::chrono::system_clock::now() + std::chrono::minutes(ObjectStore::Metadata::cacheExpiry()));
    const char *etag = resp.getHeader("ETag");
    if(etag) metadata.entityTag(etag);
    pullObjectIngester.objectStore().addObject(ingestItem.objectId(), std::move(objectData), std::move(metadata));
    
    return OGS_OK;    
}

// Utility function to convert std::string to std::vector<unsigned char>
std::vector<unsigned char> PullObjectIngester::convertToVector(const std::string &str) {
    return std::vector<unsigned char>(str.begin(), str.end());
}


MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
