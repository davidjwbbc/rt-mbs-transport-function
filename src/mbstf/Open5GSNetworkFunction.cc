/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS Application interface
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include "ogs-app.h"
#include "ogs-sbi.h"

#include <memory>
#include <stdexcept>
#include <netdb.h>  
#include <sys/types.h>
#include <sys/socket.h>

#include "common.hh"
#include "App.hh"
#include "TimerFunc.hh"
#include "Open5GSEvent.hh"
#include "Open5GSTimer.hh"
#include "Open5GSYamlDocument.hh"

#include "Open5GSNetworkFunction.hh"
#include "MBSTFNetworkFunction.hh"
#include "MBSTFEventHandler.hh"
#include "MBSTFDistributionSession.hh"


MBSTF_NAMESPACE_START

Open5GSNetworkFunction::Open5GSNetworkFunction()
{
    if (ogs_env_set("TZ", "UTC") != OGS_OK) {
        throw std::runtime_error("Failed to set clock to UTC");
    }
    if (ogs_env_set("LC_TIME", "C") != OGS_OK) {
        throw std::runtime_error("Failed to set time locale to C");
    }
    
    //ogs_sbi_context_init(nfType());
}

Open5GSNetworkFunction::~Open5GSNetworkFunction()
{
    if(m_serviceName) ogs_free(m_serviceName);
    if(m_supportedFeatures) ogs_free(m_supportedFeatures);
    if(m_apiVersion) ogs_free(m_apiVersion);
    delete[] m_serverName;
	
}

static void timer_function(void *real_fn)
{
    TimerFunc *timer_func = reinterpret_cast<TimerFunc*>(real_fn);
    timer_func->trigger();
}

std::shared_ptr<Open5GSTimer> Open5GSNetworkFunction::addTimer(TimerFunc &timer_func)
{
    ogs_timer_t *timer = ogs_timer_add(ogs_app()->timer_mgr, timer_function, &timer_func);
    std::shared_ptr<Open5GSTimer> ret(nullptr);

    if (timer) {
        ret.reset(new Open5GSTimer(timer));
    }

    return ret;
}

void Open5GSNetworkFunction::removeTimer(const std::shared_ptr<Open5GSTimer> &timer)
{
    ogs_timer_delete(timer->ogsTimer());
}

Open5GSYamlDocument Open5GSNetworkFunction::configFileDocument() const
{
    return Open5GSYamlDocument(ogs_app()->document);
}

void Open5GSNetworkFunction::pushEvent(const std::shared_ptr<Open5GSEvent> &event)
{
    ogs_queue_push(ogs_app()->queue, event->ogsEvent());
}

bool Open5GSNetworkFunction::configureLoggingDomain()
{
    return ogs_log_config_domain(ogs_app()->logger.domain, ogs_app()->logger.level) == OGS_OK;
}

bool Open5GSNetworkFunction::sbiParseConfig(const char *app_section, const char *nrf_section, const char *scp_section)
{
    return ogs_sbi_context_parse_config(app_section, nrf_section, scp_section) == OGS_OK;
}

static int server_cb(ogs_sbi_request_t *request, void *data)
{
    ogs_event_t *e = NULL;
    int rv;

    ogs_assert(request);
    ogs_assert(data);

    e = ogs_event_new(OGS_EVENT_SBI_SERVER);
    ogs_assert(e);

    e->sbi.request = request;
    e->sbi.data = data;

    rv = ogs_queue_push(ogs_app()->queue, e);
    if (rv != OGS_OK) {
        ogs_error("ogs_queue_push() failed:%d", (int)rv);
        ogs_sbi_request_free(request);
        ogs_event_free(e);
        return OGS_ERROR;
    }

    return OGS_OK;
}

bool Open5GSNetworkFunction::setNFServiceInfo(const char *serviceName, const char *supportedFeatures, const char *apiVersion, ogs_sockaddr_t *addr)
{
    m_serviceName = ogs_strdup(serviceName);
    m_supportedFeatures = ogs_strdup(supportedFeatures);
    m_apiVersion = ogs_strdup(apiVersion);
    ogs_assert(OGS_OK == ogs_copyaddrinfo(&m_addr, addr));
    return true;

}

int Open5GSNetworkFunction::setNFService() {
    ogs_uuid_t uuid;
    char id[OGS_UUID_FORMATTED_LENGTH + 1];
    ogs_sbi_nf_service_t *nf_service = NULL;
    size_t j;

    ogs_uuid_get(&uuid);
    ogs_uuid_format(id, &uuid);

    ogs_info("NF UUID: %s", id);

    nf_service = ogs_sbi_nf_service_add(ogs_sbi_self()->nf_instance, id, m_serviceName,
                                        ogs_app()->sbi.server.no_tls == false ? OpenAPI_uri_scheme_https : OpenAPI_uri_scheme_http);
    ogs_assert(nf_service);

    addAddressesToNFService(nf_service, m_addr);
    ogs_sbi_nf_service_add_version(nf_service, OGS_SBI_API_V1, m_apiVersion, NULL);
    nf_service->supported_features = ogs_strdup(m_supportedFeatures);
    ogs_info("MBSTF Service [%s]", nf_service->name);
    if (!nf_service) return OGS_ERROR;

    return OGS_OK;
}

int Open5GSNetworkFunction::setServerName(void) {

    ogs_sbi_server_t *server = NULL;
    char server_name[NI_MAXHOST];

    ogs_list_for_each(&ogs_sbi_self()->server_list, server) {

        ogs_sockaddr_t *advertise = NULL;
        int res = 0;

        advertise = server->advertise;
        if (!advertise)
            advertise = server->node.addr;
        ogs_assert(advertise);
	res = getnameinfo((struct sockaddr *) &advertise->sa,
                      ogs_sockaddr_len(advertise),
                      server_name, NI_MAXHOST,
                      NULL, 0, NI_NAMEREQD);

        if(res) {
            ogs_debug("Unable to retrieve server name: %d\n", res);
            continue;
        } else {
            ogs_debug("node=%s", server_name);
            ogs_info("SERVER NAME NODE=%s", server_name);
	    // Allocate and copy server name 
	    m_serverName = new char[strlen(server_name) + 1]; 
	    strcpy(m_serverName, server_name);
            return 1;
        }
    }
    return 0;
}



bool Open5GSNetworkFunction::sbiOpen()
{
    ogs_sbi_nf_instance_t *nf_instance = ogs_sbi_self()->nf_instance;

    ogs_sbi_nf_fsm_init(nf_instance);
    ogs_sbi_nf_instance_build_default(nf_instance);

    if (setNFService()!= OGS_OK) {
        return OGS_ERROR;
    }

    nf_instance = ogs_sbi_self()->nrf_instance;
    if (nf_instance) {
        ogs_sbi_nf_fsm_init(nf_instance);
    }

    return ogs_sbi_server_start_all(server_cb) == OGS_OK;
}

void Open5GSNetworkFunction::sbiClose()
{
    ogs_sbi_client_stop_all();
    ogs_sbi_server_stop_all();
}


bool Open5GSNetworkFunction::startEventHandler()
{
    m_eventThread = ogs_thread_create(Open5GSNetworkFunction::eventThread, NULL);
    return !!m_eventThread;
}

void Open5GSNetworkFunction::stopEventHandler()
{
    if(m_eventThread)  ogs_thread_destroy(m_eventThread);
}


void Open5GSNetworkFunction::eventThread(void *data)
{
    ogs_fsm_t fsm;
    int rv;

    ogs_fsm_init(&fsm, reinterpret_cast<void*>(Open5GSNetworkFunction::stateInitial),
                       reinterpret_cast<void*>(Open5GSNetworkFunction::stateFinal),
                       reinterpret_cast<void*>(0));

    do {
        ogs_pollset_poll(ogs_app()->pollset, ogs_timer_mgr_next(ogs_app()->timer_mgr));
        ogs_timer_mgr_expire(ogs_app()->timer_mgr);
        do {
            ogs_event_t *e = NULL;
            rv = ogs_queue_trypop(ogs_app()->queue, (void**)&e);
            ogs_assert(rv != OGS_ERROR);

            if (rv == OGS_DONE)
                goto done;

            if (rv == OGS_RETRY)
                break;

            ogs_assert(e);

            ogs_fsm_dispatch(&fsm, e);

            ogs_event_free(e);
        } while (true);
    } while (true);
done:
    ogs_fsm_fini(&fsm, 0);
}


void Open5GSNetworkFunction::addAddressesToNFService(ogs_sbi_nf_service_t *nf_service, ogs_sockaddr_t *addrs)
{
    ogs_sockaddr_t *addr = NULL;

    for (ogs_copyaddrinfo(&addr, addrs); addr && nf_service->num_of_addr < OGS_SBI_MAX_NUM_OF_IP_ADDRESS;
         addr = addr->next) {
        bool is_port = true;
        int port = 0;

        if (addr->ogs_sa_family == AF_INET) {
            nf_service->addr[nf_service->num_of_addr].ipv4 = addr;
        } else if (addr->ogs_sa_family == AF_INET6) {
            nf_service->addr[nf_service->num_of_addr].ipv6 = addr;
        } else {
            continue;
        }

        port = OGS_PORT(addr);
        if (nf_service->scheme == OpenAPI_uri_scheme_https) {
            if (port == OGS_SBI_HTTPS_PORT) is_port = false;
        } else if (nf_service->scheme == OpenAPI_uri_scheme_http) {
            if (port == OGS_SBI_HTTP_PORT) is_port = false;
        }

        nf_service->addr[nf_service->num_of_addr].is_port = is_port;
        nf_service->addr[nf_service->num_of_addr].port = port;

        nf_service->num_of_addr++;
    }
}

void Open5GSNetworkFunction::stateInitial(ogs_fsm_t *s, ogs_event_t *e)
{
    ogs_assert(s);
    OGS_FSM_TRAN(s, Open5GSNetworkFunction::stateFunctional);
}

void Open5GSNetworkFunction::stateFunctional(ogs_fsm_t *s, ogs_event_t *e)
{
    const App &app = App::self();
    Open5GSFSM fsm(s);
    Open5GSEvent event(e);
    app.dispatchEvent(fsm, event);

    int rv;
    ogs_sbi_stream_t *stream = NULL;
    ogs_sbi_request_t *request = NULL;

    ogs_sbi_nf_instance_t *nf_instance = NULL;
    ogs_sbi_subscription_data_t *subscription_data = NULL;
    ogs_sbi_response_t *response = NULL;
    ogs_sbi_message_t *message = NULL;
    ogs_sbi_xact_t *sbi_xact = NULL;

    MBSTFDistributionSession::processEvent(e);

    mbstf_sm_debug(e);

    message = (ogs_sbi_message_t*)ogs_calloc(1, sizeof(*message));

    ogs_assert(s);

    switch (event.ogsEvent()->id) {
        case OGS_FSM_ENTRY_SIG:
            ogs_info("[%s] MBSTF Running", ogs_sbi_self()->nf_instance->id);
            break;

        case OGS_FSM_EXIT_SIG:
            break;


        case OGS_EVENT_SBI_SERVER:
            request = event.ogsEvent()->sbi.request;
            ogs_assert(request);
            stream = (ogs_sbi_stream_t*)event.ogsEvent()->sbi.data;
            ogs_assert(stream);

            rv = ogs_sbi_parse_header(message, &request->h);
            if (rv != OGS_OK) {
                ogs_error("ogs_sbi_parse_header() failed");
		ogs_assert(true == ogs_sbi_server_send_error(
                                stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST,
                                message, "cannot parse HTTP message", NULL));
                ogs_sbi_message_free(message);
                break;
            }

            SWITCH(message->h.service.name)
            CASE(OGS_SBI_SERVICE_NAME_NNRF_NFM)
                if (strcmp(message->h.api.version, OGS_SBI_API_V1) != 0) {
                    ogs_error("Not supported version [%s]", message->h.api.version);
                    ogs_assert(true == ogs_sbi_server_send_error(
                                stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST,
                                message, "Not supported version", NULL));
                    break;
                }
                SWITCH(message->h.resource.component[0])
                CASE(OGS_SBI_RESOURCE_NAME_NF_STATUS_NOTIFY)
                    SWITCH(message->h.method)
                    CASE(OGS_SBI_HTTP_METHOD_POST)
                        ogs_nnrf_nfm_handle_nf_status_notify(stream, message);
                        break;

                    DEFAULT
                        ogs_error("Invalid HTTP method [%s]", message->h.method);
                        ogs_assert(true ==
                                ogs_sbi_server_send_error(stream,
                                        OGS_SBI_HTTP_STATUS_FORBIDDEN, message,
                                        "Invalid HTTP method", message->h.method));
                    END
                    break;

                DEFAULT
                    ogs_error("Invalid resource name [%s]",
                            message->h.resource.component[0]);
                    ogs_assert(true ==
                            ogs_sbi_server_send_error(stream,
                                    OGS_SBI_HTTP_STATUS_BAD_REQUEST, message,
                                    "Invalid resource name",
                                    message->h.resource.component[0]));
                END
                break;

            DEFAULT
                ogs_error("Invalid API name [%s]", message->h.service.name);
	        ogs_assert(true ==
                                ogs_sbi_server_send_error(stream,
                                        OGS_SBI_HTTP_STATUS_BAD_REQUEST, message,
                                        "Invalid API name.", message->h.method));

            END
            if (message) ogs_sbi_message_free(message);
            break;

        case OGS_EVENT_SBI_CLIENT:
            ogs_assert(event.ogsEvent());

            response = event.ogsEvent()->sbi.response;
            ogs_assert(response);
            rv = ogs_sbi_parse_header(message, &response->h);
            if (rv != OGS_OK) {
                ogs_error("ogs_sbi_parse_header() failed");
                ogs_sbi_message_free(message);
                break;
            }
            message->res_status = response->status;

            SWITCH(message->h.service.name)

            CASE(OGS_SBI_SERVICE_NAME_NNRF_NFM)

                SWITCH(message->h.resource.component[0])
                CASE(OGS_SBI_RESOURCE_NAME_NF_INSTANCES)
		    cJSON *nf_profile;
                    OpenAPI_nf_profile_t *nfprofile;

                    nf_instance = (ogs_sbi_nf_instance_t*)e->sbi.data;
                    ogs_assert(nf_instance);

		    if (response->http.content_length && response->http.content){
                       ogs_debug( "response: %s", response->http.content);
                       nf_profile = cJSON_Parse(response->http.content);
                       nfprofile = OpenAPI_nf_profile_parseFromJSON(nf_profile);
                       message->NFProfile = nfprofile;

                       if (!message->NFProfile) {
                           ogs_error("No nf_profile");
                       }
                       cJSON_Delete(nf_profile);
                    }

                    ogs_assert(OGS_FSM_STATE(&nf_instance->sm));

                    event.ogsEvent()->sbi.message = message;
                    //message = NULL;
                    ogs_fsm_dispatch(&nf_instance->sm, event.ogsEvent());
		    //Open5GSFSM fsm(&nf_instance->sm);
                    //app.dispatchEvent(fsm, event);

		    ogs_sbi_response_free(response);
                    break;

                CASE(OGS_SBI_RESOURCE_NAME_SUBSCRIPTIONS)
                    subscription_data = (ogs_sbi_subscription_data_t*)e->sbi.data;
                    ogs_assert(subscription_data);

                    SWITCH(message->h.method)
                    CASE(OGS_SBI_HTTP_METHOD_POST)
                        if (message->res_status == OGS_SBI_HTTP_STATUS_CREATED ||
                                message->res_status == OGS_SBI_HTTP_STATUS_OK) {
                            ogs_nnrf_nfm_handle_nf_status_subscribe(
                                    subscription_data, message);
                        } else {
                            ogs_error("HTTP response error : %d",
                                    message->res_status);
                        }
                        break;

                    CASE(OGS_SBI_HTTP_METHOD_DELETE)
                        if (message->res_status == OGS_SBI_HTTP_STATUS_NO_CONTENT) {
                            ogs_sbi_subscription_data_remove(subscription_data);
                        } else {
                            ogs_error("HTTP response error : %d",
                                    message->res_status);
                        }
                        break;

                    DEFAULT
                        ogs_error("Invalid HTTP method [%s]", message->h.method);
                    END
                    break;

                DEFAULT
                    ogs_error("Invalid resource name [%s]",
                            message->h.resource.component[0]);
                END
                break;

            DEFAULT
                ogs_error("Invalid service name [%s]", message->h.service.name);
                ogs_assert_if_reached();
            END

            if (message) ogs_sbi_message_free(message);
            break;

	case OGS_EVENT_SBI_TIMER:
            ogs_assert(event.ogsEvent());

            switch(event.ogsEvent()->timer_id) {
                case OGS_TIMER_NF_INSTANCE_REGISTRATION_INTERVAL:
                case OGS_TIMER_NF_INSTANCE_HEARTBEAT_INTERVAL:
                case OGS_TIMER_NF_INSTANCE_NO_HEARTBEAT:
                case OGS_TIMER_NF_INSTANCE_VALIDITY:
	            {		
                        nf_instance = (ogs_sbi_nf_instance_t*)event.ogsEvent()->sbi.data;
                        ogs_assert(nf_instance);
                        ogs_assert(OGS_FSM_STATE(&nf_instance->sm));

                        ogs_fsm_dispatch(&nf_instance->sm, event.ogsEvent());
			//Open5GSFSM fsm(&nf_instance->sm);
                        //app.dispatchEvent(fsm, event);
                        
			if (OGS_FSM_CHECK(&nf_instance->sm, ogs_sbi_nf_state_exception))
                            ogs_error("State machine exception [%d]", event.ogsEvent()->timer_id);
		    }
		    break;

                case OGS_TIMER_SUBSCRIPTION_VALIDITY:
                    subscription_data = (ogs_sbi_subscription_data_t*)event.ogsEvent()->sbi.data;
                    ogs_assert(subscription_data);

                    ogs_assert(true ==
                            ogs_nnrf_nfm_send_nf_status_subscribe(
                            ogs_sbi_self()->nf_instance->nf_type,
                            subscription_data->req_nf_instance_id,
                            subscription_data->subscr_cond.nf_type,
                            subscription_data->subscr_cond.service_name));


                    ogs_debug("Subscription validity expired [%s]",
                            subscription_data->id);
                    ogs_sbi_subscription_data_remove(subscription_data);
                    break;

                case OGS_TIMER_SBI_CLIENT_WAIT:
                    sbi_xact = (ogs_sbi_xact_t*)e->sbi.data;
                    ogs_assert(sbi_xact);

                    stream = sbi_xact->assoc_stream;

                    ogs_sbi_xact_remove(sbi_xact);

                    ogs_error("Cannot receive SBI message");
                    if (stream) {
                        ogs_assert(true ==
                                ogs_sbi_server_send_error(stream,
                                    OGS_SBI_HTTP_STATUS_GATEWAY_TIMEOUT, NULL,
                                    "Cannot receive SBI message", NULL));
                    }
                    break;

                default:
                    ogs_error("Unknown timer[%s:%d]",
                            ogs_timer_get_name(event.ogsEvent()->timer_id), event.ogsEvent()->timer_id);
            }
            break;

        default:
            ogs_error("No handler for event %s", ogs_event_get_name(event.ogsEvent()));
            break;
    }
    if (message) ogs_free(message);
}

void Open5GSNetworkFunction::stateFinal(ogs_fsm_t *s, ogs_event_t *e)
{
    ogs_assert(s);
}


MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
