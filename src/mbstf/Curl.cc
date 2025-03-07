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
#include "Curl.hh"
#include <iostream>

MBSTF_NAMESPACE_START

Curl::Curl() {
    // Ensure curl_global_init is called only once
    static std::once_flag init_flag;
    std::call_once(init_flag, []() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    });

    // Initialize the CURL handle
    m_curl = curl_easy_init();
}

Curl::~Curl() {
    if (m_curl) {
        curl_easy_cleanup(m_curl);
    }
}

long Curl::get(const std::string& url, std::chrono::milliseconds timeout) {
    m_etag.clear(); // Clear the ETag before making a new request
    m_receivedData.clear(); // Clear the received data before making a new request

    if (m_curl) {
        curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT_MS, 500l);
        curl_easy_setopt(m_curl, CURLOPT_TIMEOUT_MS, timeout.count());
        curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1L);
	//curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, this);
        //curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, headerCallback);
        curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &m_receivedData);
        curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, writeCallback);

        CURLcode res = curl_easy_perform(m_curl);
        if (res == CURLE_OK) {
	    struct curl_header *type;
            CURLHcode h;	    
            h = curl_easy_header(m_curl, "ETag", 0, CURLH_HEADER, -1, &type);
            if (h == CURLHE_OK && type) {
                m_etag = std::string(type->value);
		ogs_info("ETag: %s", m_etag.c_str());
            } else {
		ogs_info("ETag header not found.");    
            }

            // Get the Content-Type header
	    char *ct = NULL;
	    res = curl_easy_getinfo(m_curl, CURLINFO_CONTENT_TYPE, &ct);
	    if (!res && ct) { 
	        m_contentType = std::string(ct);
	    }

            char *redir_url = NULL;
            res = curl_easy_getinfo(m_curl, CURLINFO_EFFECTIVE_URL, &redir_url);
            if (!res && redir_url) {
                m_effectiveUrl = redir_url;
            }

            return m_receivedData.size(); // Return the number of bytes received
        } else if (res == CURLE_OPERATION_TIMEDOUT) {
            return -1; // Indicate timeout
        } else {
            //std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            return -2; // Indicate other error
        }
    }
    return -2; // Indicate error if m_curl is not initialized
}

std::vector<unsigned char>& Curl::getData()
{
    return m_receivedData;
}

const std::vector<unsigned char>& Curl::getData() const
{
    return m_receivedData;
}

const std::string& Curl::getEtag() const
{
    return m_etag;
}

const std::string& Curl::getContentType() const
{
    return m_contentType;
}

const std::string &Curl::getEffectiveUrl() const
{
    return m_effectiveUrl;
}

/*
size_t Curl::headerCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
    size_t totalSize = size * nitems;
    Curl* self = static_cast<Curl*>(userdata);

    std::string header(buffer, totalSize);
    std::string etagPrefix = "ETag: ";
    std::string contentTypePrefix = "Content-Type: ";

    if (header.compare(0, etagPrefix.length(), etagPrefix) == 0) {
        std::string etagValue = header.substr(etagPrefix.length());
        etagValue.erase(etagValue.find_last_not_of("\r\n") + 1); // Remove newline characters
        self->m_etag = etagValue;
    } else if (header.compare(0, contentTypePrefix.length(), contentTypePrefix) == 0) {
        std::string contentTypeValue = header.substr(contentTypePrefix.length());
        contentTypeValue.erase(contentTypeValue.find_last_not_of("\r\n") + 1); // Remove newline characters
        self->m_contentType = contentTypeValue;
    }

    return totalSize;
}
*/


size_t Curl::writeCallback(void* contents, size_t memberSize, size_t numberOfMembers, void* userData)
{
    size_t totalSize = memberSize * numberOfMembers;
    std::vector<unsigned char>* receivedData = static_cast<std::vector<unsigned char>*>(userData);
    unsigned char* data = static_cast<unsigned char*>(contents);
    receivedData->insert(receivedData->end(), data, data + totalSize);
    return totalSize;
}

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */ 

