/*
 */

#include <exception>
#include <iostream>

#include "common.hh"
#include "SubscriptionService.hh"
#include "Subscriber.hh"
#include "Event.hh"

MBSTF_NAMESPACE_USING;

class EventType1 : public Event
{
public:
    EventType1(int data) :Event("Type1"),m_data(data) {};

    virtual ~EventType1() {};

    virtual Event clone() {return EventType1(m_data);};

private:
    int m_data;
};

class EventType2 : public Event
{
public:
    EventType2(int data) :Event("Type2"),m_data(data) {};

    virtual ~EventType2() {};

    virtual Event clone() {return EventType2(m_data);};

private:
    int m_data;
};


class TestSubscriptionService : public SubscriptionService
{
public:
    TestSubscriptionService() :SubscriptionService() {};

    bool sendSynchronous(Event &event) {return sendEventSynchronous(event);};
};

class TestSubscriber : public Subscriber
{
public:
    TestSubscriber() :Subscriber() {};
    TestSubscriber(SubscriptionService &svc) :Subscriber() {
        addSubscribeToT2(svc);
    };

    void addSubscribeToAll(SubscriptionService &svc) {
        if (!subscribeTo(svc)) {
            std::cout << "TestSubscriber[" << this << "]: ERROR: Failed subscription all" << std::endl;
            throw std::runtime_error("Failed subscription");
        }
    }

    void addSubscribeToT1(SubscriptionService &svc) {
        if (!subscribeTo({"Type1"}, svc)) {
            std::cout << "TestSubscriber[" << this << "]: ERROR: Failed subscription T1" << std::endl;
            throw std::runtime_error("Failed subscription");
        }
    }
    
    void addSubscribeToT2(SubscriptionService &svc) {
        if (!subscribeTo({"Type2"}, svc)) {
            std::cout << "TestSubscriber[" << this << "]: ERROR: Failed subscription T2" << std::endl;
            throw std::runtime_error("Failed subscription");
        }
    }

    virtual ~TestSubscriber() { std::cout << "TestSubscriber[" << this << "]: going away" << std::endl; };

    virtual void processEvent(Event &event, SubscriptionService &event_service) {
        std::cout << "TestSubscriber[" << this << "]: " << event_service << " sent: " << event << std::endl;
        event.preventDefault();
    };
    virtual void subscriberRemoved(SubscriptionService &service) {
        std::cout << "TestSubscriber[" << this << "]: Subscription service " << service << " has gone away" << std::endl;
        Subscriber::subscriberRemoved(service);
    };
};

int main(int argc, char *argv[])
{
    TestSubscriptionService *test_svc = new TestSubscriptionService;
    TestSubscriptionService *test_svc2 = new TestSubscriptionService;

    TestSubscriber *subscriber1 = new TestSubscriber(*test_svc);
    subscriber1->addSubscribeToT1(*test_svc);
    subscriber1->addSubscribeToAll(*test_svc);

    TestSubscriber *subscriber2 = new TestSubscriber(*test_svc);
    subscriber2->addSubscribeToT2(*test_svc2);

    TestSubscriber *subscriber3 = new TestSubscriber();
    subscriber3->addSubscribeToT1(*test_svc);
    subscriber3->addSubscribeToAll(*test_svc2);

    EventType1 t1(42);
    EventType2 t2(128);
    EventType2 t3(1);
    EventType2 t4(0);

    std::cout << "main: sendSynchronous(t1) = " << test_svc->sendSynchronous(t1) << std::endl;
    std::cout << "main: sendSynchronous(t2) = " << test_svc->sendSynchronous(t2) << std::endl;

    delete test_svc;

    delete subscriber1;

    std::cout << "main: sendSynchronous(t3) = " << test_svc2->sendSynchronous(t3) << std::endl;

    delete subscriber2;

    std::cout << "main: sendSynchronous(t4) = " << test_svc2->sendSynchronous(t4) << std::endl;

    delete test_svc2;

    delete subscriber3;

    return 0;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
