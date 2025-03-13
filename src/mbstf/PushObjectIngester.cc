/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Push Object Ingester
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
 * Author(s): Dev Audsin <dev.audsin@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <arpa/inet.h>
#include <sys/socket.h>

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
#include "hash.hh"
#include "ObjectStore.hh"

#include "PushObjectIngester.hh"

MBSTF_NAMESPACE_START

/********************** PushObjectIngester::Request ***********************/

PushObjectIngester::Request::Request(struct MHD_Connection *mhd_connection, PushObjectIngester &poi)
    :m_mhdConnection(mhd_connection)
    ,m_mhdResponse(nullptr)
    ,m_pushObjectIngester(poi)
    ,m_objectId()
    ,m_method()
    ,m_urlPath()
    ,m_protocolVersion()
    ,m_bodyBlocks()
    ,m_totalBodySize(0)
    ,m_statusCode(0)
    ,m_errorReason()
    ,m_noMoreBodyData(false)
    ,m_mutex(new std::recursive_mutex)
    ,m_condVar()
{
}

bool PushObjectIngester::Request::addBodyBlock(const std::vector<unsigned char> &body_block)
{
    std::lock_guard<std::recursive_mutex> lock(*m_mutex);
    m_bodyBlocks.push_back(body_block);
    m_totalBodySize += body_block.size();

    if (m_totalBodySize > 65536) {
        return false;
    }

    return true;
}



void PushObjectIngester::Request::completed(struct MHD_Connection *connection,
                                            enum MHD_RequestTerminationCode term_code)
{
    std::lock_guard<std::recursive_mutex> lock(*m_mutex);

    ogs_info("PushObjectIngester::Request::terminated(%p, %u)", connection, term_code);
    ogs_info("End of request for %s", std::string(m_urlPath).c_str());
    m_noMoreBodyData = true;
    m_condVar.notify_all();
}


static MHD_Result gatherHeaders(void *cls, MHD_ValueKind kind, char const *key, char const *value)
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
    std::lock_guard<std::recursive_mutex> lock(*m_mutex);

    const char *ret = MHD_lookup_connection_value(m_mhdConnection, MHD_HEADER_KIND, field.c_str());

    if (ret == nullptr) {
        return std::nullopt; // No value found
    }

    return std::string(ret); // Wrap the string in std::optional
}

void PushObjectIngester::Request::processRequest() {
    //const char *contentLength = MHD_lookup_connection_value(m_mhdConnection, MHD_HEADER_KIND, MHD_HTTP_HEADER_CONTENT_LENGTH);
    const char *contentType = MHD_lookup_connection_value(m_mhdConnection, MHD_HEADER_KIND, MHD_HTTP_HEADER_CONTENT_TYPE);
    auto lastModified = std::chrono::system_clock::now();

    std::string url(m_urlPath);
    if (url.front() == '/') {
        url = m_pushObjectIngester.getIngestServerPrefix() + url.substr(1,url.size()-1);
    }

    ObjectStore::Metadata metadata(contentType, url, url, m_urlPath, lastModified, m_pushObjectIngester.getIngestServerPrefix());
    metadata.cacheExpires(std::chrono::system_clock::now() + std::chrono::minutes(ObjectStore::Metadata::cacheExpiry()));

    // Pull all body blocks together into one vector
    std::vector<unsigned char> body;
    ogs_debug("Building body of %zu bytes", m_totalBodySize);
    body.reserve(m_totalBodySize);
    for (auto& block : m_bodyBlocks) {
        ogs_debug("Adding body block of %zu bytes", block.size());
        body.insert(body.end(), block.begin(), block.end());
    }

    m_pushObjectIngester.objectStore().addObject(m_objectId, std::move(body), std::move(metadata));
    m_statusCode = 200;
}

void PushObjectIngester::Request::requestHandler(struct MHD_Connection *connection)
{
    std::lock_guard<std::recursive_mutex> lock(*m_mutex);
    m_mhdConnection = connection;
    m_mhdResponse = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);

    ogs_debug("Request::requestHandler: url = %s, method = %s", m_urlPath.c_str(), m_method.c_str());

    //if (m_urlPath.substr(0, 4) != "http" || m_urlPath.substr(0, 2) == "//") {
    //    setError(404, "Not Found");
    //} else
    if (m_method != "PUSH" && m_method != "PUT" && m_method != "POST") {
        setError(405, "Method Not Allowed");
    } else {
        processRequest();
    }

    ogs_info("Queue response (%u) for %s to microhttpd", m_statusCode, m_urlPath.c_str());
    // Queue the request
    MHD_queue_response(connection, m_statusCode, m_mhdResponse);
    MHD_destroy_response(m_mhdResponse);
}

/********************** PushObjectIngester::ObjectPushEvent ***************/

PushObjectIngester::ObjectPushEvent *PushObjectIngester::ObjectPushEvent::makeStartEvent(const std::shared_ptr<Request> &request)
{
    return new PushObjectIngester::ObjectPushEvent("ObjectPushStart", request);
}

PushObjectIngester::ObjectPushEvent *PushObjectIngester::ObjectPushEvent::makeBlockReceivedEvent(const std::shared_ptr<Request> &request)
{
    return new PushObjectIngester::ObjectPushEvent("ObjectPushBlockReceived", request);
}

PushObjectIngester::ObjectPushEvent *PushObjectIngester::ObjectPushEvent::makeTrailersReceivedEvent(const std::shared_ptr<Request> &request)
{
    return new PushObjectIngester::ObjectPushEvent("ObjectPushTrailersReceived", request);
}

PushObjectIngester::ObjectPushEvent::~ObjectPushEvent()
{
}

PushObjectIngester::ObjectPushEvent::ObjectPushEvent(const std::string &typ, const std::shared_ptr<Request> &request)
    :Event(typ)
    ,m_request(request)
{
}

/********************** PushObjectIngester ********************************/

PushObjectIngester::~PushObjectIngester()
{
    stop();
    abort();
}

static void request_completion_callback(void *cls, struct MHD_Connection *connection, void **con_cls, enum MHD_RequestTerminationCode termination_code)
{
    PushObjectIngester *ingester = reinterpret_cast<PushObjectIngester*>(cls);

    // Convert *con_cls into a shared pointer
    std::shared_ptr<PushObjectIngester::Request> &req = *reinterpret_cast<std::shared_ptr<PushObjectIngester::Request>*>(*con_cls);

    ogs_info("request_completion_callback[%u])", termination_code);

    req->completed(connection, termination_code);

    ingester->removeRequest(req);

    delete reinterpret_cast<std::shared_ptr<PushObjectIngester::Request>*>(*con_cls);
    *con_cls = nullptr;
}

static MHD_Result handle_request(void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls)
{
    PushObjectIngester *ingester = reinterpret_cast<PushObjectIngester*>(cls);
    ogs_debug("handle_request[%p, %p, %s, %s, %s, %p, %lu, %p]", cls, connection, url, method, version, upload_data, *upload_data_size, *con_cls);

    if (*con_cls == nullptr) {
        ogs_debug("handle_request: creating new PushObjectIngester::Request('%s', '%s', '%s')", url, method, version);
        PushObjectIngester::Request *req = new PushObjectIngester::Request(connection, *ingester);
        req->urlPath(url);
        req->method(method);
        req->protocolVersion(version);
        std::shared_ptr<PushObjectIngester::Request> *req_ptr = new std::shared_ptr<PushObjectIngester::Request>(req);
        *con_cls = req_ptr;
        ingester->addRequest(*req_ptr);
        return MHD_YES;
    }

    std::shared_ptr<PushObjectIngester::Request> req = *reinterpret_cast<std::shared_ptr<PushObjectIngester::Request>*>(*con_cls);

    if (*upload_data_size > 0) {
        ogs_debug("handle_request: Adding %zu bytes of ingest data", *upload_data_size);
        req->addBodyBlock(std::vector<unsigned char>(upload_data, upload_data + *upload_data_size));
        *upload_data_size = 0;
    } else if (*upload_data_size == 0) {
        ogs_debug("handle_request: Completed request processing.");
        ogs_info("Received client %s request for %s", method, url);
        req->requestHandler(connection);
    } else if (*upload_data_size < 0) {
        ogs_info("handle_request: Request cancelled.");
        *upload_data_size = 0;
        return MHD_NO;
    }

    ogs_debug("handle_request: Request processed.");

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
                                    NULL, NULL,
                                    handle_request, this,
                                    MHD_OPTION_NOTIFY_COMPLETED, request_completion_callback, this,
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
    {
        std::lock_guard<std::recursive_mutex> lock(m_mtx);
        m_activeRequests.push_back(req);
        ogs_debug( "Added new request, there are now %ld active requests", m_activeRequests.size());
    }
    ObjectPushEvent *evt = ObjectPushEvent::makeStartEvent(req);
    if (sendEventSynchronous(*evt)) {
        ogs_debug("Request accepted, receiving...");
    } else {
        // error - do immediate client response
    }
    delete evt;
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
}

void PushObjectIngester::doObjectIngest() {
    start();
}

const std::string &PushObjectIngester::getIngestServerPrefix()
{
    if (m_urlPrefix.empty()) {
        {
            std::lock_guard<std::recursive_mutex> lock(m_mtx);
            while(!m_mhdDaemon) m_condVar.wait(m_mtx);
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
                    ogs_error("The Push Server address family not understood %i", m_sockaddr.ss_family);
                    m_urlPrefix.clear();
                    return m_urlPrefix;
                }
                ogs_info("PUSH SERVER PORT: %d", m_port);

                // Get the IP address
                char host[NI_MAXHOST];
                if (getnameinfo(reinterpret_cast<struct sockaddr*>(&m_sockaddr), addr_len, host, sizeof(host), nullptr, 0, 0) == 0) {
                    m_domain = host;
                }
                if (getnameinfo(reinterpret_cast<struct sockaddr*>(&m_sockaddr), addr_len, host, sizeof(host), nullptr, 0, NI_NUMERICHOST) == 0) {
                    m_IPAddress = host;
                }
                if (!m_domain.empty()) {
                    m_urlPrefix = "http://" + m_domain + ":" + std::to_string(m_port) + "/";
                } else if (!m_IPAddress.empty()) {
                    m_urlPrefix = "http://" + m_IPAddress + ":" + std::to_string(m_port) + "/";
                }
            }
            ogs_info("GETSOCKNAME FAILED");
            ogs_info("getsockname error: %d", errno);
        }
    }

    return m_urlPrefix;
}

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
