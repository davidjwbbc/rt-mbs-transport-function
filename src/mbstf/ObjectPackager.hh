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


#include <atomic>
#include <memory>
#include <optional>
#include <thread>

#include <netinet/in.h>

#include <boost/asio/io_service.hpp>

#include "common.hh"

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

    ObjectPackager(ObjectStore &objectStore, ObjectController &controller, std::optional<std::string> destIpAddr = std::nullopt, uint32_t rateLimit = 0, unsigned short mtu = 0, in_port_t port = 0)
        :m_transmitter(nullptr), m_io(), m_queuedToi(0), m_queued(false)
        ,m_objectStore(objectStore), m_controller(controller), m_destIpAddr(destIpAddr), m_rateLimit(rateLimit), m_mtu(mtu)
        ,m_port(port), m_workerThread(), m_workerCancel(false)
    {};

    void abort() {
        m_workerCancel = true;
        if (m_workerThread.joinable()) {
            m_workerThread.join();
        }
    };

    virtual ~ObjectPackager() {
        abort();
    };

    ObjectPackager& setDestIpAddr(const std::optional<std::string> &destIpAddr);
    ObjectPackager& setPort(in_port_t port);
    ObjectPackager& setMtu(unsigned short mtu);
    ObjectPackager& setRateLimit(uint32_t rateLimit);
    void startWorker() {
        if (m_workerThread.get_id() != std::thread::id()) return;
        if (!!m_workerCancel) return;
        m_workerThread = std::thread(workerLoop, this);
    };

protected:
    const ObjectStore &objectStore() const { return m_objectStore; };
    ObjectStore &objectStore() { return m_objectStore; };
    const ObjectController &controller() const { return m_controller; };
    ObjectController &controller() { return m_controller; };
    const std::optional<std::string> &destIpAddr() const { return m_destIpAddr; };
    uint32_t rateLimit() const { return m_rateLimit; };
    unsigned short mtu() const { return m_mtu; };
    in_port_t port() const { return m_port; };

    virtual void doObjectPackage() = 0;

    LibFlute::Transmitter *m_transmitter;
    boost::asio::io_service m_io;
    uint32_t m_queuedToi;
    bool m_queued;

private:
    static void workerLoop(ObjectPackager*);
    ObjectStore &m_objectStore;
    ObjectController &m_controller;
    std::optional<std::string> m_destIpAddr;
    uint32_t m_rateLimit;
    unsigned short m_mtu;
    in_port_t m_port;
    std::thread m_workerThread;
    std::atomic_bool m_workerCancel;
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_MBSTF_OBJECT_PACKAGER_HH_ */

