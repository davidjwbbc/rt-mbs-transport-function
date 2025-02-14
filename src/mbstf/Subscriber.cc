/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Subscriber class
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <deque>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "common.hh"
#include "Event.hh"
#include "SubscriptionService.hh"

#include "Subscriber.hh"

MBSTF_NAMESPACE_START

Subscriber::Subscriber()
    :m_subscriptions()
{
}

Subscriber::Subscriber(const Subscriber &other)
    :m_subscriptions()
{
    /* copy subscriptions from other */
    for (auto svc : other.m_subscriptions) {
        std::list<const char*> event_names = svc->subscribedEvents(const_cast<Subscriber&>(other));
        if (!event_names.empty()) {
            if (event_names.size() == 1 && event_names.front() == nullptr) {
                svc->subscribe(*this);
            } else {
                svc->subscribe(event_names, *this);
            }
        }
    }
}

Subscriber::Subscriber(Subscriber &&other)
    :m_subscriptions()
{
    /* copy subscriptions from other and unsubscribe other */
    for (auto svc : other.m_subscriptions) {
        std::list<const char*> event_names = svc->subscribedEvents(other);
        if (!event_names.empty()) {
            if (event_names.size() == 1 && event_names.front() == nullptr) {
                svc->subscribe(*this);
            } else {
                svc->subscribe(event_names, *this);
            }
            svc->unsubscribe(other);
        }
    }
}

Subscriber::~Subscriber()
{
    /* Unsubscribe all */
    for (auto svc : m_subscriptions) {
        svc->unsubscribe(*this);
    }
}

Subscriber &Subscriber::operator=(const Subscriber &other)
{
    /* Unsubscribe all current */
    for (auto svc : m_subscriptions) {
        svc->unsubscribe(*this);
    }

    /* copy subscriptions from other */
    for (auto svc : other.m_subscriptions) {
        std::list<const char*> event_names = svc->subscribedEvents(const_cast<Subscriber&>(other));
        if (!event_names.empty()) {
            if (event_names.size() == 1 && event_names.front() == nullptr) {
                svc->subscribe(*this);
            } else {
                svc->subscribe(event_names, *this);
            }
        }
    }

    return *this;
}

Subscriber &Subscriber::operator=(Subscriber &&other)
{
    /* Unsubscribe all current */
    for (auto svc : m_subscriptions) {
        svc->unsubscribe(*this);
    }
    /* copy subscriptions from other and unsubscribe other */
    for (auto svc : other.m_subscriptions) {
        std::list<const char*> event_names = svc->subscribedEvents(other);
        if (!event_names.empty()) {
            if (event_names.size() == 1 && event_names.front() == nullptr) {
                svc->subscribe(*this);
            } else {
                svc->subscribe(event_names, *this);
            }
            svc->unsubscribe(other);
        }
    }

    return *this;
}

std::string Subscriber::reprString() const
{
    std::ostringstream os;
    os << "Subscriber()[" << this << "]";
    return os.str();
}

void Subscriber::subscriberRemoved(SubscriptionService &service)
{
    m_subscriptions.erase(&service);
}

bool Subscriber::subscribeTo(SubscriptionService &service)
{
    bool ret = service.subscribe(*this);
    if (ret) m_subscriptions.insert(&service);
    return ret;
}

bool Subscriber::subscribeTo(std::initializer_list<const char*> events_list, SubscriptionService &service)
{
    bool ret = service.subscribe(events_list, *this);
    if (ret) m_subscriptions.insert(&service);
    return ret;
}

MBSTF_NAMESPACE_STOP

std::ostream &operator<<(std::ostream &ostrm, const MBSTF_NAMESPACE_NAME(Subscriber) &subscriber)
{
    ostrm << subscriber.reprString();
    return ostrm;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
