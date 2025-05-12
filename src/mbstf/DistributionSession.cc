/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Distribution Session class
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * Author(s): Dev Audsin <dev.audsin@bbc.co.uk>
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
#include "Controller.hh"
#include "ControllerFactory.hh"
#include "hash.hh"
#include "MBSTFNetworkFunction.hh"
#include "NfServer.hh"
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
#include "openapi/model/ObjDistributionData.h"
#include "openapi/model/ObjAcquisitionMethod.h"
#include "openapi/model/TunnelAddress.h"

#include "openapi/api/IndividualMBSDistributionSessionApi-info.h"
#include "TimerFunc.hh"

// Header include for this class
#include "DistributionSession.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbstf::CreateReqData;
using reftools::mbstf::DistSession;
using reftools::mbstf::IpAddr;
using reftools::mbstf::ObjDistributionData;
using reftools::mbstf::UpTrafficFlowInfo;
using reftools::mbstf::ObjAcquisitionMethod;
using reftools::mbstf::ObjDistributionOperatingMode;
using reftools::mbstf::TunnelAddress;

MBSTF_NAMESPACE_START

static const NfServer::InterfaceMetadata g_nmbstf_distributionsession_api_metadata(
    NMBSTF_DISTSESSION_API_NAME,
    NMBSTF_DISTSESSION_API_VERSION
);

static std::shared_ptr<ObjDistributionData> get_object_distribution_data(DistributionSession &distributionSession);

DistributionSession::DistributionSession(CJson &json, bool as_request)
    :m_createReqData(std::make_shared<CreateReqData>(json, as_request))
    ,m_controller()
    //,m_eventSubscriptions()
{

    std::shared_ptr<DistSession> distSession = m_createReqData->getDistSession();

    m_generated = std::chrono::system_clock::now();
    m_lastUsed = m_generated;

    std::string json_str(json.serialise());
    m_hash = calculate_hash(std::vector<std::string::value_type>(json_str.begin(), json_str.end()));
    m_distributionSessionId = distSession->getDistSessionId();

    //App::self().context()->addDistributionSession(m_distributionSessionId, std::shared_ptr<DistributionSession> DistributionSession)
}

DistributionSession::~DistributionSession()
{
    // TODO: if session is in ACTIVE state then send SESSION_DEACTIVED event to any event subscribers that are listening for it.
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
                //request.setOwner(true);
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
                        ogs_debug("In MBSTF Distribution session");
                        std::shared_ptr<DistributionSession> distributionSession;
                        ogs_debug("Request body: %s", request.content());
                        //ogs_debug("Request " OGS_SBI_CONTENT_TYPE ": %s", request.headerValue(OGS_SBI_CONTENT_TYPE, std::string()).c_str());
                        if (request.headerValue(OGS_SBI_CONTENT_TYPE, std::string()) != "application/json") {
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE,
                                                                   3, message, app_meta, api, "Unsupported Media Type",
                                                                   "Expected content type: application/json"));
                            return true;
                        }

                        CJson distSession(CJson::Null);
                        try {
                            distSession = CJson::parse(request.content());
                        } catch (std::exception &ex) {
                            static const char *err = "Unable to parse MBSTF Distribution Session as JSON.";
                            ogs_error("%s", err);
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message,
                                                                    app_meta, api, "Bad Data Reporting Session", err));
                            return true;
                        }

                        {
                            std::string txt(distSession.serialise());
                            ogs_debug("Request Parsed JSON: %s", txt.c_str());
                        }

                        try {
                            distributionSession.reset(new DistributionSession(distSession, true));
                        } catch (std::exception &err) {
                            ogs_error("Error while populating MBSTF Distribution Session: %s", err.what());
                            char *error = ogs_msprintf("Bad request [%s]", err.what());
                            ogs_error("%s", error);
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 0, message,
                                                                            app_meta, api, "Bad Request", error));
                            ogs_free(error);
                            return true;
                        }

			try {

                            distributionSession->m_controller.reset(ControllerFactory::makeController(*distributionSession));
		  	    if(!distributionSession->m_controller) {
				const std::string &mode = distributionSession->getObjectDistributionOperatingMode();
			        char *error = ogs_msprintf("No handler found for objDistributionOperatingMode [%s]", mode.c_str());
				ogs_error("%s", error);
				ogs_assert(true == NfServer::sendError(stream, 501, 0, message,
                                                                            app_meta, api, "Not Implemented", error));
                                ogs_free(error);
                                return true;
			    }
                        } catch (std::runtime_error &err) {
                            ogs_error("Error while populating MBSTF Distribution Session: %s", err.what());
                            char *error = ogs_msprintf("Invalid ObjDistributionData parameters [%s]", err.what());
                            ogs_error("%s", error);
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 0, message,
                                                                            app_meta, api, "Invalid ObjDistributionData parameters", error));
                            ogs_free(error);
                            return true;
                        }

                        App::self().context()->addDistributionSession(distributionSession);

                        // TODO: Subscribe to Events from the Controller - to be forwarded to DistributionSessionSubscriptions

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
                        return true;
                    } else if (method == OGS_SBI_HTTP_METHOD_GET) {
                        if (!ptr_resource1) {
                            std::ostringstream err;
                            err << "Invalid resource [" << message.uri() << "]";
                            ogs_error("%s", err.str().c_str());
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message,
                                                                    app_meta, api, "Bad Request", err.str()));
                            return true;
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
                        return true;
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
                            return true;
                        }
                    } else {
                        std::ostringstream err;

                        err << "Invalid method [" << message.method() << "] for " << message.serviceName() << "/"
                                << message.apiVersion() << "/" << message.resourceComponent(0);
                        ogs_error("%s", err.str().c_str());
                        ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message,
                                                                app_meta, api, "Bad request", err.str()));
                        return true;
                    }
                } else {
                    std::ostringstream err;
                    err << "Unknown object type \"" << message.resourceComponent(0) << "\" in MBSTF Distribution Session";
                    ogs_error("%s", err.str().c_str());
                    ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message, app_meta,
                                                            api, "Bad request", err.str()));
                    return true;
                }
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

const ObjDistributionData::ObjAcquisitionIdsPullType &DistributionSession::getObjectAcquisitionPullUrls()
{
    std::shared_ptr<CreateReqData> createReqData = distributionSessionReqData();
    std::shared_ptr<DistSession> distSession = createReqData->getDistSession();
    const std::optional<std::shared_ptr<ObjDistributionData> > &object_distribution_data = distSession->getObjDistributionData();
    if (object_distribution_data.has_value()) {
        std::shared_ptr<ObjDistributionData> objectDistributionDataPtr = object_distribution_data.value();
        return objectDistributionDataPtr->getObjAcquisitionIdsPull();
    } else {
        ogs_error("ObjectDistributionData is not available");
        static const ObjDistributionData::ObjAcquisitionIdsPullType empty_result;
        return empty_result;
    }
}

const std::optional<std::string> &DistributionSession::getDestIpAddr()
{
    std::shared_ptr<CreateReqData> createReqData = distributionSessionReqData();
    std::shared_ptr<DistSession> distSession = createReqData->getDistSession();
    const std::optional<std::shared_ptr< UpTrafficFlowInfo > > &upTrafficFlowInfo = distSession->getUpTrafficFlowInfo();
    if (upTrafficFlowInfo.has_value()) {
        std::shared_ptr<UpTrafficFlowInfo> upTrafficFlow = upTrafficFlowInfo.value();
        const std::shared_ptr<IpAddr> ipAddr = upTrafficFlow->getDestIpAddr();
        if (ipAddr) {
            return ipAddr->getIpv4Addr();
        }
    }

    static const std::optional<std::string> empty = std::nullopt;
    return empty;
}

const std::optional<std::string> &DistributionSession::getTunnelAddr()
{
    std::shared_ptr<CreateReqData> createReqData = distributionSessionReqData();
    std::shared_ptr<DistSession> distSession = createReqData->getDistSession();
    std::optional<std::shared_ptr<TunnelAddress> > mbUpfTunAddr = distSession->getMbUpfTunAddr();
    if (mbUpfTunAddr.has_value()) {
        return mbUpfTunAddr.value()->getIpv4Addr();
    }

    static const std::optional<std::string> empty = std::nullopt;
    return empty;
}

in_port_t DistributionSession::getPortNumber()
{
    in_port_t portNumber = 0;
    std::shared_ptr<CreateReqData> createReqData = distributionSessionReqData();
    std::shared_ptr<DistSession> distSession = createReqData->getDistSession();
    std::optional<std::shared_ptr<UpTrafficFlowInfo> > upTrafficFlowInfo = distSession->getUpTrafficFlowInfo();
    if (upTrafficFlowInfo.has_value()) {
        std::shared_ptr<UpTrafficFlowInfo> upTrafficFlow = upTrafficFlowInfo.value();
        portNumber = static_cast<in_port_t>(upTrafficFlow->getPortNumber());
    }
    return portNumber;
}

in_port_t DistributionSession::getTunnelPortNumber()
{
    in_port_t portNumber = 0;
    std::shared_ptr<CreateReqData> createReqData = distributionSessionReqData();
    std::shared_ptr<DistSession> distSession = createReqData->getDistSession();
    std::optional<std::shared_ptr<TunnelAddress> > mbUpfTunAddr = distSession->getMbUpfTunAddr();
    if (mbUpfTunAddr.has_value()) {
        portNumber = static_cast<in_port_t>(mbUpfTunAddr.value()->getPortNumber());
    }
    return portNumber;
}

uint32_t DistributionSession::getRateLimit()
{
    std::shared_ptr<CreateReqData> createReqData = distributionSessionReqData();
    std::shared_ptr<DistSession> distSession = createReqData->getDistSession();
    const std::optional<std::string> &mbr = distSession->getMbr();

    if (mbr) {
        try {
            return static_cast<uint32_t>(std::stoul(mbr.value()));
        } catch (const std::invalid_argument &e) {
            throw std::runtime_error("Invalid MBR value");
        } catch (const std::out_of_range &e) {
            throw std::runtime_error("MBR value out of range");
        }
    }

    return 0;
}

const std::optional<std::string> &DistributionSession::getObjectIngestBaseUrl()
{
    std::shared_ptr<CreateReqData> createReqData = distributionSessionReqData();
    std::shared_ptr<DistSession> distSession = createReqData->getDistSession();
    const std::optional<std::shared_ptr<ObjDistributionData> > &object_distribution_data = distSession->getObjDistributionData();
    if (object_distribution_data.has_value()) {
        std::shared_ptr<ObjDistributionData> objectDistributionDataPtr = object_distribution_data.value();
        return objectDistributionDataPtr->getObjIngestBaseUrl();
    } else {
        ogs_error("ObjectDistributionData is not available");
        static const std::optional<std::string> nullValue = std::nullopt;
        return nullValue;
    }
}

const std::string &DistributionSession::getObjectAcquisitionMethod()
{
    std::shared_ptr<CreateReqData> createReqData = distributionSessionReqData();
    std::shared_ptr<DistSession> distSession = createReqData->getDistSession();
    const std::optional<std::shared_ptr<ObjDistributionData> > &object_distribution_data = distSession->getObjDistributionData();
    if (object_distribution_data.has_value()) {
        std::shared_ptr<ObjDistributionData> objectDistributionDataPtr = object_distribution_data.value();
        std::shared_ptr< ObjAcquisitionMethod > objAcquisitionMethod = objectDistributionDataPtr->getObjAcquisitionMethod();
        return objAcquisitionMethod->getString();
    } else {
        ogs_error("ObjectDistributionData is not available");
        static std::string emptyObjAcquisitionMethod = std::string();
        return emptyObjAcquisitionMethod;
    }
}

void DistributionSession::setObjectIngestBaseUrl(std::string ingestBaseUrl)
{
    const std::shared_ptr<CreateReqData> createReqData = distributionSessionReqData();
    std::shared_ptr<DistSession> distSession = createReqData->getDistSession();
    const std::optional<std::shared_ptr<ObjDistributionData> > &object_distribution_data = distSession->getObjDistributionData();
    if (object_distribution_data.has_value()) {
        std::shared_ptr<ObjDistributionData> objectDistributionDataPtr = object_distribution_data.value();
        const std::optional<std::string> &baseUrl = ingestBaseUrl.empty() ? std::nullopt : std::optional<std::string>(ingestBaseUrl);
        objectDistributionDataPtr->setObjIngestBaseUrl(baseUrl);
    } else {
        ogs_error("ObjectDistributionData is not available");
    }
}

const std::optional<std::string> &DistributionSession::getObjectAcquisitionPushId()
{
    std::shared_ptr<CreateReqData> createReqData = distributionSessionReqData();
    std::shared_ptr<DistSession> distSession = createReqData->getDistSession();
    const std::optional<std::shared_ptr<ObjDistributionData> > &object_distribution_data = distSession->getObjDistributionData();
    if (object_distribution_data.has_value()) {
        std::shared_ptr<ObjDistributionData> objectDistributionDataPtr = object_distribution_data.value();
        return objectDistributionDataPtr->getObjAcquisitionIdPush();
    } else {
        ogs_error("ObjectDistributionData is not available");
        static const std::optional<std::string> nullValue = std::nullopt;
        return nullValue;
    }
}

bool DistributionSession::setObjectAcquisitionIdPush(std::optional<std::string> &id) {
    std::shared_ptr<ObjDistributionData> object_distribution_data = get_object_distribution_data(*this);
    if (object_distribution_data) {
        return object_distribution_data->setObjAcquisitionIdPush(id);
    } else {
        ogs_error("ObjectDistributionData is not available");
        return false;
    }
    return false;

}

static std::shared_ptr<ObjDistributionData> get_object_distribution_data(DistributionSession &distributionSession)
{
    std::shared_ptr<CreateReqData> createReqData = distributionSession.distributionSessionReqData();
    std::shared_ptr<DistSession> distSession = createReqData->getDistSession();
    const std::optional<std::shared_ptr<ObjDistributionData> > &object_distribution_data = distSession->getObjDistributionData();
    if (object_distribution_data.has_value()) {
        std::shared_ptr<ObjDistributionData> objectDistributionDataPtr = object_distribution_data.value();
        return objectDistributionDataPtr;
    } else {
        return nullptr;
    }

}

const std::string &DistributionSession::getObjectDistributionOperatingMode() {
    std::shared_ptr<ObjDistributionData> object_distribution_data = get_object_distribution_data(*this);
    if (object_distribution_data) {
        std::shared_ptr< ObjDistributionOperatingMode > operating_mode = object_distribution_data->getObjDistributionOperatingMode();
        return operating_mode->getString();
    } else {
        ogs_error("ObjectDistributionData is not available");
        static std::string emptyObjAcquisitionMethod = std::string();
        return emptyObjAcquisitionMethod;
    }
}

const std::optional<std::string> &DistributionSession::objectDistributionBaseUrl() const
{
    std::shared_ptr<CreateReqData> createReqData = distributionSessionReqData();
    std::shared_ptr<DistSession> distSession = createReqData->getDistSession();
    const std::optional<std::shared_ptr<ObjDistributionData> > &object_distribution_data = distSession->getObjDistributionData();
    if (object_distribution_data.has_value()) {
        std::shared_ptr<ObjDistributionData> objectDistributionDataPtr = object_distribution_data.value();
        return objectDistributionDataPtr->getObjDistributionBaseUrl();
    } else {
        ogs_error("ObjectDistributionData is not available");
        static const std::optional<std::string> nullValue = std::nullopt;
        return nullValue;
    }
}

std::string DistributionSession::trimSlashes(const std::string &path)
{
    size_t start = path.starts_with('/') ? 1 : 0;
    size_t end = path.ends_with('/') ? path.size() - 1 : path.size();

    return path.substr(start, end - start);
}


MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
