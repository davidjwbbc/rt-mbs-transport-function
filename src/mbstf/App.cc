/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Main app entry point
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include "ogs-core.h"

#include <stdexcept>

#include "common.hh"
#include "Context.hh"
#include "MBSTFEventHandler.hh"
#include "MBSTFNetworkFunction.hh"
#include "Open5GSFSM.hh"
#include "Open5GSEvent.hh"
#include "openapi/api/IndividualMBSDistributionSessionApi-info.h"
#include "App.hh"
#include "mbstf-version.h"

MBSTF_NAMESPACE_USING;

static App *self = nullptr;
static EventHandler *event_handler = nullptr;

extern "C" int app_initialize(const char *const argv[])
{
    if (!self) {
        try {
            self = new App(argv);
            self->initialise();
            event_handler = new MBSTFEventHandler();
            self->registerEventHandler(event_handler);
            self->startEventHandler();
        } catch (std::exception &err) {
            ogs_error("Fatal error: %s", err.what());
            return OGS_ERROR;
        }
        return OGS_OK;
    }

    ogs_error("Attempt to initalise the application whilst already initialised");

    return OGS_ERROR;
}

static ogs_timer_t *t_termination_holding = NULL;

extern "C" void event_termination(void)
{
    ogs_sbi_nf_instance_t *nf_instance = NULL;

    ogs_list_for_each(&ogs_sbi_self()->nf_instance_list, nf_instance)
        ogs_sbi_nf_fsm_fini(nf_instance);

    t_termination_holding = ogs_timer_add(ogs_app()->timer_mgr, NULL, NULL);
    ogs_assert(t_termination_holding);
#define TERMINATION_HOLDING_TIME ogs_time_from_msec(300)
    ogs_timer_start(t_termination_holding, TERMINATION_HOLDING_TIME);

    ogs_queue_term(ogs_app()->queue);
    ogs_pollset_notify(ogs_app()->pollset);
}

extern "C" void app_terminate(void)
{
    if (event_handler) {
        self->registerEventHandler(nullptr);
        delete event_handler;
        event_handler = nullptr;
    }

    event_termination();

    self->stopEventHandler();
    ogs_timer_delete(t_termination_holding);
    self->appSbiClose();
    ogs_sbi_context_final();

    if (self) {
        delete self;
        self = nullptr;
        return;
    }

    ogs_error("Attempt to terminate when application whilst in a non-initialised state");

    return;
}

MBSTF_NAMESPACE_START

App::App(const char *const argv[])
    :m_app()     // initialise logging first
    ,m_context() // initialise logging first
{
    initialise_logging();
    m_app.reset(new MBSTFNetworkFunction());
    m_app->initialise();
    m_context.reset(new Context());

    m_app->configureLoggingDomain();
}

void App::initialise()
{
	
    const char *serviceName = NMBSTF_DISTSESSION_API_NAME;
    const char *supportedFeatures = "0";
    const char *apiVersion = NMBSTF_DISTSESSION_API_VERSION;

    if (!m_context->parseConfig()) {
        throw std::runtime_error("Unable to load MBSTF configuration!");
    }
    if (!m_app->sbiParseConfig("mbstf")) {
        throw std::runtime_error("Open5GS parse SBI configuration failed!");
    }

    ogs_sockaddr_t *addr = getMBSTFDistributionSessionServerAddress();
    ogs_assert(addr);

    m_app->setNFServiceInfo(serviceName, supportedFeatures, apiVersion, addr);

    MBSTFAppMetadata();

    if (!m_app->sbiOpen()) {
        throw std::runtime_error("Open SBI servers failed!");
    }
}

void App::startEventHandler()
{
    if (!m_app->startEventHandler()) {
        throw std::runtime_error("Unable to start event processing thread!");
    }
}

void App::stopEventHandler()
{
    m_app->stopEventHandler();
}

void App::appSbiClose()
{
    m_app->sbiClose();
}


App::~App()
{
    m_context.reset();
    m_app.reset();
}

const App &App::self()
{
    if (!::self) {
        throw std::runtime_error("Application is not initialised!");
    }
    return *::self;
}

EventHandler *App::registerEventHandler(EventHandler *event_handler)
{
    EventHandler *old = m_eventHandler;
    m_eventHandler = event_handler;
    return old;
}

Open5GSYamlDocument App::configDocument() const
{
    return m_app->configFileDocument();
}

void App::dispatchEvent(Open5GSFSM &fsm, Open5GSEvent &event) const
{
    if (m_eventHandler) {
        m_eventHandler->dispatch(fsm, event);
    }
}

ogs_sockaddr_t *App::getMBSTFDistributionSessionServerAddress()
{
    return m_context->MBSTFDistributionSessionServerAddress();
}

char* App::nf_name = NULL;
// Definition and initialization of the static member
nf_server_app_metadata_t App::app_metadata = { MBSTF_NAME, MBSTF_VERSION, NULL };

const nf_server_app_metadata_t* App::MBSTFAppMetadata() const
{
    if (!nf_name) {
        if (!m_app->serverName()) m_app->setServerName();
	nf_name = ogs_msprintf("MBSTF-%s", (m_app->serverName()));
        ogs_assert(nf_name);
        app_metadata.server_name = nf_name;
    }
    ogs_info("NF NAME: %s", nf_name);

    return &app_metadata;
}


MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
