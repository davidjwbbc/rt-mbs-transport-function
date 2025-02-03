#ifndef _MBS_TF_MBSTF_OBJECT_INGESTER_HH_
#define _MBS_TF_MBSTF_OBJECT_INGESTER_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSTF Object Ingester base class
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */


#include "common.hh"
#include <thread>

MBSTF_NAMESPACE_START

class ObjectStore;
class Controller;

class ObjectIngester {
public:
    ObjectIngester() = delete;
    ObjectIngester(ObjectStore &objectStore, Controller &controller)
        : m_objectStore(objectStore), m_controller(controller), m_workerThread(&ObjectIngester::doObjectIngest, this) {}

    void abort() {
    }

    virtual ~ObjectIngester() {
        if (m_workerThread.joinable()) {
            m_workerThread.join();
        }
    }

protected:
    ObjectStore &objectStore() { return m_objectStore; }
    const ObjectStore &objectStore() const { return m_objectStore; }
    Controller &controller() { return m_controller; }
    const Controller &controller() const { return m_controller; }

    virtual void doObjectIngest() = 0;

private:
    ObjectStore &m_objectStore;
    Controller &m_controller;
    std::thread m_workerThread;
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_MBSTF_OBJECT_INGESTER_HH_ */
