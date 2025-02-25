#ifndef _MBS_TF_MBSTF_OBJECT_PACKAGER_HH_
#define _MBS_TF_MBSTF_OBJECT_PACKAGER_HH_
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
#include <atomic>
#include <memory>

#include <boost/asio/io_service.hpp>

namespace LibFlute{
    class Transmitter;
}

MBSTF_NAMESPACE_START

class ObjectStore;
class ObjectController;

class ObjectPackager {
public:
    ObjectPackager() = delete;
    ObjectPackager(ObjectPackager &&) = delete;
    ObjectPackager(const ObjectPackager &) = delete;

    ObjectPackager(ObjectStore &objectStore, ObjectController &controller)
        : m_transmitter(nullptr), m_io(), m_queuedToi(0), m_queued(false)
	, m_objectStore(objectStore), m_controller(controller), m_destIpAddr(nullptr), m_rateLimit(0), m_mtu(0), m_port(0), m_workerThread(), m_workerCancel(false) {}

    ObjectPackager(ObjectStore &objectStore, ObjectController &controller, std::shared_ptr<std::string> destIpAddr, uint32_t rateLimit, unsigned short mtu, short port)
        : m_transmitter(nullptr), m_io(), m_queuedToi(0), m_queued(false)
	, m_objectStore(objectStore), m_controller(controller), m_destIpAddr(destIpAddr), m_rateLimit(rateLimit), m_mtu(mtu), m_port(port), m_workerThread(), m_workerCancel(false) {}

    void abort() {
        m_workerCancel = true;
        if (m_workerThread.joinable()) {
            m_workerThread.join();
        }

    }

    virtual ~ObjectPackager() {
        abort();
    }

    ObjectPackager& setDestIpAddr(std::shared_ptr<std::string> destIpAddr);
    ObjectPackager& setPort(short port);
    ObjectPackager& setMtu(unsigned short mtu);
    ObjectPackager& setRateLimit(uint32_t rateLimit);
    void startWorker(){m_workerThread = std::thread(workerLoop, this);};

protected:
    const ObjectStore &objectStore() const { return m_objectStore; }
    ObjectStore &objectStore() { return m_objectStore; }
    ObjectController &controller() { return m_controller; }
    const ObjectController &controller() const { return m_controller; }
    const std::shared_ptr<std::string> &destIpAddr() const { return m_destIpAddr; }
    const uint32_t &rateLimit() const { return m_rateLimit; }
    const unsigned short &mtu() const { return m_mtu; }
    const short &port() const { return m_port; }


    virtual void doObjectPackage() = 0;
    LibFlute::Transmitter *m_transmitter;
    boost::asio::io_service m_io;
    uint32_t m_queuedToi;
    bool m_queued;

private:
    static void workerLoop(ObjectPackager*);
    ObjectStore &m_objectStore;
    ObjectController &m_controller;
    std::shared_ptr<std::string> m_destIpAddr;
    uint32_t m_rateLimit;
    unsigned short m_mtu;
    short m_port;
    std::thread m_workerThread;
    std::atomic_bool m_workerCancel;
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_MBSTF_OBJECT_PACKAGER_HH_ */

