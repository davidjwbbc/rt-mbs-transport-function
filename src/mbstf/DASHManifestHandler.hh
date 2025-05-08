#ifndef _MBS_TF_DASH_MANIFEST_HANDLER_HH_
#define _MBS_TF_DASH_MANIFEST_HANDLER_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: DASH Manifest Handler class
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
 * Author(s): Dev Audsin <dev.audsin@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <libmpd++/libmpd++.hh>

#include "common.hh"
#include "ManifestHandler.hh"
#include "ObjectStore.hh"
#include "PullObjectIngester.hh"

MBSTF_NAMESPACE_START

class DASHManifestHandler : public ManifestHandler {
public:
    DASHManifestHandler() = delete;
    DASHManifestHandler(const ObjectStore::Object &object);
    DASHManifestHandler(const DASHManifestHandler &) = delete;
    DASHManifestHandler(DASHManifestHandler &&) = delete;

    virtual ~DASHManifestHandler();

    DASHManifestHandler &operator=(const DASHManifestHandler &) = delete;
    DASHManifestHandler &operator=(DASHManifestHandler &&) = delete;

    virtual std::pair<ManifestHandler::time_type, ManifestHandler::ingest_list> nextIngestItems();
    virtual ManifestHandler::durn_type getDefaultDeadline();
    virtual bool update(const ObjectStore::Object &new_manifest);
    static unsigned int factoryPriority() { return 100; };
private:

  LIBMPDPP_NAMESPACE_CLASS(MPD)  m_mpd;

};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_DASH_MANIFEST_HANDLER_HH_ */
