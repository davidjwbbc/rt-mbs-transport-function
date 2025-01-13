#ifndef _MBS_TF_OPEN5GS_NETWORK_FUNCTION_HH_
#define _MBS_TF_OPEN5GS_NETWORK_FUNCTION_HH_
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
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include <memory>
#include <string>
#include <cstring>

#include "common.hh"
#include "Open5GSYamlDocument.hh"

MBSTF_NAMESPACE_START

class Open5GSEvent;
class Open5GSTimer;
class TimerFunc;

class Open5GSNetworkFunction {
public:
    Open5GSNetworkFunction();
    Open5GSNetworkFunction(Open5GSNetworkFunction &&other) = delete;
    Open5GSNetworkFunction(const Open5GSNetworkFunction &other) = delete;
    Open5GSNetworkFunction &operator=(Open5GSNetworkFunction &&other) = delete;
    Open5GSNetworkFunction &operator=(const Open5GSNetworkFunction &other) = delete;
    virtual ~Open5GSNetworkFunction();

    bool noIPv4() const { return ogs_app()->parameter.no_ipv4; };
    bool noIPv6() const { return ogs_app()->parameter.no_ipv6; };

    std::shared_ptr<Open5GSTimer> addTimer(TimerFunc &timer_func);
    void removeTimer(const std::shared_ptr<Open5GSTimer> &timer);

    Open5GSYamlDocument configFileDocument() const;

    void pushEvent(const std::shared_ptr<Open5GSEvent> &event);

    bool configureLoggingDomain();
    bool sbiParseConfig(const char *app_section, const char *nrf_section = "nrf", const char *scp_section = "scp");
    bool sbiOpen();
    void sbiClose();

    bool startEventHandler();
    void stopEventHandler();

    bool setNFServiceInfo(const char *serviceName, const char *supportedFeatures, const char *apiVersion, ogs_sockaddr_t *addr);
    int setNFService();
    int setServerName();

    void initialise() {ogs_sbi_context_init(nfType());};

    virtual OpenAPI_nf_type_e nfType() const { return OpenAPI_nf_type_AF; };
    char  *serverName() { return m_serverName; };

   
private:
    static void eventThread(void *data);
    static void addAddressesToNFService(ogs_sbi_nf_service_t *nf_service, ogs_sockaddr_t *addrs);

    static void stateInitial(ogs_fsm_t *s, ogs_event_t *e);
    static void stateFunctional(ogs_fsm_t *s, ogs_event_t *e);
    static void stateFinal(ogs_fsm_t *s, ogs_event_t *e);

    ogs_thread_t *m_eventThread;
    char *m_serviceName;
    char *m_supportedFeatures;
    char *m_apiVersion;
    char *m_serverName;
    ogs_sockaddr_t *m_addr;

};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_OPEN5GS_NETWORK_FUNCTION_HH_ */
