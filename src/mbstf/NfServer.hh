#ifndef NF_SERVER_H
#define NF_SERVER_H
/*
 * License: 5G-MAG Public License (v1.0)
 * Author: Dev Audsin
 * Copyright: (C) 2022-2024 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <chrono>
#include <map>
#include <memory>
#include <optional>
#include <string>

#include "common.hh"
#include "openapi/model/CJson.hh"

using fiveg_mag_reftools::CJson;

MBSTF_NAMESPACE_START

class Open5GSSBIMessage;
class Open5GSSBIResponse;
class Open5GSSBIStream;

class NfServer {
public:
    using time_type = std::chrono::system_clock::time_point;

    class InterfaceMetadata {
    public:
        InterfaceMetadata(const std::string &api_title, const std::string &api_version);
        InterfaceMetadata(const char *api_title, const char *api_version);
        InterfaceMetadata(const InterfaceMetadata &);

        InterfaceMetadata() = delete;
        InterfaceMetadata(InterfaceMetadata &&) = delete;

        ~InterfaceMetadata();

        InterfaceMetadata &operator=(const InterfaceMetadata &);
        InterfaceMetadata &operator=(InterfaceMetadata &&) = delete;

        bool operator==(const InterfaceMetadata &other) const {
            return m_apiTitle == other.apiTitle() && m_apiVersion == other.apiVersion();
        };

        const std::string &apiTitle() const { return m_apiTitle; };
        const std::string &apiVersion() const { return m_apiVersion; };

    private:
        std::string m_apiTitle;
        std::string m_apiVersion;
    };

    class AppMetadata {
    public:
        AppMetadata(const std::string &app_name, const std::string &app_version, const std::string &server_name);
        AppMetadata(const char *app_name, const char *app_version, const char *server_name);

        AppMetadata() = delete;
        AppMetadata(AppMetadata &&) = delete;
        AppMetadata(const AppMetadata &) = delete;

        ~AppMetadata();

        AppMetadata &operator=(AppMetadata &&) = delete;
        AppMetadata &operator=(const AppMetadata &) = delete;

        const std::string &appName() const { return m_appName; };
        const std::string &appVersion() const { return m_appVersion; };
        const std::string &serverName() const { return m_serverName; };

        AppMetadata &appName(const std::string &app_name) { m_appName = app_name; return *this; };
        AppMetadata &appName(std::string &&app_name) { m_appName = std::move(app_name); return *this; };
        AppMetadata &appVersion(const std::string &app_version) { m_appVersion = app_version; return *this; };
        AppMetadata &appVersion(std::string &&app_version) { m_appVersion = std::move(app_version); return *this; };
        AppMetadata &serverName(const std::string &server_name) { m_serverName = server_name; return *this; };
        AppMetadata &serverName(std::string &&server_name) { m_serverName = std::move(server_name); return *this; };

    private:
        std::string m_appName;
        std::string m_appVersion;
        std::string m_serverName;
    };

    static const InterfaceMetadata nullInterfaceMetadata;

    static bool sendError(Open5GSSBIStream &stream, int status, size_t number_of_components,
                          const Open5GSSBIMessage &message, const AppMetadata &app,
                          const std::optional<InterfaceMetadata> &interface = std::nullopt,
                          const std::optional<std::string> &title = std::nullopt,
                          const std::optional<std::string> &detail = std::nullopt,
                          const std::optional<CJson> &problem_detail_json = std::nullopt,
                          const std::optional<std::map<std::string,std::string> > &invalid_params = std::nullopt,
                          const std::optional<std::string> &problem_type = std::nullopt);

    static std::shared_ptr<Open5GSSBIResponse> newResponse(const std::optional<std::string> &location,
                                                           const std::optional<std::string> &content_type,
                                                           const std::optional<time_type> &last_modified,
                                                           const std::optional<std::string> &etag, int cache_control_max_age,
                                                           const std::optional<std::string> &allow_methods,
                                                           const std::optional<InterfaceMetadata> &interface,
                                                           const AppMetadata &app);

    static std::shared_ptr<Open5GSSBIResponse> populateResponse(std::shared_ptr<Open5GSSBIResponse> &response, const std::string &content, int status);

    static std::map<std::string, std::string> makeInvalidParams(const std::string &param, const std::string &reason);
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* NF_SERVER_H */
