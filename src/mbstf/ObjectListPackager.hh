#ifndef _MBS_TF_MBSTF_LIST_PACKAGER_HH_
#define _MBS_TF_MBSTF_LIST_PACKAGER_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSTF Object List Controller
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <memory>
#include <list>
#include <optional>
#include <string>
#include <chrono>
#include <thread>
#include <memory>
#include <boost/asio/io_service.hpp>

#include "common.hh"
//#include "ObjectListController.hh"
#include "ObjectStore.hh"
#include "ObjectPackager.hh"

namespace LibFlute{
    class Transmitter;
}

MBSTF_NAMESPACE_START

class ObjectListController;
class ObjectPackager;

class ObjectListPackager : public ObjectPackager {
public:
    using time_type = std::chrono::system_clock::time_point;

    class PackageItem {
    public:
        PackageItem() = delete;
        PackageItem(const std::string &object_id, const std::optional<time_type> &deadline = std::nullopt);
        PackageItem(const PackageItem &other);
        PackageItem(PackageItem &&other);
        virtual ~PackageItem() {};

        const std::string &objectId() const { return m_objectId; }
        PackageItem &objectId(const std::string &object_id) { m_objectId = object_id; return *this; }
        PackageItem &objectId(std::string &&object_id) { m_objectId = std::move(object_id); return *this; }

        bool hasDeadline() const { return m_deadline.has_value(); }
        const std::optional<time_type> &deadline() const { return m_deadline; }
        time_type deadline(const time_type &default_deadline) const { return m_deadline.value_or(default_deadline); }
        PackageItem &deadline(std::nullopt_t) { m_deadline.reset(); return *this; }
        PackageItem &deadline(const time_type &deadline) { m_deadline = time_type(deadline); return *this; }
        PackageItem &deadline(time_type &&deadline) { m_deadline = std::move(deadline); return *this; }

    private:
        std::string m_objectId;
        std::optional<time_type> m_deadline;
    };

    ObjectListPackager() = delete;
    ObjectListPackager(ObjectStore &object_store, ObjectListController &controller, const std::list<PackageItem> &object_to_package, const std::shared_ptr<std::string> &address, uint32_t rateLimit, unsigned short mtu, short port);
    ObjectListPackager(ObjectStore &object_store, ObjectListController &controller, std::list<PackageItem> &&object_to_package, const std::shared_ptr<std::string> &address, uint32_t rateLimit, unsigned short mtu, short port);
    ObjectListPackager(ObjectStore &object_store, ObjectListController &controller, const std::shared_ptr<std::string> &address, uint32_t rateLimit, unsigned short mtu, short port);
    virtual ~ObjectListPackager();

    bool add(const PackageItem &item);
    bool add(PackageItem &&item);

protected:
    virtual void doObjectPackage();

private:
    void sortListIntoPriorityOrder();
    std::list<PackageItem> m_packageItems;
    LibFlute::Transmitter *m_transmitter;
    boost::asio::io_service m_io;
    uint32_t m_queuedToi;
    bool m_queued;
    // std::shared_ptr<flute> m_flute;
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_MBSTF_OBJECT_LIST_PACKAGER_HH_ */

