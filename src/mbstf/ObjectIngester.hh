#ifndef _MBS_TF_OBJECT_INGESTER_HH_
#define _MBS_TF_OBJECT_INGESTER_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Object Ingester base class
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * Author(s): Dev Audsin <dev.audsin@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */


#include "common.hh"
#include <thread>
#include <atomic>

MBSTF_NAMESPACE_START

class ObjectStore;
class ObjectController;

class ObjectIngester {
public:
    ObjectIngester() = delete;
    ObjectIngester(ObjectStore &objectStore, ObjectController &controller)
        : m_objectStore(objectStore), m_controller(controller), m_workerThread(), m_workerCancel(false) {}

    void abort() {
	m_workerCancel = true;
        if (m_workerThread.joinable()) {
	    m_workerThread.join();
        }

    }

    virtual ~ObjectIngester() {
	abort();
    }

protected:
    ObjectStore &objectStore() { return m_objectStore; }
    const ObjectStore &objectStore() const { return m_objectStore; }
    ObjectController &controller() { return m_controller; }
    const ObjectController &controller() const { return m_controller; }
    void startWorker(){m_workerThread = std::thread(workerLoop, this);};

    virtual void doObjectIngest() = 0;

private:
    static void workerLoop(ObjectIngester*);
    ObjectStore &m_objectStore;
    ObjectController &m_controller;
    std::thread m_workerThread;
    std::atomic_bool m_workerCancel;
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_OBJECT_INGESTER_HH_ */
