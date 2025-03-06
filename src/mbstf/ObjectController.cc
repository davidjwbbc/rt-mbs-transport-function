/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSTF ObjectController
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <memory>
#include <list>
#include <regex>

#include "common.hh"
#include "Controller.hh"
#include "DistributionSession.hh"
#include "ObjectStore.hh"
#include "PullObjectIngester.hh"

#include "ObjectController.hh"

MBSTF_NAMESPACE_START

std::shared_ptr<PullObjectIngester> &ObjectController::addPullObjectIngester(PullObjectIngester *ingester) {
    // Transfer ownership from unique_ptr to shared_ptr
    std::shared_ptr<PullObjectIngester> ingesterPtr(ingester);
    m_pullIngesters.push_back(ingesterPtr);
    return m_pullIngesters.back();
}

bool ObjectController::removePullObjectIngester(std::shared_ptr<PullObjectIngester> &pullIngester) {
    auto it = std::find(m_pullIngesters.begin(), m_pullIngesters.end(), pullIngester);
    if (it != m_pullIngesters.end()) {
        m_pullIngesters.erase(it);
        return true;
    }
    return false;
}

// Function to remove leading and trailing slashes from a string
std::string ObjectController::trimSlashes(const std::string& str) {
    size_t start = (str.front() == '/') ? 1 : 0;
    size_t end = (str.back() == '/') ? str.size() - 1 : str.size();
    return str.substr(start, end - start);
}

// Function to remove the domain part of an absolute URL
std::string ObjectController::removeBaseURL(const std::string& url) {
    std::regex baseUrlPattern(R"(https?://[^/]+)");
    return trimSlashes(std::regex_replace(url, baseUrlPattern, ""));
}


MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
