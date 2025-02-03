#ifndef _MBS_TF_MBSTF_PULL_OBJECT_INGESTER_HH_
#define _MBS_TF_MBSTF_PULL_OBJECT_INGESTER_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSTF Pull Object Ingester class
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include "ogs-proto.h"
#include "ogs-sbi.h"

#include <string>

#include "common.hh"
#include "ObjectStore.hh"
#include "ObjectIngester.hh"

MBSTF_NAMESPACE_START

class ObjectStore;

class PullObjectIngester : public ObjectIngester{
public:

    PullObjectIngester() = delete;
    PullObjectIngester(ObjectStore& objectStore);
    ~PullObjectIngester();	
    bool addObjectPull(const std::string &object_id, const std::string &url);
    void abort() override;
    static int client_notify_cb(int status, ogs_sbi_response_t *response, void *data);
	
private:
    static std::vector<unsigned char> stringToVector(const std::string &str);
    //Open5GSSBIRequest createPullObjectIngestorRequest(const std::string &url);

};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_MBSTF_PULL_OBJECT_INGESTER_HH_ */
