#ifndef _MBS_TF_OPEN5GS_YAML_ITER_HH_
#define _MBS_TF_OPEN5GS_YAML_ITER_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS SBI Server interface
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

#include "common.hh"

MBSTF_NAMESPACE_START

class Open5GSYamlDocument;

class Open5GSYamlIter {
public:
    Open5GSYamlIter(const Open5GSYamlDocument &document);
    Open5GSYamlIter(Open5GSYamlIter &parent);
    Open5GSYamlIter() = delete;
    Open5GSYamlIter(Open5GSYamlIter &&other) = delete;
    Open5GSYamlIter(const Open5GSYamlIter &other) = delete;
    Open5GSYamlIter &operator=(Open5GSYamlIter &&other) = delete;
    Open5GSYamlIter &operator=(const Open5GSYamlIter &other) = delete;
    virtual ~Open5GSYamlIter() {};

    int next();

    int type() const;
    const char *key() const;
    const char *value() const;
    bool valueBool() const;

private:
    ogs_yaml_iter_t m_iterator;
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_OPEN5GS_YAML_ITER_HH_ */
