#ifndef _MBS_TF_APP_HH_
#define _MBS_TF_APP_HH_
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

#include <memory>
#include <string>

#include "common.hh"
#include "Context.hh"
#include "EventHandler.hh"
#include "Open5GSEvent.hh"
#include "Open5GSFSM.hh"
#include "MBSTFNetworkFunction.hh"
#include "NfServer.hh"
#include "Open5GSNetworkFunction.hh"
#include "Open5GSYamlDocument.hh"

extern "C" {
    extern int app_initialize(const char *const argv[]);
    extern void app_terminate(void);
}

MBSTF_NAMESPACE_START

class App {
public:
    App(const char *const argv[]);
    App() = delete;
    App(App &&other) = delete;
    App(const App &other) = delete;
    App &operator=(App &&other) = delete;
    App &operator=(const App &other) = delete;
    virtual ~App();

    static const App &self(); // Get singleton self

    void initialise();
    void startEventHandler();

    const std::shared_ptr<Open5GSNetworkFunction> &ogsApp() const { return m_app; };
    const std::shared_ptr<Context> &context() const { return m_context; };

    EventHandler *registerEventHandler(EventHandler *event_handler);
    Open5GSYamlDocument configDocument() const;

    const NfServer::AppMetadata &mbstfAppMetadata() const;
    const std::string &serverName();
    const std::string &serverName() const { return m_appMetadata.serverName(); };

protected:
    friend class Open5GSNetworkFunction;
    void dispatchEvent(Open5GSFSM &fsm, Open5GSEvent &event) const;

private:
    std::shared_ptr<Open5GSNetworkFunction> m_app;
    std::shared_ptr<Context>                m_context;
    NfServer::AppMetadata                   m_appMetadata;
    EventHandler                           *m_eventHandler;
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_APP_HH_ */
