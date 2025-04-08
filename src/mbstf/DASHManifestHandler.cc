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
#include <chrono>
#include <string>

#include "common.hh"
#include "ManifestHandler.hh"
#include "ManifestHandlerFactory.hh"
#include "ObjectStore.hh"
#include "PullObjectIngester.hh"

#include "DASHManifestHandler.hh"

using namespace std::literals::chrono_literals;

MBSTF_NAMESPACE_START

DASHManifestHandler::DASHManifestHandler(const ObjectStore::Object &object)
    :ManifestHandler()
{
}

DASHManifestHandler::~DASHManifestHandler()
{
}

std::pair<ManifestHandler::time_type, PullObjectIngester::IngestItem> DASHManifestHandler::nextIngestItem()
{
    // TODO: make this work properly with a DASH MPD contents - just fake ingest items for now.
    auto fetch_time = std::chrono::system_clock::now() + 4s;
    auto deadline = fetch_time + 4s;
    std::string base_url("http://localhost/");
    std::string url("http://localhost/dummy-object");
    static const std::string empty;

    return std::make_pair(fetch_time, PullObjectIngester::IngestItem(empty, url, empty, base_url, std::nullopt, deadline));
}

static bool g_registered = ManifestHandlerFactory::registerManifestHandler("application/dash+xml", new ManifestHandlerConstructorClass<DASHManifestHandler>());

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
