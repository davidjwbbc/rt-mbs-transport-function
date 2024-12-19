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

#include "App.hh"

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

extern "C" void app_terminate(void)
{
    if (event_handler) {
        self->registerEventHandler(nullptr);
        delete event_handler;
        event_handler = nullptr;
    }

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
    m_context.reset(new Context());

    m_app->configureLoggingDomain();
}

void App::initialise()
{
    if (!m_context->parseConfig()) {
        throw std::runtime_error("Unable to load MBSTF configuration!");
    }
    if (!m_app->sbiParseConfig("mbstf")) {
        throw std::runtime_error("Open5GS parse SBI configuration failed!");
    }
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

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
