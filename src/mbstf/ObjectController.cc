/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSTF ObjectController
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
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
#include <utility>
#include <chrono>
#include <thread>
#include <mutex>

#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "ObjectController.hh"
#include "ObjectStore.hh"

class ObjectController;
class ObjectStore;

MBSTF_NAMESPACE_START

ObjectController::ObjectController(DistributionSession &distributionSession, ObjectStore &objectStore)
    :m_distributionSession(distributionSession)
    ,m_objectStore(objectStore)	
{
}

/*ObjectStore& objectStore(DistributionSession &distributionSession)
{
    return m_distributionSession.m_objectStore;
}
*/

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
