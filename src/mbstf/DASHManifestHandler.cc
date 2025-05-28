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
#include <cstring>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <exception>
#include <optional>
#include <algorithm>
#include <uuid/uuid.h>

#include <libmpd++/SegmentAvailability.hh>

#include "ogs-app.h"
#include "common.hh"
#include "ManifestHandler.hh"
#include "ManifestHandlerFactory.hh"
#include "ObjectStore.hh"
#include "PullObjectIngester.hh"

#include "DASHManifestHandler.hh"

using namespace std::literals::chrono_literals;

LIBMPDPP_NAMESPACE_USING_ALL;

MBSTF_NAMESPACE_START

using time_type = std::chrono::system_clock::time_point;

static LIBMPDPP_NAMESPACE_CLASS(MPD) ingest_manifest(const ObjectStore::Object &new_manifest);

DASHManifestHandler::DASHManifestHandler(const ObjectStore::Object &object, bool pull_distribution)
    :ManifestHandler(pull_distribution)
    ,m_mpd(ingest_manifest(object))
    ,m_manifest(&object)
    ,m_refreshMpd(false)
{
    m_mpd.selectAllRepresentations();

    //TODO: Use selectedInitializationSegments() to fetch Initialization Segments
}

DASHManifestHandler::~DASHManifestHandler()
{
}

std::pair<ManifestHandler::time_type, ManifestHandler::ingest_list> DASHManifestHandler::nextIngestItems()
{

    std::list<PullObjectIngester::IngestItem> ingest_items;
    static const std::string empty;
    auto current_time = std::chrono::system_clock::now();
    std::optional<std::chrono::system_clock::time_point> time_to_update;
    std::string manifest_url;
    std::list<LIBMPDPP_NAMESPACE_CLASS(SegmentAvailability)> available_segments;
    time_type fetch_time;

    available_segments = m_mpd.selectedSegmentAvailability();

    for (auto &sa : available_segments) {
      std::ostringstream oss;
      oss << sa;
      ogs_debug("    %s", oss.str().c_str());
    }

    if (m_pullDistribution && !m_refreshMpd && m_mpd.hasMinimumUpdatePeriod()) {
        auto min_update_time = m_manifest->second.receivedTime() + m_mpd.minimumUpdatePeriod().value();
        time_to_update = m_manifest->second.hasExpiryTime() ? std::max(min_update_time, m_manifest->second.ExpiryTime()): min_update_time;
    }

    if (time_to_update && current_time > *time_to_update) {
    // Pick the fetched URL if available; otherwise, use the original URL.
        manifest_url = m_manifest->second.getFetchedUrl();
	available_segments.push_back(LIBMPDPP_NAMESPACE_CLASS(SegmentAvailability) (time_to_update.value(), 0s, manifest_url, m_mpd.availabilityEndTime()));
    }

    if(!available_segments.empty()) {

        available_segments.sort();

        auto first_available_segment = available_segments.front();
        fetch_time = first_available_segment.availabilityStartTime();

        ingest_items.push_back(PullObjectIngester::IngestItem(nextObjectId(), first_available_segment.segmentURL(), empty,
				    std::nullopt, std::nullopt, first_available_segment.availabilityEndTime()));


	try {
            if(first_available_segment.segmentURL()  == manifest_url) m_refreshMpd = true;
        } catch (std::domain_error &err) {
            ogs_error("Invalid Segment URL: %s", err.what());
	    throw;
        }
        auto it = available_segments.begin();
        // Iterate from second element.
        for ( ++it; it != available_segments.end(); ++it ) {

            if(it->availabilityStartTime() != fetch_time) break;
	    ingest_items.push_back(PullObjectIngester::IngestItem(nextObjectId(), it->segmentURL(), empty, std::nullopt, std::nullopt, it->availabilityEndTime()));

             if(it->segmentURL()  == manifest_url) m_refreshMpd = true;

        }
    }


    return std::make_pair(fetch_time, ingest_items);
}

std::string DASHManifestHandler::nextObjectId()
{
    return generateUUID();
}

std::string DASHManifestHandler::generateUUID() {
    uuid_t uuid;
    uuid_generate_random(uuid);
    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);
    return std::string(uuid_str);
}


ManifestHandler::durn_type DASHManifestHandler::getDefaultDeadline()
{
    // TODO: get the segment length from the DASH MPD
    return 4s;
}

bool DASHManifestHandler::update(const ObjectStore::Object &new_manifest)
{
    // TODO: process the new MPD and see what has changed, throw an exception of the Object is not understood or invalid
    
    m_refreshMpd = false;
    m_mpd = ingest_manifest(new_manifest);
    m_mpd.selectAllRepresentations();
    //TO DO: SelectedInitialistionSegments(): For everything init segments in the list schedule an ingester.
    return true; // assume manifest updated, use false for no manifest change
}

static bool g_registered = ManifestHandlerFactory::registerManifestHandler("application/dash+xml", new ManifestHandlerConstructorClass<DASHManifestHandler>());

static LIBMPDPP_NAMESPACE_CLASS(MPD) ingest_manifest(const ObjectStore::Object &new_manifest)
{
    if ( new_manifest.second.mediaType() != "application/dash+xml" ){
         throw std::invalid_argument("Does not look like a DASH Manifest as the media type is invalid. Expected media type: application/dash+xml");
    }
    return LIBMPDPP_NAMESPACE_CLASS(MPD) (new_manifest.first);


}

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
