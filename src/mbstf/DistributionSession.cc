/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS Application interface
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 *
 * Licensed under the License terms and conditions for use, reproduction, and
 * distribution of 5G-MAG software (the “License”).  You may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 * https://www.5g-mag.com/reference-tools.  Unless required by applicable law or
 * agreed to in writing, software distributed under the License is distributed on
 * an “AS IS” BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied.
 *
 * See the License for the specific language governing permissions and limitations
 * under the License.
 */

// Open5GS includes
#include "ogs-app.h"
#include "ogs-sbi.h"

// standard template library includes
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>

// App header includes
#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "hash.hh"
#include "MBSTFNetworkFunction.hh"
#include "NfServer.hh"
#include "ObjectListController.hh"
#include "Open5GSEvent.hh"
#include "Open5GSSBIMessage.hh"
#include "Open5GSSBIRequest.hh"
#include "Open5GSSBIResponse.hh"
#include "Open5GSSBIServer.hh"
#include "Open5GSSBIStream.hh"
#include "Open5GSTimer.hh"
#include "Open5GSYamlDocument.hh"
#include "Open5GSNetworkFunction.hh"
#include "openapi/model/CreateReqData.h"
#include "openapi/model/DistSession.h"
#include "openapi/api/IndividualMBSDistributionSessionApi-info.h"
#include "TimerFunc.hh"

// Header include for this class
#include "DistributionSession.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbstf::CreateReqData;
using reftools::mbstf::DistSession;

MBSTF_NAMESPACE_START

static const NfServer::InterfaceMetadata g_nmbstf_distributionsession_api_metadata(
    NMBSTF_DISTSESSION_API_NAME,
    NMBSTF_DISTSESSION_API_VERSION
);

DistributionSession::DistributionSession(CJson &json, bool as_request)
    : m_createReqData(std::make_shared<CreateReqData>(json, as_request)) {

    ogs_uuid_t uuid;

    char id[OGS_UUID_FORMATTED_LENGTH + 1];

    ogs_uuid_get(&uuid);
    ogs_uuid_format(id, &uuid);

    std::shared_ptr<DistSession> distSession = m_createReqData->getDistSession();
    distSession->setDistSessionId(std::string(id));

    m_generated = std::chrono::system_clock::now();
    m_lastUsed = m_generated;

    std::string json_str(json.serialise());
    m_hash = calculate_hash(std::vector<std::string::value_type>(json_str.begin(), json_str.end()));
    m_distributionSessionId = id;

    //App::self().context()->addDistributionSession(m_distributionSessionId, std::shared_ptr<DistributionSession> DistributionSession)

}

DistributionSession::~DistributionSession()
{

}

CJson DistributionSession::json(bool as_request = false) const
{
    return m_createReqData->toJSON(as_request);	
}

const std::shared_ptr<DistributionSession> &DistributionSession::find(const std::string &id)
{
    const std::map<std::string, std::shared_ptr<DistributionSession> > &distributionSessions = App::self().context()->distributionSessions;
    auto it = distributionSessions.find(id);
    if (it == distributionSessions.end()) {
	throw std::out_of_range("MBST Distribution session not found");
    }
    return it->second;
}

bool DistributionSession::processEvent(Open5GSEvent &event)
{
    const NfServer::InterfaceMetadata &nmbstf_distributionsession_api = g_nmbstf_distributionsession_api_metadata;
    const NfServer::AppMetadata &app_meta = App::self().mbstfAppMetadata();

    switch (event.id()) {
    case OGS_EVENT_SBI_SERVER:
        {
            Open5GSSBIRequest request(event.sbiRequest());
            ogs_assert(request);
            Open5GSSBIMessage message;
            Open5GSSBIStream stream(reinterpret_cast<ogs_sbi_stream_t*>(event.sbiData()));
            ogs_assert(stream);
            Open5GSSBIServer server(stream.server());
            ogs_assert(server);
            std::optional<NfServer::InterfaceMetadata> api(std::nullopt);

            try {
                message.parseHeader(request);
            } catch (std::exception &ex) {
                ogs_error("Failed to parse request headers");
                break;
            }

            std::string service_name(message.serviceName());
            std::string resource0(message.resourceComponent(0));
            ogs_debug("OGS_EVENT_SBI_SERVER: service=%s, component[0]=%s", service_name.c_str(), resource0.c_str());
            if (service_name == "nmbstf-distsession") {
                api.emplace(nmbstf_distributionsession_api);
	        } else {
                return false;
            }

            if (api.value() == nmbstf_distributionsession_api) {
                /******** nmbstf-distsession ********/
                std::string api_version(message.apiVersion());
		        if (api_version != OGS_SBI_API_V1) {
                    ogs_error("Unsupported API version [%s]", api_version.c_str());
                    ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 0, message, app_meta,
                                                           api, "Unsupported API version"));
                    return true;
                }

                if (resource0 == "dist-sessions") {
                    std::string method(message.method());
                    const char *ptr_resource1 = message.resourceComponent(1);
                    if (method == OGS_SBI_HTTP_METHOD_POST) {
			            ogs_debug("POST response: status = %i", message.resStatus());
                        //const char *ptr_resource1 = message.resourceComponent(1);
                        /*
		        const char *ptr_resource2 = nullptr;
                        if (ptr_resource1) ptr_resource2 = message.resourceComponent(2);
                        if (ptr_resource2) {
                            std::string resource2(ptr_resource2);
			    if (resource2 != "dist-session") {
			    */
		                ogs_debug("In MBSTF Distribution session");
				std::shared_ptr<DistributionSession> distributionSession;
				ogs_debug("Request body: %s", request.content());
    				if (request.headerValue(OGS_SBI_CONTENT_TYPE, std::string()) != "application/json") {
                                    ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE,
                                                                    3, message, app_meta, api, "Unsupported Media Type",
                                                                    "Expected content type: application/json"));
                                 break;
                                }
				

                                CJson distSession(CJson::Null);
                                try {
			                        distSession = CJson::parse(request.content());
                                } catch (std::exception &ex) {
                                    static const char *err = "Unable to parse MBSTF Distribution Session as JSON.";
                                    ogs_error("%s", err);
                                    ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message,
                                                                           app_meta, api, "Bad Data Reporting Session", err));
        			                break;
                                }

                                {
                                    std::string txt(distSession.serialise());
	    			                ogs_debug("Request Parsed JSON: %s", txt.c_str());
                                }

                                try {
                                    distributionSession = std::make_shared<DistributionSession>(distSession, true);
                                } catch (std::exception &err) {
                                    ogs_error("Error while populating MBSTF Distribution Session: %s", err.what());
                                    char *error = ogs_msprintf("Bad request [%s]", err.what());
                                    ogs_error("%s", error);
                                    ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 0, message,
                                                                            app_meta, api, "Bad Request", error));
                                    ogs_free(error);
                                    break;
                                }

                                App::self().context()->addDistributionSession(distributionSession);

				//std::shared_ptr<ObjectListController> controller = std::make_shared<ObjectListController>(*distributionSession); 
				distributionSession->m_controller.reset(new ObjectListController(*distributionSession));
                                //distributionSession->setController(controller);


                                    /*
                                std::shared_ptr<DistributionSession> distributionSession = std::make_shared<DistributionSession>(distSession, true);
                                App::self().context()->addDistributionSession(distributionSession);
                                    */

                                CJson createdReqData_json(distributionSession->json(false));
                                std::string body(createdReqData_json.serialise());
                                ogs_debug("Response Parsed JSON: %s", body.c_str());
                                std::ostringstream location;
                                location << request.uri() << "/" << distributionSession->distributionSessionId();
                                std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(location.str(),
                                                                                    body.empty()?nullptr:"application/json",
                                                                                    distributionSession->generated(),
                                                                                    distributionSession->hash().c_str(),
                                                                                    App::self().context()->cacheControl.distMaxAge,
                                                                                    std::nullopt/*nullptr*/, api, app_meta));
                                ogs_assert(response);
                                NfServer::populateResponse(response, body, OGS_SBI_HTTP_STATUS_CREATED);
                                ogs_assert(true == Open5GSSBIServer::sendResponse(stream, *response));
                                break;
                            //}
                        //}

	    	    } else if (method == OGS_SBI_HTTP_METHOD_GET) {
                        if (!ptr_resource1) {
                            std::ostringstream err;
                            err << "Invalid resource [" << message.uri() << "]";
                            ogs_error("%s", err.str().c_str());
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message,
                                                                    app_meta, api, "Bad Request", err.str()));
                            break;
                        }
                        std::string dist_session_id(ptr_resource1);
                        try {
                            int response_code = 200;

                            std::shared_ptr<DistributionSession> distSess = DistributionSession::find(dist_session_id);
                            CJson createdReqData_json(distSess->json(false));
                            std::string body(createdReqData_json.serialise());
                            ogs_debug("Parsed JSON: %s", body.c_str());
                            std::ostringstream location;
                            location << request.uri() << "/" << distSess->distributionSessionId();
                            std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(location.str(),
                                                    body.empty()?nullptr:"application/json",
                                                    distSess->generated(),
                                                    distSess->hash().c_str(),
                                                    App::self().context()->cacheControl.distMaxAge,
                                                    std::nullopt/*nullptr*/, api, app_meta));
                            ogs_assert(response);
                            NfServer::populateResponse(response, body, response_code);
                            ogs_assert(true == Open5GSSBIServer::sendResponse(stream, *response));
                        } catch (const std::out_of_range &e) {
                            std::ostringstream err;
                            err << "MBSTF Distribution Session [" << dist_session_id << "] does not exist.";
                            ogs_error("%s", err.str().c_str());

                            static const std::string param("{sessionId}");
                            std::ostringstream reason;
                            reason << "Invalid MBSTF Distribution Session identifier [" << dist_session_id << "]";
                            std::map<std::string, std::string> invalid_params(
                                                                        NfServer::makeInvalidParams(param, reason.str()));

                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_NOT_FOUND, 2, message,
                                                                    app_meta, api, "MBSTF Distribution Session not found",
                                                                    err.str(), std::nullopt, invalid_params));
                        }
		    } else if (method == OGS_SBI_HTTP_METHOD_DELETE) {
		        if (message.resourceComponent(1) && !message.resourceComponent(2)) {
                            std::string dist_session_id(message.resourceComponent(1));
                            try {
			        App::self().context()->deleteDistributionSession(dist_session_id);
	                        std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(std::nullopt, std::nullopt, std::nullopt, std::nullopt, 0, std::nullopt, api, app_meta));
                                NfServer::populateResponse(response, "", OGS_SBI_HTTP_STATUS_NO_CONTENT);
                                ogs_assert(true == Open5GSSBIServer::sendResponse(stream, *response));

		            } catch (const std::out_of_range &e) {
                                std::ostringstream err;
                                err << "MBSTF Distribution Session [" << dist_session_id << "] does not exist.";
                                ogs_error("%s", err.str().c_str());

                                static const std::string param("{sessionId}");
                                std::ostringstream reason;
                                reason << "Invalid MBSTF Distribution Session identifier [" << dist_session_id << "]";
                                std::map<std::string, std::string> invalid_params(
                                                                        NfServer::makeInvalidParams(param, reason.str()));

                                ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_NOT_FOUND, 2, message,
                                                        app_meta, api, "MBSTF Distribution Session not found", err.str(),
                                                        std::nullopt, invalid_params));
                            }
		        }
		    } else {
                        std::ostringstream err;

                        err << "Invalid method [" << message.method() << "] for " << message.serviceName() << "/"
                                << message.apiVersion() << "/" << message.resourceComponent(0);
                        ogs_error("%s", err.str().c_str());
                        ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message,
                                                                app_meta, api, "Bad request", err.str()));
                    }
	        } else {
                    std::ostringstream err;
                    err << "Unknown object type \"" << message.resourceComponent(0) << "\" in MBSTF Distribution Session";
                    ogs_error("%s", err.str().c_str());
                    ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message, app_meta,
                                                            api, "Bad request", err.str()));
	        }
		//    }
        //        }
	    } else {
                static const char *err = "Missing service name from URL path";
                ogs_error("%s", err);
                ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 0, message, app_meta, std::nullopt,
                                                "Missing service name", err));
            }
            return true;
	    }
	default:
        break;
    }
    return false;
}

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
