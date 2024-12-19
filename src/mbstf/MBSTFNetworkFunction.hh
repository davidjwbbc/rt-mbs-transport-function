#ifndef _MBS_TF_MBSTF_NETWORK_FUNCTION_HH_
#define _MBS_TF_MBSTF_NETWORK_FUNCTION_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSTF Network Function class
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

#include "common.hh"
#include "Open5GSNetworkFunction.hh"

MBSTF_NAMESPACE_START

class MBSTFNetworkFunction : public Open5GSNetworkFunction {
public:
    MBSTFNetworkFunction() : Open5GSNetworkFunction() {};
    MBSTFNetworkFunction(MBSTFNetworkFunction &&other) = delete;
    MBSTFNetworkFunction(const MBSTFNetworkFunction &other) = delete;
    MBSTFNetworkFunction &operator=(MBSTFNetworkFunction &&other) = delete;
    MBSTFNetworkFunction &operator=(const MBSTFNetworkFunction &other) = delete;
    virtual ~MBSTFNetworkFunction() {};

    virtual OpenAPI_nf_type_e nfType() const { return OpenAPI_nf_type_MBSTF; };
   
private:
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_MBSTF_NETWORK_FUNCTION_HH_ */
