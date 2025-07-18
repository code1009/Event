/////////////////////////////////////////////////////////////////////////////
//===========================================================================
#include <iostream>
#include <memory>
#include <map>
#include <format>
#include <functional>
#include <unordered_map>

/////////////////////////////////////////////////////////////////////////////
//===========================================================================
namespace ev
{
	using EventType = std::int32_t;
}

/////////////////////////////////////////////////////////////////////////////
//===========================================================================
namespace ev
{
	class EventData : public std::enable_shared_from_this<EventData>
	{
	public:
		virtual ~EventData() = default;
	};
}

/////////////////////////////////////////////////////////////////////////////
//===========================================================================
namespace ev
{
	class Event
	{
	private:
		EventType _EventType;
		std::shared_ptr<EventData> _EventData;
		bool _Handled{ false };

	public:
		explicit Event(EventType const eventType, std::shared_ptr<EventData>& eventData);

	public:
		virtual ~Event() = default;

	public:
		EventType eventType() const;
		std::shared_ptr<EventData> eventData() const;
		template<typename T> std::shared_ptr<T> eventDataAs() const;

	public:
		bool handled() const;
		void handled(bool const handled);
	};

	template<typename T>
	std::shared_ptr<T> Event::eventDataAs() const
	{
		return std::dynamic_pointer_cast<T>(_EventData);
	}

	Event::Event(EventType const eventType, std::shared_ptr<EventData>& eventData) :
		_Handled(false), 
		_EventType(eventType), 
		_EventData(eventData)
	{
	}
	EventType Event::eventType() const
	{
		return _EventType;
	}
	std::shared_ptr<EventData> Event::eventData() const
	{
		return _EventData;
	}
	bool Event::handled() const
	{
		return _Handled;
	}
	void Event::handled(bool const handled)
	{
		_Handled = handled;
	}
}

/////////////////////////////////////////////////////////////////////////////
//===========================================================================
namespace ev
{
	using EventHandler = std::function<void(Event&)>;
}



/////////////////////////////////////////////////////////////////////////////
//===========================================================================
#include "ev.hpp"
#include "ev-key.hpp"


/////////////////////////////////////////////////////////////////////////////
//===========================================================================
namespace ev
{
	class ObjectEventData : public ev::EventData
	{
	public:
		int value;

	public:
		explicit ObjectEventData(int val) : value(val)
		{
		}
	};
}

/////////////////////////////////////////////////////////////////////////////
//===========================================================================
namespace ev
{
	class Object : public std::enable_shared_from_this<Object>
	{
	private:
		std::uint32_t _Id;

	public:
		explicit Object(std::uint32_t id) : _Id(id)
		{
		}

	public:
		virtual ~Object() = default;

		void eventHandler_A(ev::Event& event)
		{
			std::cout
				<< std::format("[{}] ", _Id)
				<< "eventHandler_A:"
				<< " type=" << event.eventType()
				<< " value=" << event.eventDataAs<ObjectEventData>()->value
				<< std::endl
				;

			//event.handled(true);
		}

		void eventHandler_B(ev::Event& event)
		{
			std::cout
				<< std::format("[{}] ", _Id)
				<< "eventHandler_B:"
				<< " type=" << event.eventType()
				<< " value=" << event.eventDataAs<ObjectEventData>()->value
				<< std::endl
				;

			//event.handled(true);
		}

		void eventHandler_C(ev::Event& event)
		{
			std::cout
				<< std::format("[{}] ", _Id)
				<< "eventHandler_B:"
				<< " type=" << event.eventType()
				<< " is_null=" << (event.eventData() == nullptr ? "null" : "not null")
				<< std::endl
				;

			//event.handled(true);
		}
	};
}

/////////////////////////////////////////////////////////////////////////////
//===========================================================================
void test1(void)
{
	const ev::EventType EventType_A = 1;
	const ev::EventType EventType_B = 2;
	const ev::EventType EventType_C = 3;

	std::shared_ptr<ev::Object> object1 = std::make_shared<ev::Object>(1);
	std::shared_ptr<ev::Object> object2 = std::make_shared<ev::Object>(2);

	ev::EventDispatcher eventDispatcher;
	ev::EventHandlerRegistry eventHandlerRegistry(eventDispatcher);


	eventHandlerRegistry.registerEventHandler(
		object1,
		EventType_A,
		std::bind(&ev::Object::eventHandler_A, object1, std::placeholders::_1)
	);
	eventHandlerRegistry.registerEventHandler(
		object1,
		EventType_B,
		std::bind(&ev::Object::eventHandler_B, object1, std::placeholders::_1)
	);

	eventHandlerRegistry.registerEventHandler(
		object2,
		EventType_A,
		std::bind(&ev::Object::eventHandler_A, object2, std::placeholders::_1)
	);
	eventHandlerRegistry.registerEventHandler(
		object2,
		EventType_B,
		std::bind(&ev::Object::eventHandler_B, object2, std::placeholders::_1)
	);
	eventHandlerRegistry.registerEventHandler(
		object2,
		EventType_C,
		std::bind(&ev::Object::eventHandler_C, object2, std::placeholders::_1)
	);

	eventDispatcher.notifyEvent(object1, EventType_A, std::make_shared<ev::ObjectEventData>(101));
	eventDispatcher.notifyEvent(object1, EventType_B, std::make_shared<ev::ObjectEventData>(102));
	eventDispatcher.notifyEvent(object2, EventType_A, std::make_shared<ev::ObjectEventData>(103));
	eventDispatcher.notifyEvent(object2, EventType_B, std::make_shared<ev::ObjectEventData>(104));


	eventHandlerRegistry.unregisterEventHandler(object1);


	eventDispatcher.notifyEvent(object1, EventType_B, std::make_shared<ev::ObjectEventData>(105));
	eventDispatcher.notifyEvent(object2, EventType_B, std::make_shared<ev::ObjectEventData>(106));
	eventDispatcher.notifyEvent(object2, EventType_C, nullptr);
	eventDispatcher.notifyEvent(object2, EventType_C, std::make_shared<ev::ObjectEventData>(107));
}

/////////////////////////////////////////////////////////////////////////////
//===========================================================================
void test2(void)
{
	const ev::EventType EventType_A = 1;
	const ev::EventType EventType_B = 2;

	std::shared_ptr<ev::Object> object1 = std::make_shared<ev::Object>(3);
	std::shared_ptr<ev::Object> object2 = std::make_shared<ev::Object>(4);

	ev::key::EventListener eventListener;

	eventListener.attach(
		object1.get(),
		std::bind(&ev::Object::eventHandler_A, object1, std::placeholders::_1)
	);
	eventListener.attach(
		object1.get(),
		std::bind(&ev::Object::eventHandler_B, object1, std::placeholders::_1)
	);
	eventListener.attach(
		object2.get(),
		std::bind(&ev::Object::eventHandler_A, object2, std::placeholders::_1)
	);
	eventListener.attach(
		object2.get(),
		std::bind(&ev::Object::eventHandler_B, object2, std::placeholders::_1)
	);

	std::shared_ptr<ev::EventData> eventData = std::make_shared<ev::ObjectEventData>(211);
	ev::Event event_A{ EventType_A, eventData };
	eventListener.notify(event_A);

	eventListener.notify(EventType_B, std::make_shared<ev::ObjectEventData>(212));

	eventListener.detach(object1.get());
	eventListener.notify(EventType_A, std::make_shared<ev::ObjectEventData>(213));
	eventListener.notify(EventType_B, std::make_shared<ev::ObjectEventData>(214));
}

/////////////////////////////////////////////////////////////////////////////
//===========================================================================
void test3(void)
{
	const ev::EventType EventType_A = 1;
	const ev::EventType EventType_B = 2;
	const ev::EventType EventType_C = 3;

	std::shared_ptr<ev::Object> object1 = std::make_shared<ev::Object>(1);
	std::shared_ptr<ev::Object> object2 = std::make_shared<ev::Object>(2);

	ev::key::EventDispatcher eventDispatcher;
	ev::key::EventHandlerRegistry eventHandlerRegistry(eventDispatcher);


	eventHandlerRegistry.registerEventHandler(
		object1.get(),
		EventType_A,
		std::bind(&ev::Object::eventHandler_A, object1, std::placeholders::_1)
	);
	eventHandlerRegistry.registerEventHandler(
		object1.get(),
		EventType_B,
		std::bind(&ev::Object::eventHandler_B, object1, std::placeholders::_1)
	);

	eventHandlerRegistry.registerEventHandler(
		object2.get(),
		EventType_A,
		std::bind(&ev::Object::eventHandler_A, object2, std::placeholders::_1)
	);
	eventHandlerRegistry.registerEventHandler(
		object2.get(),
		EventType_B,
		std::bind(&ev::Object::eventHandler_B, object2, std::placeholders::_1)
	);
	eventHandlerRegistry.registerEventHandler(
		object2.get(),
		EventType_C,
		std::bind(&ev::Object::eventHandler_C, object2, std::placeholders::_1)
	);

	eventDispatcher.notifyEvent(EventType_A, std::make_shared<ev::ObjectEventData>(101));
	eventDispatcher.notifyEvent(EventType_B, std::make_shared<ev::ObjectEventData>(102));
	eventDispatcher.notifyEvent(EventType_A, std::make_shared<ev::ObjectEventData>(103));
	eventDispatcher.notifyEvent(EventType_B, std::make_shared<ev::ObjectEventData>(104));


	eventHandlerRegistry.unregisterEventHandler(object1.get());


	eventDispatcher.notifyEvent(EventType_B, std::make_shared<ev::ObjectEventData>(105));
	eventDispatcher.notifyEvent(EventType_B, std::make_shared<ev::ObjectEventData>(106));
	eventDispatcher.notifyEvent(EventType_C, nullptr);
	eventDispatcher.notifyEvent(EventType_C, std::make_shared<ev::ObjectEventData>(107));
}

/////////////////////////////////////////////////////////////////////////////
//===========================================================================
int main()
{
	std::cout
		<< "-----------------------------------------------------------------"
		<< std::endl
		;
	test1();
	std::cout
		<< std::endl
		<< std::endl
		<< std::endl
		;



	std::cout
		<< "-----------------------------------------------------------------"
		<< std::endl
		;
	test2();
	std::cout
		<< std::endl
		<< std::endl
		<< std::endl
		;



	std::cout
		<< "-----------------------------------------------------------------"
		<< std::endl
		;
	test3();
	std::cout
		<< std::endl
		<< std::endl
		<< std::endl
		;

	return 0;
}
