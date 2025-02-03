/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: MBSTF Controller
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
#include "Controller.hh"
#include "ObjectStore.hh"

class Controller;
class ObjectStore;

MBSTF_NAMESPACE_START

Controller::Controller(MBSTFDistributionSession &distributionSession, ObjectStore &objectStore)
    :m_distributionSession(distributionSession)
    ,m_objectStore(objectStore)	
{
}

/*ObjectStore& objectStore(MBSTFDistributionSession &distributionSession)
{
    return m_distributionSession.m_objectStore;
}
*/

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
