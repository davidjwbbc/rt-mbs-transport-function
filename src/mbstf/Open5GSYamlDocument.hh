#ifndef _MBS_TF_OPEN5GS_YAML_DOCUMENT_HH_
#define _MBS_TF_OPEN5GS_YAML_DOCUMENT_HH_
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

#include "ogs-proto.h"

#include <memory>

#include "common.hh"
#include "Open5GSYamlIter.hh"

MBSTF_NAMESPACE_START

class Open5GSYamlDocument {
public:
    Open5GSYamlDocument(void *document) :m_document(document) {};
    Open5GSYamlDocument() = delete;
    Open5GSYamlDocument(Open5GSYamlDocument &&other) :m_document(other.m_document) {};
    Open5GSYamlDocument(const Open5GSYamlDocument &other) :m_document(other.m_document) {};
    Open5GSYamlDocument &operator=(Open5GSYamlDocument &&other) {m_document = other.m_document; return *this;};
    Open5GSYamlDocument &operator=(const Open5GSYamlDocument &other) {m_document = other.m_document; return *this;};
    virtual ~Open5GSYamlDocument() {};

    void *ogsDocument() const { return m_document; };

private:
    void *m_document;
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_OPEN5GS_YAML_DOCUMENT_HH_ */
