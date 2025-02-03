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
#include "ObjectStore.hh"

MBSTF_NAMESPACE_START

class ObjectStore;

//PullObjectIngester::PullObjectIngester() {}

PullObjectIngester::PullObjectIngester(ObjectStore& objectStore)
    :m_objectStore(objectStore)
{

}

PullObjectIngester::~PullObjectIngester() {}

bool PullObjectIngester::addObjectPull(const std::string &object_id, const std::string &url)
{
    //std::shared_ptr<Open5GSSBIClient> client;
    //client.reset(new Open5GSSBIClient(url));
    const std::string method = "GET";
    const std::string apiVersion = "v1";

    Open5GSSBIClient client(url);
    Open5GSSBIRequest request(method, url, apiVersion, std::nullopt, std::nullopt);
    /*
    Open5GSSBIRequest request = createPullObjectIngestorRequest(url);
    std::optional<std::vector<unsigned char>> optData = std::nullopt;
    std::optional<std::vector<unsigned char>> data = stringToVector(object_id);
    */
    auto* storage = new std::pair<const std::string&, ObjectStore&>(object_id, this->objectStore());

    void* data = reinterpret_cast<void*>(storage);

    bool rv = client.sendRequest(client_notify_cb, request, data);
    return rv;

}

int PullObjectIngester::client_notify_cb(int status, ogs_sbi_response_t *response, void *data) 
{
    ogs_info("In client_notify_cb");

    int rv;
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

    auto* storage = reinterpret_cast<std::pair<const std::string&, ObjectStore&>*>(data);
    
    const std::string& objectId = storage->first;
    ObjectStore& objectStore = storage->second;
    std::vector<unsigned char> objectData = stringToVector(std::string(content)); 
    objectStore.addObject(objectId, std::move(objectData), std::string(contentType));
    
    return OGS_OK;    
}

// Utility function to convert std::string to std::vector<unsigned char>
std::vector<unsigned char> PullObjectIngester::stringToVector(const std::string &str) {
    return std::vector<unsigned char>(str.begin(), str.end());
}

/*
Open5GSSBIRequest PullObjectIngester::createPullObjectIngestorRequest(const std::string &url)
{
    const std::string method = "GET";
    const std::string apiVersion = "v1";

    Open5GSSBIRequest request(method, url, apiVersion, std::nullopt, std::nullopt);
    return request;
}
*/


MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
