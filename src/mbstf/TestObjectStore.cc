/******************************************************************************
 * 5G-MAG Reference Tools: MBS Transport Function: Open5GS Application interface
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <memory>
#include <stdexcept>
#include <utility>
#include <chrono>
#include <iostream>
#include <thread>
#include <string>
#include <vector>

#include "ObjectStore.hh"
#include "TestObjectStore.hh"

MBSTF_NAMESPACE_START

int TestObjectStore::testObjectStore() {
    std::string object1 = "object1";
    std::string object2 = "object2";
    const std::string type1 = "type1";
    const std::string type2 = "type2";

    ObjectStore store;

    // Create some initial data
    ObjectStore::ObjectData testData1 = {0x31, 0x32};
    ObjectStore::ObjectData testData2 = {0x50, 0x51, 0x52};

    // Create threads for adding, retrieving, and deleting objects
    std::vector<std::thread> threads;
    threads.emplace_back(addObject, std::ref(store), object1, testData1, type1);
    threads.emplace_back(addObject, std::ref(store), object2, testData2, type2);
    threads.emplace_back(getObject, std::ref(store), object1);
    threads.emplace_back(getMetadata, std::ref(store), object1);
    threads.emplace_back(getObject, std::ref(store), object2);
    threads.emplace_back(getMetadata, std::ref(store), object2);
    threads.emplace_back(deleteObject, std::ref(store), object2);
    threads.emplace_back(getObject, std::ref(store), object1);
    threads.emplace_back(getObject, std::ref(store), object2);

    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }

    return 0;

}


void TestObjectStore::addObject(ObjectStore& store, const std::string& object_id, const ObjectStore::ObjectData& data, const std::string& type) {
    store.addObject(object_id, ObjectStore::ObjectData(data), type);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void TestObjectStore::getObject(ObjectStore& store, const std::string& object_id) {
    try {
	std::ostringstream retrievedDataElements;
        std::ostringstream retrievedDataSize;
        std::ostringstream rData;

        ObjectStore::ObjectData retrievedData = store.getObject(object_id);

        retrievedDataElements << "Retrived elements: " << retrievedData.size() << " ";
        retrievedDataSize << "Size of retrieved data: " << retrievedData.size() * sizeof(unsigned char) << " bytes " ;
        ogs_info("%s", retrievedDataElements.str().c_str());
        ogs_info("%s", retrievedDataSize.str().c_str());
        

        for (const auto& byte : retrievedData) {
	    rData << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << ", ";
        }
        ogs_info("Retrieved data: %s", rData.str().c_str());

    } catch (const std::out_of_range&) {
	std::ostringstream oor;
        oor << "Object " << object_id << " not found" << "";     
        ogs_info("%s", oor.str().c_str());
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

void TestObjectStore::getMetadata(ObjectStore& store, const std::string& object_id) {

    std::ostringstream meta;
   // Get metadata from the object store
    ObjectStore::Metadata metadata = store.getMetadata(object_id);
    meta << "Metadata - Type: " << metadata.type
              << ", Created: " << timePointToString(metadata.created)
              << ", Modified: " << timePointToString(metadata.modified)
              << std::endl;
    ogs_info("%s", meta.str().c_str());
}

void TestObjectStore::deleteObject(ObjectStore& store, const std::string& object_id) {
    
    std::ostringstream deletedObject;
    store.deleteObject(object_id);
    deletedObject << "Object " << object_id << " deleted" << "";
    ogs_info("%s", deletedObject.str().c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

std::string TestObjectStore::timePointToString(const std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>& tp) {
    auto timeT = std::chrono::system_clock::to_time_t(tp);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}


MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
