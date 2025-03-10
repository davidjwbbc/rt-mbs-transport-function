/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSTF Push Object Ingester
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <arpa/inet.h>
#include <memory>
#include <stdexcept>
#include <utility>
#include <chrono>
#include <iostream>
#include <string>
#include <microhttpd.h>
#include <netdb.h>
#include <uuid/uuid.h>
#include "common.hh"
#include "App.hh"
#include "PushObjectIngester.hh"
#include "hash.hh"
#include "ObjectStore.hh"
#include <sys/socket.h>


//static MHD_Result gatherHeaders(void *cls, MHD_ValueKind kind, char const *key, char const *value);

MBSTF_NAMESPACE_START

class ObjectStore;


PushObjectIngester::Request::Request(struct MHD_Connection *mhd_connection, PushObjectIngester &poi)
    :m_mhdConnection(mhd_connection)
   ,m_pushObjectIngester(poi)	
{

}

bool PushObjectIngester::Request::addBodyBlock(const std::vector<unsigned char> &body_block)
{

    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_bodyBlocks.push_back(body_block);

    size_t total_size = 0;
    for (const auto& block : m_bodyBlocks) {
        total_size += block.size();
    }

    if (total_size > 65536) {
        return false; 
    }

    return true; 
}



void PushObjectIngester::Request::terminated(struct MHD_Connection *connection,
                             enum MHD_RequestTerminationCode term_code)
{
    {    
    
        std::lock_guard<std::recursive_mutex> lock(m_mutex);

        ogs_info("PushObjectIngester::Request::terminated(%p, %u)", connection, term_code);
        ogs_info("End of request for %s", std::string(m_urlPath).c_str());
        m_noMoreBodyData = true;
    }
    m_condVar.notify_all();
}


extern "C" MHD_Result gatherHeaders(void *cls, MHD_ValueKind kind, char const *key, char const *value)
{
    std::list<std::pair<std::string, std::string> > *headers =
        reinterpret_cast<std::list<std::pair<std::string, std::string> >*>(cls);

    headers->push_back(std::make_pair(std::string(key), std::string(value)));

    return MHD_YES;
}

bool PushObjectIngester::Request::setError(unsigned int status_code, const std::string &reason)
{

    if (m_noMoreBodyData) return false;
    m_statusCode = status_code;
    m_errorReason = reason;
    return true;
}

std::optional<std::string> PushObjectIngester::Request::getHeader(const std::string &field) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    const char *ret = MHD_lookup_connection_value(m_mhdConnection,
                                                  MHD_HEADER_KIND,
                                                  field.c_str());

    if (ret == nullptr) {
        return std::nullopt; // No value found
    }

    return std::string(ret); // Wrap the string in std::optional
}

void PushObjectIngester::Request::processRequest() {
    //const char *contentLength = MHD_lookup_connection_value(m_mhdConnection, MHD_HEADER_KIND, MHD_HTTP_HEADER_CONTENT_LENGTH);
    const char *contentType = MHD_lookup_connection_value(m_mhdConnection, MHD_HEADER_KIND, MHD_HTTP_HEADER_CONTENT_TYPE);
    auto lastModified = std::chrono::system_clock::now();

    ObjectStore::Metadata metadata(contentType, m_urlPath, m_urlPath, nullptr, lastModified);
    metadata.cacheExpires(std::chrono::system_clock::now() + std::chrono::minutes(ObjectStore::Metadata::cacheExpiry()));
	 for (auto& block : m_bodyBlocks) {
             m_pushObjectIngester.objectStore().addObject(m_objectId, std::move(block), std::move(metadata));
         }
         //m_pushObjectIngester.objectStore().addObject(m_objectId, std::move(m_bodyBlocks), std::move(metadata));
    
}

void PushObjectIngester::Request::requestHandler(struct MHD_Connection *connection)
{
    
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_mhdConnection = connection;
    m_mhdResponse = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);

    if (m_urlPath.substr(0, 4) != "http" || m_urlPath.substr(0, 2) == "//")
    {
        setError(404, "Not Found");
    } else if ( m_method != "PUSH" || m_method != "PUT")
    {
        setError(501, "Not Implemented");
    } else {
        processRequest();
    }

    ogs_info("Queue response (%u) for %s to microhttpd", m_statusCode, std::string(m_urlPath).c_str());
    // Queue the request
    MHD_queue_response(connection, m_statusCode, m_mhdResponse);
    MHD_destroy_response(m_mhdResponse);
    ogs_info("PushObjectIngester::Request::requestHandler: end");
}

PushObjectIngester::~PushObjectIngester() 
{ 
    stop(); 
    abort();
}


extern "C" void requestTerminationCallback(void *cls, struct MHD_Connection *connection, void **con_cls, enum MHD_RequestTerminationCode termination_code)
{
    PushObjectIngester *ingester = reinterpret_cast<PushObjectIngester*>(cls);

    // Convert *con_cls into a shared pointer
    std::shared_ptr<PushObjectIngester::Request> req = std::static_pointer_cast<PushObjectIngester::Request>(*reinterpret_cast<std::shared_ptr<void>*>(con_cls));

    ogs_info("requestTerminationCallback[%u])", termination_code);

    req->terminated(connection, termination_code);

    ingester->removeRequest(req);

    delete reinterpret_cast<std::shared_ptr<PushObjectIngester::Request>*>(*con_cls);
    *con_cls = nullptr;
}



extern "C" MHD_Result handleRequest(void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls)
{
    PushObjectIngester *ingester = reinterpret_cast<PushObjectIngester*>(cls);
    ogs_debug("handleRequest[%p, %p, %s, %s, %s, %p, %lu, %p]", cls, connection, url, method, version, upload_data, *upload_data_size, *con_cls);
    std::shared_ptr<PushObjectIngester::Request> req;

    if (*con_cls == 0) {
        ogs_debug("handleRequest: creating new PushObjectIngester::Request('%s', '%s', '%s')", url, method, version);
	req.reset(new PushObjectIngester::Request(connection, *ingester));
	req->urlPath(std::string(url));
	req->method(std::string(method));
	req->protocolVersion(version);
        //*con_cls = req;
	*con_cls = new std::shared_ptr<com::fiveg_mag::ref_tools::mbstf::PushObjectIngester::Request>(req);
	ingester->addRequest(req);
    } else {
	//req = reinterpret_cast<PushObjectIngester::Request*>(*con_cls);
	auto req = *reinterpret_cast<std::shared_ptr<PushObjectIngester::Request>*>(con_cls);
    }

    if (*upload_data_size) {
        ogs_info("handleRequest: Adding ingest data");
	req->addBodyBlock(std::vector<unsigned char>(upload_data, upload_data + *upload_data_size));
    } else {
        ogs_info("handleRequest: Perform request processing.");
        ogs_info("Received client request for %s", url);
	req->requestHandler(connection);
    }

    if (*upload_data_size < 0) {
        ogs_info("requestHandle: Request cancelled.");
	*upload_data_size = 0;
	return MHD_NO;
    }

    ogs_info("requestHandle: Request processed.");

    return MHD_YES;
}


bool PushObjectIngester::start()
{
    if (m_mhdDaemon) return false;
   
        	

    ogs_info("PushObjectIngester::start(): Starting MHD");
    m_sockaddr.ss_family = AF_INET;
    struct sockaddr_in *sockaddr = reinterpret_cast<struct sockaddr_in *>(&m_sockaddr);
    sockaddr->sin_addr.s_addr = INADDR_ANY;
 /*
     if (inet_pton(AF_INET, "127.0.0.1", &sockaddr->sin_addr) <= 0) {
            ogs_info("inet_pton error\n");
            return 1;
        }
        sockaddr->sin_port = htons(9090);
	*/

    sockaddr->sin_port = htons(0);

    	
    {
          std::lock_guard<std::recursive_mutex> lock(m_mtx);	     
          m_mhdDaemon = MHD_start_daemon(
		     MHD_USE_SELECT_INTERNALLY,
                     0,
                     NULL, 
                     NULL, 
                     handleRequest, 
                    this, 
                    MHD_OPTION_NOTIFY_COMPLETED, requestTerminationCallback, this,
                    MHD_OPTION_SOCK_ADDR, (union MHD_DaemonInfo *)sockaddr,
                    MHD_OPTION_END 
                    );
         m_condVar.notify_all();	  
    }
    ogs_debug("PushObjectIngester::start(): Started MHD (%p)", m_mhdDaemon);

    return m_mhdDaemon != 0;
}



bool PushObjectIngester::stop()
{
    std::lock_guard<std::recursive_mutex> lock(m_mtx);

    ogs_debug("PushObjectIngester::stop(): Stopping MHD");
    
    if (m_mhdDaemon == 0) return false;

    //delete m_sockaddr;
    MHD_stop_daemon(m_mhdDaemon);
    //delete m_sockaddr;


    m_activeRequests.clear();

    // Forget the old daemon
    m_mhdDaemon = 0;

    return true;
}

void PushObjectIngester::addRequest(std::shared_ptr<PushObjectIngester::Request> req)
{
   
    std::lock_guard<std::recursive_mutex> lock(m_mtx);
    m_activeRequests.push_back(req);
    ogs_debug( "Added new request, there are now %ld active requests", m_activeRequests.size());
}

void PushObjectIngester::removeRequest(std::shared_ptr<PushObjectIngester::Request> req)
{

    std::lock_guard<std::recursive_mutex> lock(m_mtx);
    for (std::list<std::shared_ptr<PushObjectIngester::Request> >::iterator it = m_activeRequests.begin();
	 it != m_activeRequests.end();
	 ++it) {
	if (*it == req) {
	    it = m_activeRequests.erase(it);
	    ogs_info("Removing request. There are now %ld active requests.", m_activeRequests.size());
            break;
        }
    }
    //delete req;
}

void PushObjectIngester::doObjectIngest() {
    start();
}

std::string PushObjectIngester::getIngestServerInfo()
{
    {
         std::lock_guard<std::recursive_mutex> lock(m_mtx);
         while(!m_mhdDaemon)
	     m_condVar.wait(m_mtx);	 
    }
    const union MHD_DaemonInfo *daemon_info = MHD_get_daemon_info(m_mhdDaemon, MHD_DAEMON_INFO_LISTEN_FD);
    if (daemon_info != nullptr && daemon_info->listen_fd != -1) {
        int sock_fd = daemon_info->listen_fd;
        socklen_t addr_len = sizeof(m_sockaddr);
        if (getsockname(sock_fd, reinterpret_cast<struct sockaddr*>(&m_sockaddr), &addr_len) == 0) {
            switch(m_sockaddr.ss_family ) {
            case AF_INET:
                {    
	            struct sockaddr_in *sin = reinterpret_cast<struct sockaddr_in*>(&m_sockaddr);
		    m_port = ntohs(sin->sin_port);
		}
                break;
            case AF_INET6:
		{
		    struct sockaddr_in6 *sin6 = reinterpret_cast<struct sockaddr_in6*>(&m_sockaddr);
                    m_port = ntohs(sin6->sin6_port);
		}
                break;
            default:
	        ogs_error("Yout Push Server address family not understood %i", m_sockaddr.ss_family);
		return std::string();
	    }
            ogs_info("PUSH SERVER PORT: %d", m_port);

            // Get the IP address
            char host[NI_MAXHOST];
            char domain[NI_MAXHOST];
            if (getnameinfo(reinterpret_cast<struct sockaddr*>(&m_sockaddr), addr_len, host, sizeof(host), nullptr, 0, NI_NUMERICHOST) == 0) {
                m_IPAddress = host;
		return m_IPAddress + ":" + std::to_string(m_port);
            }
            if (getnameinfo(reinterpret_cast<struct sockaddr*>(&m_sockaddr), addr_len, domain, sizeof(domain), nullptr, 0, 0) == 0) {
                m_domain = domain;
		return m_domain + ":" + std::to_string(m_port);
            } 
        }
        ogs_info("GETSOCKNAME FAILED");
        ogs_info("getsockname error: %d", errno);
    }

    return std::string();
}

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */ 

