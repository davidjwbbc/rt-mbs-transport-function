#ifndef _MBS_TF_MBSTF_OBJECT_INGESTER_HH_
#define _MBS_TF_MBSTF_OBJECT_INGESTER_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSTF Object Ingester base class
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */


#include "common.hh"

MBSTF_NAMESPACE_START

class ObjectIngester {
public:
    virtual void abort() = 0;
    virtual ~ObjectIngester() {};
	
private:
};

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_MBSTF_OBJECT_INGESTER_HH_ */
