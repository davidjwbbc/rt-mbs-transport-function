#ifndef _MBS_TF_MBSTF_TEST_OBJECT_STORE_HH_
#define _MBS_TF_MBSTF_TEST_OBJECT_STORE_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSTF Network Function class
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#pragma once

#include "ogs-app.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include <memory>
#include <utility>
#include <chrono>
#include <vector>
#include <map>

#include "common.hh"
#include "ObjectStore.hh"

MBSTF_NAMESPACE_START

class TestObjectStore: public ObjectStore {
public:
    TestObjectStore() : ObjectStore() {};
    virtual ~TestObjectStore() {};
    static void addObject(ObjectStore& store, const std::string& object_id, const ObjectStore::ObjectData& data, const std::string& type);
    static void getObject(ObjectStore& store, const std::string& object_id);
    static void getMetadata(ObjectStore& store, const std::string& object_id);
    static void deleteObject(ObjectStore& store, const std::string& object_id); 
    static int testObjectStore();

private:

    static std::string timePointToString(const std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>& tp);

};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_MBSTF_TEST_OBJECT_STORE_HH_ */
