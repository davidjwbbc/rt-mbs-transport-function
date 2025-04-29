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
#include <stdexcept>
#include <exception>
#include "tinyxml2.h"

#include "common.hh"
#include "ManifestHandler.hh"
#include "ManifestHandlerFactory.hh"
#include "ObjectStore.hh"
#include "PullObjectIngester.hh"

#include "DASHManifestHandler.hh"

using namespace std::literals::chrono_literals;
using namespace tinyxml2;

MBSTF_NAMESPACE_START

static bool is_valid_manifest(const ObjectStore::ObjectData &manifest_data);
static bool validate_manifest(const ObjectStore::Object &new_manifest, const ObjectStore::ObjectData &manifest_data, const ObjectStore::Metadata &metadata);


DASHManifestHandler::DASHManifestHandler(const ObjectStore::Object &object)
    :ManifestHandler()
{
}

DASHManifestHandler::~DASHManifestHandler()
{
}

std::pair<ManifestHandler::time_type, ManifestHandler::ingest_list> DASHManifestHandler::nextIngestItems()
{
    // TODO: make this work properly with a DASH MPD contents - just fake ingest items for now.
    auto fetch_time = std::chrono::system_clock::now() + 4s;
    auto deadline = fetch_time + 4s;
    std::string base_url("http://localhost/");
    std::string url("http://localhost/dummy-object");
    static const std::string empty;
    std::list<PullObjectIngester::IngestItem> ingest_items({PullObjectIngester::IngestItem(empty, url, empty, base_url, std::nullopt, deadline)});

    return std::make_pair(fetch_time, ingest_items);
}

ManifestHandler::durn_type DASHManifestHandler::getDefaultDeadline()
{
    // TODO: get the segment length from the DASH MPD
    return 4s;
}

bool DASHManifestHandler::update(const ObjectStore::Object &new_manifest, const ObjectStore::ObjectData &manifest_data, const ObjectStore::Metadata &metadata)
{
    // TODO: process the new MPD and see what has changed, throw an exception of the Object is not understood or invalid
    
    validate_manifest(new_manifest, manifest_data, metadata);
    return true; // assume manifest updated, use false for no manifest change
}

bool DASHManifestHandler::validateManifest(const ObjectStore::Object &new_manifest, const ObjectStore::ObjectData &manifest_data, const ObjectStore::Metadata &metadata) {
    return validate_manifest(new_manifest, manifest_data, metadata);

}

static bool g_registered = ManifestHandlerFactory::registerManifestHandler("application/dash+xml", new ManifestHandlerConstructorClass<DASHManifestHandler>());

static bool validate_manifest(const ObjectStore::Object &new_manifest, const ObjectStore::ObjectData &manifest_data, const ObjectStore::Metadata &metadata)
{
    if ( !metadata.mediaType().empty()  && metadata.mediaType() != "application/dash+xml" ){
         throw std::invalid_argument("Does not look like a DASH Manifest as the media type is invalid. Expected media type: application/dash+xml");
    }

    if(is_valid_manifest(manifest_data)) return true;

    return true;


}

static bool is_valid_manifest(const ObjectStore::ObjectData &manifest_data) {
    std::string xml_data(manifest_data.begin(), manifest_data.end());
    XMLDocument doc;
    // Parse the XML content (returns XML_SUCCESS if no errors were found)
    XMLError error = doc.Parse(xml_data.c_str());
    if (error != XML_SUCCESS) {

        throw std::invalid_argument("Error parsing XML:" + std::string(doc.ErrorStr()));
    }
    // Retrieve the root element.
    XMLElement* root = doc.RootElement();
    if (!root) {
        throw std::invalid_argument("Error parsing XML: No root element found.");
    }
    // Verify that the root element is named "MPD".
    if (std::string(root->Name()) != "MPD") {
        throw std::invalid_argument("Error parsing XML: Invalid element found. Expected 'MPD', found: " + std::string(root->Name()));
    }
    // Check for the required 'xmlns' attribute in the MPD element.
    const char* xmlnsAttr = root->Attribute("xmlns");
    if (!xmlnsAttr) {

        throw std::invalid_argument("Missing 'xmlns' attribute in MPD element.");
    }
    // Define a list of accepted standard MPD namespaces.
    std::vector<std::string> validNamespaces = {
        "urn:mpeg:dash:schema:mpd:2011"
        // Add more valid namespaces here if needed.
    };

    // Validate that the namespace matches one of the accepted ones.
    bool validNamespace = false;
    for (const auto& ns : validNamespaces) {
        if (ns == std::string(xmlnsAttr)) {
            validNamespace = true;
            break;
        }
    }
    if (!validNamespace) {
        throw std::invalid_argument("Invalid MPD namespace. Found: " + std::string(xmlnsAttr));
    }
    return true;
}



MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
