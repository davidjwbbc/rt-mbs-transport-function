#ifndef _MBS_TF_MANIFEST_HANDLER_HH_
#define _MBS_TF_MANIFEST_HANDLER_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Manifest Handler Factory
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include <chrono>
#include <utility>

#include "common.hh"
#include "ObjectStore.hh"
#include "PullObjectIngester.hh"

MBSTF_NAMESPACE_START

class ManifestHandler {
public:
    using time_type = std::chrono::system_clock::time_point;
    using durn_type = std::chrono::system_clock::duration;
    using ingest_list = std::list<PullObjectIngester::IngestItem>;

    virtual std::pair<time_type, ingest_list> nextIngestItems() = 0;
    virtual durn_type getDefaultDeadline() = 0;
    virtual bool update(const ObjectStore::Object &new_manifest) = 0;

};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_MANIFEST_HANDLER_HH_ */
