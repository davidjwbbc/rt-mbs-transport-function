#ifndef _MBS_TF_CONTEXT_HH_
#define _MBS_TF_CONTEXT_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSTF Context
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <map>
#include <memory>

#include "common.hh"

MBSTF_NAMESPACE_START

class MBSTFDistributionSession;
class Open5GSSBIServer;
class Open5GSSockAddr;
class Open5GSYamlIter;

class Context {
public:
    Context();
    Context(Context &&other) = delete;
    Context(const Context &other) = delete;
    Context &operator=(Context &&other) = delete;
    Context &operator=(const Context &other) = delete;
    virtual ~Context();

    bool parseConfig();

    std::shared_ptr<Open5GSSockAddr> MBSTFDistributionSessionServerAddress();

/*
    void addDistributionSession(const std::string &session_id, const std::shared_ptr<MBSTFDistributionSession> &session) {
        distributionSessions.insert(std::make_pair(session_id, std::shared_ptr<MBSTFDistributionSession>(session)));
    }
*/
    void addDistributionSession(const std::shared_ptr<MBSTFDistributionSession> &MBSTFDistributionSession);
    void deleteDistributionSession(const std::string &distributionSessionid);

    enum ServerType {
	SERVER_DISTRIBUTION_SESSION,
        SERVER_OBJECT_PUSH,
	SERVER_RTP,
        SERVER_MAX_NUM
    };

    std::map<std::string, std::shared_ptr<MBSTFDistributionSession> > distributionSessions;
    std::shared_ptr<Open5GSSBIServer> servers[SERVER_MAX_NUM];
    struct {
        unsigned int distMaxAge;
        unsigned int defaultObjectMaxAge; // Use if not given by push/pull resource Cache-Control.
    } cacheControl;
    
private:
    void parseCacheControl(Open5GSYamlIter &iter);
    void parseConfiguration(std::string &pc_key, Open5GSYamlIter &iter);

};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_CONTEXT_HH_ */
