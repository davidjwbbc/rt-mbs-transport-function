/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: ObjectController class
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <memory>
#include <list>

#include "common.hh"
#include "Controller.hh"
#include "DistributionSession.hh"
#include "ObjectStore.hh"
#include "PullObjectIngester.hh"
#include "PushObjectIngester.hh"

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

std::shared_ptr<PushObjectIngester> &ObjectController::setPushIngester(PushObjectIngester *pushIngester) {
    m_pushIngester = std::shared_ptr<PushObjectIngester>(pushIngester);
    return m_pushIngester;
}


MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
