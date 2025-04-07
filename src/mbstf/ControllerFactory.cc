/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Session Controller Factory
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <algorithm>
#include <exception>
#include <list>
#include <memory>

#include "common.hh"
#include "DistributionSession.hh"

#include "ControllerFactory.hh"

MBSTF_NAMESPACE_START

struct ControllerConstructorCompare {
    bool operator()(const std::unique_ptr<ControllerFactoryConstructor> &a, const std::unique_ptr<ControllerFactoryConstructor> &b)
    {
        return (a->priority() > b->priority());
    };
} g_factoryCompare;

static std::list<std::unique_ptr<ControllerFactoryConstructor> > g_constructors;

bool ControllerFactory::registerController(ControllerFactoryConstructor *controller_constructor)
{
        std::unique_ptr<ControllerFactoryConstructor> cc(controller_constructor);
        g_constructors.insert(std::lower_bound(g_constructors.begin(), g_constructors.end(), cc, g_factoryCompare), std::move(cc));
        return true;
}

Controller *ControllerFactory::makeController(DistributionSession &distributionSession)
{
    for (const auto &cc : g_constructors) {
        try {
            return cc->makeController(distributionSession);
	} catch (const std::runtime_error& e) {
            // Handle runtime_error specifically
            throw std::runtime_error(e.what());
        } catch (std::exception &ex) {
            // That controller couldn't handle that DistributionSession, let's try the next
        }
    }
    return nullptr;
}

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
