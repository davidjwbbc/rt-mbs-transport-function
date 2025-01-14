/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS Application interface
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
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

#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "Utilities.hh"
#include "server.hh"
#include "TimerFunc.hh"
#include "Open5GSEvent.hh"
#include "Open5GSTimer.hh"
#include "Open5GSYamlDocument.hh"

#include "Open5GSNetworkFunction.hh"
#include "MBSTFNetworkFunction.hh"
#include "MBSTFDistributionSession.hh"
#include "openapi/model/CreateReqData.h"
#include "openapi/model/CreateRspData.h"
#include "openapi/model/DistSession.h"
#include "openapi/api/IndividualMBSDistributionSessionApi-info.h"
#include "hash.hh"

using fiveg_mag_reftools::CJson; 
using reftools::mbstf::CreateReqData;
using reftools::mbstf::CreateRspData;
using reftools::mbstf::DistSession;

static const nf_server_interface_metadata_t
nmbstf_distributionsession_api_metadata = {
    NMBSTF_DISTSESSION_API_NAME,
    NMBSTF_DISTSESSION_API_VERSION
};


MBSTF_NAMESPACE_START

MBSTFDistributionSession::MBSTFDistributionSession(CJson *json, bool as_request)
    : m_createReqData(std::make_shared<CreateReqData>(*json, as_request)) {
 
    ogs_uuid_t uuid;
    
    char id[OGS_UUID_FORMATTED_LENGTH + 1];

    ogs_uuid_get(&uuid);
    ogs_uuid_format(id, &uuid);

    std::shared_ptr<DistSession> distSession = m_createReqData->getDistSession();
    distSession->setDistSessionId(std::string(id));

    m_generated = ogs_time_now();
    m_lastUsed = m_generated;

    m_hash = std::string(calculate_DistributionSessionHash(json));
    m_distributionSessionId = std::string(id);

    //App::self().context()->addDistributionSession(m_distributionSessionId, std::shared_ptr<MBSTFDistributionSession> MBSTFDistributionSession)

}

MBSTFDistributionSession::~MBSTFDistributionSession()
{

}


CJson* MBSTFDistributionSession::json(bool as_request = false) const
{
    return new CJson(m_createReqData->toJSON(as_request));	
}

const std::string &MBSTFDistributionSession::id() const
{
    return m_distributionSessionId;	
}

const std::shared_ptr<MBSTFDistributionSession> &MBSTFDistributionSession::find(const std::string &id)
{
    std::map<std::string, std::shared_ptr<MBSTFDistributionSession> > distributionSessions = App::self().context()->getDistributionSessions();   
    auto it = distributionSessions.find(id); 
    if (it != distributionSessions.end()) { 
        return it->second; 
    } else { 
	throw std::out_of_range("MBST Distribution session not found");    
        return nullptr;  
    }
}

char *MBSTFDistributionSession::calculate_DistributionSessionHash(CJson *json)
{
    char *distSessionHash;
    char *distSessionHashed;

    distSessionHash = cJSON_Print(json->exportCJSON());
    ogs_info("distSessionToHash: %s", distSessionHash);

    distSessionHashed = calculate_hash(distSessionHash);
    cJSON_free(distSessionHash);

    return distSessionHashed;
}


bool MBSTFDistributionSession::processEvent(ogs_event_t *e)
{

    //ogs_debug("MBSTFDistributionSession::ProcessEvent: %s", _eventGetName(e));

    static const nf_server_interface_metadata_t *nmbstf_distributionsession_api = &nmbstf_distributionsession_api_metadata;
    const nf_server_app_metadata_t *app_meta = App::self().MBSTFAppMetadata();

    switch (e->id) {
    case OGS_EVENT_SBI_SERVER:
        {
            int rv = 0;
            ogs_sbi_request_t *request = (ogs_sbi_request_t *)e->sbi.request;
            ogs_sbi_message_t message;
            ogs_sbi_stream_t *stream = (ogs_sbi_stream_t*)e->sbi.data;
            ogs_sbi_server_t *server;
            const nf_server_interface_metadata_t *api = NULL;
            //static const Context::ServerType serverTypes[] = {SERVER_DISTRIBUTION_SESSION};
            //Context::ServerType server_found = -1;
            int i;

            ogs_assert(request);
            ogs_assert(stream);

            server = ogs_sbi_server_from_stream(stream);
            ogs_assert(server);

	    /*
	    for (i=0; i<(sizeof(server_types)/sizeof(server_types[0])); i++) {
                if (__does_stream_server_match_server(server, server_types[i])) {
                    server_found = server_types[i];
                    break;
                }
            }
            if (server_found == -1) {
                return false;
            }
	    */


            rv = ogs_sbi_parse_header(&message, &request->h);
            if (rv != OGS_OK) {
                ogs_error("Failed to parse request headers");
                ogs_sbi_message_free(&message);
                break;
            }

            ogs_debug("OGS_EVENT_SBI_SERVER: service=%s, component[0]=%s", message.h.service.name, message.h.resource.component[0]);
            if (message.h.service.name) {
                SWITCH(message.h.service.name)
                CASE("nmbstf-distsession")
                    api = nmbstf_distributionsession_api;
                    break;
		DEFAULT
		    ogs_sbi_message_free(&message);
                    //ogs_sbi_request_free(request);
                    return false;

                END
                if (api == nmbstf_distributionsession_api) {
                    /******** nmbstf-distsession ********/
		    if (strcmp(message.h.api.version, OGS_SBI_API_V1) != 0) {
                        ogs_error("Unsupported API version [%s]", message.h.api.version);
                        ogs_assert(true == nf_server_send_error(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 0, &message,
                                                "Unsupported API version", NULL, NULL, NULL, NULL, api, app_meta));
                        return true;
                    }

                    if (message.h.resource.component[0]) {
                        SWITCH(message.h.resource.component[0])
                        CASE("dist-sessions")
			    SWITCH(message.h.method)
                            CASE(OGS_SBI_HTTP_METHOD_POST)
			        ogs_debug("POST response: status = %i", message.res_status);

				if (message.h.resource.component[2] && !strcmp(message.h.resource.component[2], "dist-session")) {
				} else {
		                    ogs_debug("In MBSTF Distribution session");

				    CJson *distSession = NULL;
				    char *location;
                                    ogs_sbi_response_t *response;
                                    char *body;
				    std::shared_ptr<MBSTFDistributionSession> distributionSession;

				    ogs_debug("Request body: %s", request->http.content);

				    if (!check_http_content_type(request->http,"application/json")) {
                                        ogs_assert(true == nf_server_send_error(stream, OGS_SBI_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE,
                                                                    3, &message, "Unsupported Media Type",
                                                                    "Expected content type: application/json", NULL, NULL, NULL,
                                                                    api, app_meta));
                                        break;
                                    }

				    distSession = new CJson(CJson::parse(request->http.content));

                                    {
                                        char *txt = cJSON_Print(distSession->exportCJSON());
					ogs_debug("Parsed JSON: %s", txt);
                                        cJSON_free(txt);
                                    }

				    if (!distSession) {
                                        char *err = NULL;
                                        err = ogs_msprintf("Unable to parse MBSTF Distribution Session as JSON.");
                                        ogs_error("%s", err);
                                        ogs_assert(true == nf_server_send_error(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1,
                                                                    &message, "Bad Data Reporting Session", err, NULL, NULL, NULL,
                                                                    api, app_meta));
                                        ogs_free(err);
					break;
                                    }

 				    try {

				        distributionSession = std::make_shared<MBSTFDistributionSession>(distSession, true);
				    
				    } catch (std::exception &err) {
                                        ogs_error("Error while populating MBSTF Distribution Session: %s", err.what());
					char *error = ogs_msprintf("Bad request [%s]", err.what());
                                        ogs_error("%s", error);
                                        ogs_assert(true == nf_server_send_error(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 0,
                                                                    &message, "Bad Request", error, NULL, NULL, NULL, api, app_meta));
                                        ogs_free(error);
                                        break;

                                    }


                                    App::self().context()->addDistributionSession(distributionSession->distributionSessionId(), distributionSession);

                                   /*  
				    std::shared_ptr<MBSTFDistributionSession> distributionSession = std::make_shared<MBSTFDistributionSession>(distSession, true);
				    App::self().context()->addDistributionSession(distributionSession->distributionSessionId(), std::shared_ptr<MBSTFDistributionSession> distributionSession);
                                    */
                                    delete distSession;
                                    distSession = nullptr;

				    distributionSession->createRspDataAndSend(distributionSession, api, app_meta, stream, request);

				    /*
				    CJson *createRspData = distributionSession->json(false);
				    body = cJSON_Print(createRspData->exportCJSON());
                                    ogs_debug("Parsed JSON: %s", body);
				    location = ogs_msprintf("%s/%s", request->h.uri, (distributionSession->distributionSessionId()).c_str());
                                    response = nf_server_new_response(location, body?"application/json":NULL,
                                                        distributionSession->generated(),
                                                        distributionSession->hash().c_str(),
                                                        App::self().context()->cacheControl.distMaxAge,
                                                        NULL, api, app_meta);
                                    ogs_assert(response);
                                    nf_server_populate_response(response, body?strlen(body):0, body, OGS_SBI_HTTP_STATUS_CREATED);
                                    ogs_assert(true == ogs_sbi_server_send_response(stream, response));

                                    delete createRspData;
				    createRspData = nullptr;
				    ogs_free(location);
				    */
				    
                                    break;

		                }
			        break;
		            CASE(OGS_SBI_HTTP_METHOD_GET)
			        {
                                    const char *distributionSessionId = message.h.resource.component[1];

                                    if (!distributionSessionId) {
                                        char *err = ogs_msprintf("Invalid resource [%s]", message.h.uri);
                                        ogs_error("%s", err);
                                        ogs_assert(true == nf_server_send_error(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1,
                                                                    &message, "Bad Request", err, NULL, NULL, NULL, api, app_meta));
                                        ogs_free(err);
                                        break;
                                    }
				    try {
				        char *location;
                                        ogs_sbi_response_t *response;
                                        char *body;
					int response_code = 200;

					std::shared_ptr<MBSTFDistributionSession> distSess = MBSTFDistributionSession::find(std::string(distributionSessionId));
			                CJson *distSessionJSON = distSess->json(false);
                                        body = cJSON_Print(distSessionJSON->exportCJSON());
                                        ogs_debug("Parsed JSON: %s", body);
                                        location = ogs_msprintf("%s/%s", request->h.uri, (distSess->distributionSessionId()).c_str());
                                        response = nf_server_new_response(location, body?"application/json":NULL,
                                                        distSess->generated(),
                                                        distSess->hash().c_str(),
                                                        App::self().context()->cacheControl.distMaxAge,
                                                        NULL, api, app_meta);
                                        ogs_assert(response);
                                        nf_server_populate_response(response, body?strlen(body):0, body, response_code);
                                        ogs_assert(true == ogs_sbi_server_send_response(stream, response));
                                        delete distSessionJSON;
					distSessionJSON = nullptr;
                                        ogs_free(location);

				    } catch (const std::out_of_range &e) {
				        char *err = NULL;
                                        OpenAPI_list_t *invalid_params;
                                        static const char *param = "{sessionId}";
                                        char *reason = NULL;

                                        err = ogs_msprintf("MBSTF Distribution Session [%s] does not exist.", distributionSessionId);
                                        ogs_error("%s", err);

                                        reason = ogs_msprintf("Invalid MBSTF Distribution Session identifier [%s]", distributionSessionId);
                                        invalid_params = nf_server_make_invalid_params(param, reason);

                                        ogs_assert(true == nf_server_send_error(stream, OGS_SBI_HTTP_STATUS_NOT_FOUND, 1, &message,
                                                                    "MBSTF Distribution Session not found", err, NULL, invalid_params,
                                                                    NULL, api, app_meta));
                                        ogs_free(err);
                                        ogs_free(reason);

                                   }
				}
                                break;

		            CASE(OGS_SBI_HTTP_METHOD_DELETE)
                                {
				    if (message.h.resource.component[1] && !message.h.resource.component[2]) {
                                        const char *distributionSessionId = message.h.resource.component[1];
                                        try {
					    App::self().context()->deleteDistributionSession(std::string(distributionSessionId));
					    ogs_sbi_response_t *response = nf_server_new_response(NULL, NULL, 0, NULL, 0, NULL, api, app_meta);
                                            nf_server_populate_response(response, 0, NULL, OGS_SBI_HTTP_STATUS_NO_CONTENT);
                                            ogs_assert(response);
                                            ogs_assert(true == ogs_sbi_server_send_response(stream, response));

					} catch (const std::out_of_range &e) {
                                            char *err = NULL;
                                            OpenAPI_list_t *invalid_params;
                                            static const char *param = "{sessionId}";
                                            char *reason = NULL;

                                            err = ogs_msprintf("MBSTF Distribution Session [%s] does not exist.", distributionSessionId);
                                            ogs_error("%s", err);

                                            reason = ogs_msprintf("Invalid MBSTF Distribution Session identifier [%s]", distributionSessionId);
                                            invalid_params = nf_server_make_invalid_params(param, reason);

                                            ogs_assert(true == nf_server_send_error(stream, OGS_SBI_HTTP_STATUS_NOT_FOUND, 2, &message,
                                                                    "MBSTF Distribution Session not found", err, NULL, invalid_params,
                                                                    NULL, api, app_meta));
                                            ogs_free(err);
                                            ogs_free(reason);

                                       }


				    }

				}
                                break;
			    DEFAULT
                                ogs_debug("Invalid method [%s] for %s/%s/%s", message.h.method, message.h.service.name,
                                            message.h.api.version, message.h.resource.component[0]);
			        char *err = ogs_msprintf("Invalid method [%s] for %s/%s/%s", message.h.method,
                                                            message.h.service.name, message.h.api.version,
                                                            message.h.resource.component[0]);
                                ogs_error("%s", err);
                                ogs_assert(true == nf_server_send_error(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, &message,
                                                            "Bad request", err, NULL, NULL, NULL, api, app_meta));
                                ogs_free(err);
                            END
			    break;
			DEFAULT
                            char *err = ogs_msprintf("Unknown object type \"%s\" in Data Reporting request",
                                                        message.h.resource.component[0]);
                            ogs_error("%s", err);
                            ogs_assert(true == nf_server_send_error(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, &message,
                                                        "Bad request", err, NULL, NULL, NULL, api, app_meta));
                            ogs_free(err);
			END
		    }
                }
	    } else {
                    static const char *err = "Missing service name from URL path";
                    ogs_error("%s", err);
                    ogs_assert(true == nf_server_send_error(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 0, &message,
                                                "Missing service name", err, NULL, NULL, NULL, NULL, app_meta));
            }
            ogs_sbi_message_free(&message);
            return true;
	}
	default:
        break;
    }
    return false;
}

void MBSTFDistributionSession::createRspDataAndSend(std::shared_ptr<MBSTFDistributionSession> distributionSession, const nf_server_interface_metadata_t *api, const nf_server_app_metadata_t *app_meta,ogs_sbi_stream_t *stream, ogs_sbi_request_t *request)
{

    char *location;
    ogs_sbi_response_t *response;
    char *body;
    int response_code = 200;

    std::shared_ptr<CreateRspData> createRspData = std::make_shared<CreateRspData>();
    createRspData->setDistSession(this->distSession());

    CJson *createRspDataJSON = new CJson(createRspData->toJSON(false));

    body = cJSON_Print(createRspDataJSON->exportCJSON());
    ogs_debug("Parsed JSON: %s", body);
    location = ogs_msprintf("%s/%s", request->h.uri, (this->distributionSessionId()).c_str());
    response = nf_server_new_response(location, body?"application/json":NULL,
		    this->generated(), this->hash().c_str(),
		    App::self().context()->cacheControl.distMaxAge,
		    NULL, api, app_meta);
    ogs_assert(response);
    nf_server_populate_response(response, body?strlen(body):0, body, response_code);
    ogs_assert(true == ogs_sbi_server_send_response(stream, response));
    ogs_free(location);
    delete createRspDataJSON;
    createRspDataJSON = nullptr;
}



MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
