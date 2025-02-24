/*
 */

#include <condition_variable>
#include <exception>
#include <iostream>
#include <optional>
#include <mutex>

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

    int data() const { return m_data; };

private:
    int m_data;
};

class EventType2 : public Event
{
public:
    EventType2(int data) :Event("Type2"),m_data(data) {};

    virtual ~EventType2() {};

    virtual Event clone() {return EventType2(m_data);};

    int data() const { return m_data; };

private:
    int m_data;
};


class TestSubscriptionService : public SubscriptionService
{
public:
    TestSubscriptionService() :SubscriptionService() {};

    bool sendSynchronous(Event &event) {return sendEventSynchronous(event);};
    void sendAsynchronous(const std::shared_ptr<Event> &event) {sendEventAsynchronous(event);};
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

class TestAsyncSubscriber : public Subscriber
{
public:
    TestAsyncSubscriber() :Subscriber(),m_eventResult(std::nullopt),m_condMutex(),m_condVar() {};

    virtual ~TestAsyncSubscriber() {};

    virtual void processEvent(Event &event, SubscriptionService &event_service) {
        std::cout << "TestAsyncSubscriber[" << this << "]: " << event_service << " sent: " << event << std::endl;
        if (event.eventName() == "Type1") {
            EventType1 &ev1 = dynamic_cast<EventType1&>(event);
            std::lock_guard guard(m_condMutex);
            m_eventResult = ev1.data();
            m_condVar.notify_all();
        }   
        event.preventDefault();
    };

    virtual void subscriberRemoved(SubscriptionService &service) {
        std::cout << "TestAsyncSubscriber[" << this << "]: Subscription service " << service << " has gone away" << std::endl;
        Subscriber::subscriberRemoved(service);
    };

    void waitForEvent() {
        std::lock_guard guard(m_condMutex);
        std::cout << "TestAsyncSubscriber[" << this << "]: Wait for event" << std::endl;
        if (!m_eventResult) {
            m_condVar.wait(m_condMutex);
        }
        std::cout << "TestAsyncSubscriber[" << this << "]: got " << m_eventResult.value() << std::endl;
    };

    void addSubscribeToAll(SubscriptionService &svc) {
        if (!subscribeTo(svc)) {
            std::cout << "TestAsyncSubscriber[" << this << "]: ERROR: Failed subscription all" << std::endl;
            throw std::runtime_error("Failed subscription");
        }
    };

private:
    std::optional<int> m_eventResult;
    std::recursive_mutex m_condMutex;
    std::condition_variable_any m_condVar;
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

    // async test

    TestAsyncSubscriber *asubscriber1 = new TestAsyncSubscriber();
    TestSubscriptionService *test_svc3 = new TestSubscriptionService;
    std::shared_ptr<Event> t5(new EventType1(1));
    
    asubscriber1->addSubscribeToAll(*test_svc3);

    test_svc3->sendAsynchronous(t5);
    t5.reset();

    asubscriber1->waitForEvent();

    delete asubscriber1;
    delete test_svc3;

    return 0;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
