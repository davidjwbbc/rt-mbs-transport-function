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

#include "common.hh"
#include "App.hh"
#include "TimerFunc.hh"
#include "Open5GSEvent.hh"
#include "Open5GSTimer.hh"
#include "Open5GSYamlDocument.hh"

#include "Open5GSNetworkFunction.hh"

MBSTF_NAMESPACE_START

Open5GSNetworkFunction::Open5GSNetworkFunction()
{
    if (ogs_env_set("TZ", "UTC") != OGS_OK) {
        throw std::runtime_error("Failed to set clock to UTC");
    }
    if (ogs_env_set("LC_TIME", "C") != OGS_OK) {
        throw std::runtime_error("Failed to set time locale to C");
    }
    
    ogs_sbi_context_init(nfType());
}

Open5GSNetworkFunction::~Open5GSNetworkFunction()
{
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

bool Open5GSNetworkFunction::sbiOpen()
{
    ogs_sbi_nf_instance_t *nf_instance = ogs_sbi_self()->nf_instance;

    ogs_sbi_nf_fsm_init(nf_instance);
    ogs_sbi_nf_instance_build_default(nf_instance);

    nf_instance = ogs_sbi_self()->nrf_instance;
    if (nf_instance) {
        ogs_sbi_nf_fsm_init(nf_instance);
    }

    return ogs_sbi_server_start_all(server_cb) == OGS_OK;
}

bool Open5GSNetworkFunction::startEventHandler()
{
    m_eventThread = ogs_thread_create(Open5GSNetworkFunction::eventThread, NULL);
    return !!m_eventThread;
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
}

void Open5GSNetworkFunction::stateFinal(ogs_fsm_t *s, ogs_event_t *e)
{
    ogs_assert(s);
}


MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
