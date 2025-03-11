#ifndef _MBS_TF_PUSH_OBJECT_INGESTER_HH_
#define _MBS_TF_PUSH_OBJECT_INGESTER_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Push Object Ingester class
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
 * Author(s): Dev Audsin <dev.audsin@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <string>
#include <list>
#include <microhttpd.h>
#include <vector>

#include "common.hh"
#include "ObjectIngester.hh"
#include "SubscriptionService.hh"

MBSTF_NAMESPACE_START

class ObjectStore;
class ObjectController;

class PushObjectIngester : public ObjectIngester, public SubscriptionService {
public:
	
    class Request {
    public:
	Request(struct MHD_Connection *mhd_connection, PushObjectIngester &poi);
	virtual ~Request() {};
        	
	const std::string &method() const { return m_urlPath; };
          
	Request &method(const std::string &method) { m_method = method; return *this; };
        Request &method(std::string &&method) { m_method = std::move(method); return *this; };
        
        const std::string &urlPath() const { return m_urlPath; };
        Request &urlPath(const std::string &url_path) { m_urlPath = url_path; return *this; };
        Request &urlPath(std::string &&url_path) { m_urlPath = std::move(url_path); return *this; };
        
        const std::string &protocolVersion() const { return m_urlPath; };
        Request &protocolVersion(const std::string &proto_ver) { m_protocolVersion = proto_ver; return *this; };
        
	Request &protocolVersion(std::string &&proto_ver) { m_protocolVersion = std::move(proto_ver); return *this;};
	
	std::optional<std::string> getHeader(const std::string &field) const;
	
	bool addBodyBlock(const std::vector<unsigned char> &body_block);
	bool setError(unsigned int status_code = 0, const std::string &reason = std::string());
        void terminated(struct MHD_Connection *connection, enum MHD_RequestTerminationCode term_code);
	virtual void waitClose() {};
	void requestHandler(struct MHD_Connection *connection);
	typedef bool (*HeaderProcessingCallback)(const std::string &key, const std::string &value, void *data);
	void processRequestHeader(HeaderProcessingCallback callback, void *data) const;


    protected:
	//Request(const std::string &url, const std::string &method, const std::string &version);
	virtual void processRequest();

	struct MHD_Connection *m_mhdConnection;
        struct MHD_Response *m_mhdResponse;

    private:
      
        PushObjectIngester &m_pushObjectIngester;
        std::string m_objectId;
        std::string m_method;
        std::string m_urlPath;
        std::string m_protocolVersion;
        //MHD_connection *m_mhdConnection;

        std::list<std::vector<unsigned char> > m_bodyBlocks;

        unsigned int m_statusCode;
        std::string m_errorReason;
        bool m_noMoreBodyData;	

	mutable std::recursive_mutex m_mutex;
        std::condition_variable_any m_condVar; /**< CondVar for new response content/eof */

    };

    PushObjectIngester(ObjectStore& object_store, ObjectController &controller)
      :ObjectIngester(object_store, controller)
      ,m_mhdDaemon(nullptr)
      ,m_sockaddr()
      ,m_activeRequests()
      ,m_IPAddress()
      ,m_domain()
      ,m_port(0)
      ,m_mtx()	
    { startWorker(); };

    bool start();
    bool stop();
    void addRequest(std::shared_ptr<Request> request);
    void removeRequest(std::shared_ptr<Request> request);

    //void addConnection(Request *request);
    //void removeConnection(Request *request);
    std::string getIngestServerInfo();

    virtual ~PushObjectIngester();

    //static int client_notify_cb(int status, ogs_sbi_response_t *response, void *data);

protected:
    virtual void doObjectIngest();

private:
    std::string generateUUID();
    struct MHD_Daemon *m_mhdDaemon;
    struct sockaddr_storage m_sockaddr;
    std::list<std::shared_ptr<Request> > m_activeRequests;
    //std::vector<Request*>  m_connections;
    std::string m_IPAddress;
    std::string m_domain;
    int m_port;
    std::recursive_mutex m_mtx;
    std::condition_variable_any m_condVar; /**< CondVar for new response content/eof */
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_PUSH_OBJECT_INGESTER_HH_ */
