#ifndef _MBS_TF_MBSTF_CURL_HH_
#define _MBS_TF_MBSTF_CURL_HH_
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
    

#include <curl/curl.h>
#include <iostream>
#include <string>
#include <mutex>
#include <chrono>
#include <functional>
#include <vector>
#include "common.hh"

MBSTF_NAMESPACE_START

class Curl {
public:
    Curl();
    ~Curl();

    long get(const std::string& url, std::chrono::milliseconds timeout);
    std::vector<unsigned char> &getData();
    const std::vector<unsigned char> &getData() const;
    const std::string &getEtag() const;
    const std::string &getContentType() const;
    const std::string &getEffectiveUrl() const;

private:
    //static size_t headerCallback(char* buffer, size_t size, size_t numberOfItems, void* userData);
    static size_t writeCallback(void* contents, size_t memberSize, size_t numberOfMembers, void* userData);

    CURL* m_curl;
    std::vector<unsigned char> m_receivedData;
    std::string m_etag;
    std::string m_contentType;
    std::string m_effectiveUrl;
};

class CurlGlobalCleanup {
public:
	
    CurlGlobalCleanup() { };
    ~CurlGlobalCleanup() {
        curl_global_cleanup();
    }
};

// Create a static object to ensure cleanup at program exit
static CurlGlobalCleanup curlGlobalCleanup;

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_MBSTF_CURL_HH_ */

