/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Testing MBSTF Pull Ingester
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
#include "PullObjectIngester.hh"

MBSTF_NAMESPACE_START
using namespace std::literals;

class ObjectController {};

int pass= 0;
int fail = 0;


MBSTF_NAMESPACE_STOP
MBSTF_NAMESPACE_USING;

int main() {
    // Create instances of ObjectStore and ObjectController
    ObjectController objectController;
    ObjectStore store(objectController);

    // Create a list of IngestItem
    //using time_type = std::chrono::system_clock::time_point;
    std::list<PullObjectIngester::IngestItem> id_to_url_map = {
        {"object1", "http://127.0.0.1/object1", std::nullopt},
        {"object2", "http://127.0.0.1/object2", std::nullopt}
    };

    // Create an instance of PullObjectIngester
    PullObjectIngester pullIngester(store, objectController, id_to_url_map);

    // Print information about each IngestItem
    for (const auto& item : id_to_url_map) {
        std::cout << "Object ID: " << item.objectId() << std::endl;
        std::cout << "URL: " << item.url() << std::endl;
        if (item.hasDeadline()) {
            std::cout << "Deadline: " << std::chrono::system_clock::to_time_t(item.deadline(std::chrono::system_clock::now())) << std::endl;
        } else {
            std::cout << "No deadline" << std::endl;
        }
    }

    return 0;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
