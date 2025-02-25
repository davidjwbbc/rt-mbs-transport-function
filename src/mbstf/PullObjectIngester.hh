#ifndef _MBS_TF_MBSTF_PULL_OBJECT_INGESTER_HH_
#define _MBS_TF_MBSTF_PULL_OBJECT_INGESTER_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSTF Pull Object Ingester class
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <string>
#include <list>

#include "common.hh"
#include "ObjectStore.hh"
#include "ObjectIngester.hh"
#include "Curl.hh"

MBSTF_NAMESPACE_START

class ObjectStore;
class ObjectController;
class Curl;

class PullObjectIngester : public ObjectIngester {
public:
	
    using time_type = std::chrono::system_clock::time_point;

    class IngestItem {
    public:

        IngestItem() = delete;
        IngestItem(const std::string &object_id, const std::string &url, const std::optional<time_type> &download_deadline = std::nullopt);
        IngestItem(const IngestItem &other);
        IngestItem(IngestItem &&other);
        virtual ~IngestItem() {};

        const std::string &objectId() const { return m_objectId; };
        IngestItem &objectId(const std::string &object_id) { m_objectId = object_id; return *this; };
        IngestItem &objectId(std::string &&object_id) { m_objectId = std::move(object_id); return *this; };

        const std::string &url() const { return m_url; };
        IngestItem &url(const std::string &url) { m_url = url; return *this; };
        IngestItem &url(std::string &&url) { m_url = std::move(url); return *this; };

        bool hasDeadline() const { return m_deadline.has_value(); };
        const std::optional<time_type> &deadline() const { return m_deadline; };
        time_type deadline(const time_type &default_deadline) const { return m_deadline.value_or(default_deadline); };
        IngestItem &deadline(std::nullopt_t) { m_deadline.reset(); return *this; };
        IngestItem &deadline(const time_type &dl_deadline) { m_deadline = time_type(dl_deadline); return *this; };
        IngestItem &deadline(time_type &&dl_deadline) { m_deadline = std::move(dl_deadline); return *this; };


    private:
        std::string m_objectId;
        std::string m_url;
        std::optional<time_type> m_deadline;
    };

    PullObjectIngester() = delete;
    PullObjectIngester(ObjectStore& object_store, ObjectController &controller, const std::list<IngestItem> &id_to_url_map)
      :ObjectIngester(object_store, controller)
      ,m_fetchList(id_to_url_map)
    { sortListByPolicy(); startWorker(); };
    PullObjectIngester(ObjectStore& object_store, ObjectController &controller, std::list<IngestItem> &&id_to_url_map)
      :ObjectIngester(object_store, controller)
      ,m_fetchList(std::move(id_to_url_map))
    { sortListByPolicy(); startWorker();};
    virtual ~PullObjectIngester();

    bool fetch(const IngestItem &item);
    bool fetch(IngestItem &&item);
    bool fetch(const std::string &object_id, const std::optional<time_type> &download_deadline);

    std::shared_ptr<Curl> curl() {return m_curl;};
    //static int client_notify_cb(int status, ogs_sbi_response_t *response, void *data);
    static std::vector<unsigned char> convertToVector(const std::string &str);

protected:
    virtual void doObjectIngest();

private:
    void sortListByPolicy();
    std::list<IngestItem> m_fetchList;
    std::shared_ptr<Curl> m_curl;
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_MBSTF_PULL_OBJECT_INGESTER_HH_ */
