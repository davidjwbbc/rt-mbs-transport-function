/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Main app entry point
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <map>
#include <memory>
#include <string>

#include "common.hh"
#include "App.hh"
#include "Open5GSYamlDocument.hh"
#include "Open5GSYamlIter.hh"

#include "Context.hh"

MBSTF_NAMESPACE_START

Context::Context()
    :servers()
    ,cacheControl({60, 60})
{
}

Context::~Context()
{
    for (auto &svr : servers) {
        svr.reset();
    }
}

bool Context::parseConfig()
{
    Open5GSYamlDocument doc(App::self().configDocument());
    Open5GSYamlIter root_iter(doc);
    while (root_iter.next()) {
        std::string root_key(root_iter.key());
        if (root_key == "mbstf") {
            Open5GSYamlIter mbstf_iter(root_iter);
            while (mbstf_iter.next()) {
                std::string mbstf_key(mbstf_iter.key());
                if (mbstf_key == "sbi" || mbstf_key == "service_name" || mbstf_key == "discovery") {
                    // Handled by SBI config parser
                } else {
                    ogs_warn("Unknown key `mbstf.%s` in configuration", mbstf_key.c_str());
                }
            }
        }
    }

    //if (!validate()) {
    //    return false;
    //}
    
    return true;
}

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
