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

MBSTF_NAMESPACE_START

PullObjectIngester::PullObjectIngester() {}

PullObjectIngester::~PullObjectIngester() {}

bool PullObjectIngester::addObjectPull(const std::string &object_id, const std::string &url)
{
    //std::shared_ptr<Open5GSSBIClient> client;
    //client.reset(new Open5GSSBIClient(url));
    const std::string method = "GET";
    const std::string apiVersion = "v1";

    Open5GSSBIClient client(url);
    Open5GSSBIRequest request(method, url, apiVersion, std::nullopt, std::nullopt);
    //Open5GSSBIRequest request = createPullObjectIngestorRequest(url);
    //std::optional<std::vector<unsigned char>> optData = std::nullopt;
    std::optional<std::vector<unsigned char>> data = stringToVector(object_id);
    bool rv = client.sendRequest(client_notify_cb, request, data);
    return rv;

}

int PullObjectIngester::client_notify_cb(int status, ogs_sbi_response_t *response, void *data) 
{
    ogs_info("In client_notify_cb");


    if (status != OGS_OK) {
        ogs_log_message(
                status == OGS_DONE ? OGS_LOG_DEBUG : OGS_LOG_WARN, 0,
                "client_notify_cb() failed [%d]", status);
        if (response) ogs_sbi_response_free(response);
        return OGS_ERROR;
    }

    int rv;

    const App &app = App::self();


    std::string *objectIdPtr = reinterpret_cast<std::string*>(data); // Cast void* to std::string*
    std::string objectId = *objectIdPtr;

    Open5GSSBIResponse resp(response);
    std::shared_ptr<Open5GSEvent> event = std::make_shared<Open5GSEvent>(ogs_event_new(OGS_EVENT_SBI_CLIENT));
    //std::shared_ptr<Open5GSEvent> event(ogs_event_new(OGS_EVENT_SBI_CLIENT));
    event->sbiResponse(resp);
    event->setSbiData(data);

    rv = app.queuePush(event);
    if (rv !=OGS_OK) {
        ogs_error("App Event Push failed %d", rv);
        ogs_sbi_response_free(response);
        return OGS_ERROR;
    }

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
