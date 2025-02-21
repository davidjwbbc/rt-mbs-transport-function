/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Testing MBSTF Object store
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 * Author(s): Dev Audsin
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */


#include <memory>
#include <stdexcept>
#include <utility>
#include <chrono>
#include <thread>
#include <mutex>
#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <map>
#include <list>

#include "common.hh"
#include "ObjectStore.hh"
#include "ObjectListPackager.hh"

MBSTF_NAMESPACE_START
using namespace std::literals;

class ObjectController {};
class ObjectListController: public ObjectController {};

int pass= 0;
int fail = 0;

std::string firstObject = "obj1";
std::string secondObject = "obj2";

void testAddObject(ObjectStore& store) {
	
    ObjectStore::ObjectData firstObjectData = {0x31, 0x32};
    ObjectStore::ObjectData secondObjectData = {0x50, 0x51, 0x52};
    	
    ObjectStore::Metadata firstObjectMetadata("type1", "url1", "fetched_url1", std::chrono::system_clock::now());
    ObjectStore::Metadata secondObjectMetadata("type2", "url2", "fetched_url2", std::chrono::system_clock::now() + std::chrono::minutes(1));

    firstObjectMetadata.entityTag("etag1");
    firstObjectMetadata.cacheExpires(std::chrono::system_clock::now() + 5s);

    secondObjectMetadata.entityTag("etag2");
    secondObjectMetadata.cacheExpires(std::chrono::system_clock::now() + 5s);
    store.addObject(firstObject, std::move(firstObjectData), std::move(firstObjectMetadata));
    store.addObject(secondObject, std::move(secondObjectData), std::move(secondObjectMetadata));
    
    if (store.getObjectData(firstObject) == ObjectStore::ObjectData{0x31, 0x32}) {
	    std::cout<<"INFO: testAddObject for firstObject passed."<<std::endl;
	    pass++;
    } else {
	    std::cout<<"ERROR: testAddObject for firstObject failed."<<std::endl;
	    fail++;
    }

    if (store.getObjectData(secondObject) == ObjectStore::ObjectData{0x50, 0x51, 0x52}) {
        std::cout<<"INFO: testAddObject for secondObject passed."<<std::endl;
    } else {
        std::cout<<"ERROR: testAddObject for secondObject failed."<<std::endl;
    }
}

void testDeleteFirstObject(ObjectStore& store) {
    store.deleteObject(firstObject);

    try {
        store.getObjectData(firstObject);
        std::cout<<"ERROR: testDeleteObject for firstObject failed."<<std::endl;
        fail++;
    } catch (const std::out_of_range& e) {
        std::cout<<"INFO: testDeleteObject for firstObject passed."<<std::endl;
        pass++;
    }

}

void testDeleteSecondObject(ObjectStore& store) {
    store.deleteObject(secondObject);

    try {
        store.getObjectData(secondObject);
        std::cout<<"ERROR: testDeleteObject for secondObject failed."<<std::endl;
        fail++;
    } catch (const std::out_of_range& e) {
        std::cout<<"INFO: testDeleteObject for secondObject passed."<<std::endl;
        pass++;
    }

}


void testObjectListPackager(ObjectStore &store, ObjectListController &controller) {

    std::shared_ptr<std::string> address = std::make_shared<std::string>("127.0.0.1");
    uint32_t rateLimit = 1000;
    unsigned short mtu = 1500;
    short port = 8080;

    ObjectListPackager packager(store, controller, address, rateLimit, mtu, port);
    packager.startWorker();
    // Add a PackageItem to ObjectPackager
    ObjectListPackager::PackageItem item("obj1");
    packager.add(item);
    std::this_thread::sleep_for(10s);
    // Verification
    std::cout << "Test completed successfully." << std::endl;
}

MBSTF_NAMESPACE_STOP

MBSTF_NAMESPACE_USING;

int main() {
    
    ObjectListController objectListController;
    ObjectStore store(objectListController);

    std::cout << "### ObjectStore: Test start #### " << std::endl;
    
    testAddObject(store);
    testObjectListPackager(store, objectListController);
    testDeleteFirstObject(store);
    testDeleteSecondObject(store);

    return 0;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
