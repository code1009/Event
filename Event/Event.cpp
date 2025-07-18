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
namespace Test
{
	using EventType = std::int32_t;
}

/////////////////////////////////////////////////////////////////////////////
//===========================================================================
namespace Test
{
	class EventData : public std::enable_shared_from_this<EventData>
	{
	public:
		virtual ~EventData() = default;
	};
}

/////////////////////////////////////////////////////////////////////////////
//===========================================================================
namespace Test
{
	class Event
	{
	private:
		bool _Handled;
		EventType _EventType;
		std::shared_ptr<EventData> _EventData;

	public:
		explicit Event(EventType const& eventType, std::shared_ptr<EventData>& eventData);

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

	Event::Event(EventType const& eventType, std::shared_ptr<EventData>& eventData) :
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
namespace Test
{
	using EventHandler = std::function<void(Event&)>;
}

/////////////////////////////////////////////////////////////////////////////
//===========================================================================
namespace Test
{
	class EventListener
	{
	public:
		using Token = std::uint32_t;

	private:
		Token _CurrentToken;
		std::unordered_map<Token, EventHandler> _EventHandlers;

	public:
		EventListener();

		Token attach(EventHandler const& handler);
		void detach(Token const token);

		void clear();
		bool empty() const;
		void notify(Event& event);
	};

	EventListener::EventListener()
		: _CurrentToken(0)
	{
	}
	EventListener::Token EventListener::attach(EventHandler const& handler)
	{
		_CurrentToken++;
		_EventHandlers[_CurrentToken] = handler;
		return _CurrentToken;
	}
	void EventListener::detach(Token const token)
	{
		_EventHandlers.erase(token);
	}
	void EventListener::clear()
	{
		_EventHandlers.clear();
		_CurrentToken = 0;
	}
	bool EventListener::empty() const
	{
		return _EventHandlers.empty();
	}
	void EventListener::notify(Event& event)
	{
		for (const auto& [token, eventHandler] : _EventHandlers)
		{
			eventHandler(event);
			if (event.handled())
			{
				break;
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
//===========================================================================
namespace Test
{
	class Object;
	using EventTarget = std::shared_ptr<Object>;
}

/////////////////////////////////////////////////////////////////////////////
//===========================================================================
namespace Test
{
	class EventId
	{
	private:
		EventType _EventType;
		EventTarget _EventTarget;

	public:
		EventId(EventType const eventType, EventTarget const& eventTarget);

	public:
		EventType const eventType() const;
		EventTarget const eventTarget() const;
	};

	EventId::EventId(EventType const eventType, EventTarget const& eventTarget) :
		_EventType(eventType), 
		_EventTarget(eventTarget)
	{
	}
	EventType const EventId::eventType() const
	{
		return _EventType;
	}
	EventTarget const EventId::eventTarget() const
	{
		return _EventTarget;
	}

	inline bool operator==(EventId const& lhs, EventId const& rhs)
	{
		return (lhs.eventType() == rhs.eventType() && lhs.eventTarget() == rhs.eventTarget());
	}
	inline bool operator!=(EventId const& lhs, EventId const& rhs)
	{
		return !(lhs == rhs);
	}
	inline bool operator<(EventId const& lhs, EventId const& rhs)
	{
		if (lhs.eventType() < rhs.eventType())
		{
			return true;
		}

		if (lhs.eventType() > rhs.eventType())
		{
			return false;
		}

		return lhs.eventTarget() < rhs.eventTarget();
	}
}

/////////////////////////////////////////////////////////////////////////////
//===========================================================================
namespace Test
{
	class EventDispatcher
	{
	private:
		std::map<EventId, std::shared_ptr<EventListener>> _EventListenerMap;

	public:
		void registerEventListener(EventId const eventId, std::shared_ptr<EventListener> listener);
		void unregisterEventListener(EventId const eventId);
		std::shared_ptr<EventListener> getEventListener(EventId const eventId);

	protected:
		void dispatchEvent(EventId const& eventId, Event& event);

	public:
		void notifyEvent(EventId const& eventId, Event& event);
		void notifyEvent(EventTarget const eventTarget, EventType const eventType, std::shared_ptr<EventData> eventData);
	};

	void EventDispatcher::registerEventListener(EventId const eventId, std::shared_ptr<EventListener> listener)
	{
		_EventListenerMap[eventId] = listener;
	}
	void EventDispatcher::unregisterEventListener(EventId const eventId)
	{
		_EventListenerMap.erase(eventId);
	}
	std::shared_ptr<EventListener> EventDispatcher::getEventListener(EventId const eventId)
	{
		auto it = _EventListenerMap.find(eventId);
		if (it != _EventListenerMap.end())
		{
			return it->second;
		}
		return nullptr;
	}
	void EventDispatcher::dispatchEvent(EventId const& eventId, Event& event)
	{
		auto eventListener = getEventListener(eventId);
		if (eventListener)
		{
			eventListener->notify(event);
		}
	}
	void EventDispatcher::notifyEvent(EventId const& eventId, Event& event)
	{
		dispatchEvent(eventId, event);
	}
	void EventDispatcher::notifyEvent(EventTarget const eventTarget, EventType const eventType, std::shared_ptr<EventData> eventData)
	{
		EventId eventId{ eventType, eventTarget };
		Event event{ eventType, eventData };
		notifyEvent(eventId, event);
	}
}

/////////////////////////////////////////////////////////////////////////////
//===========================================================================
namespace Test
{
	class EventHandlerRegistry
	{
	private:
		EventDispatcher& _EventDispatcher;
		std::map<EventId, EventListener::Token> _EventTokenMap;

	public:
		explicit EventHandlerRegistry(EventDispatcher& eventDispatcher);

		void registerEventHandler(
			EventType const eventType,
			EventTarget const& target,
			EventHandler const& handler);

		void unregisterTarget(EventTarget const& target);
	};

	EventHandlerRegistry::EventHandlerRegistry(EventDispatcher & eventDispatcher)
		: _EventDispatcher(eventDispatcher)
	{
	}
	void EventHandlerRegistry::registerEventHandler(
		EventType const eventType,
		EventTarget const& target,
		EventHandler const& handler)
	{
		EventId eventId{ eventType, target };

		auto listener = _EventDispatcher.getEventListener(eventId);
		if (!listener)
		{
			listener = std::make_shared<EventListener>();
			_EventDispatcher.registerEventListener(eventId, listener);
		}

		auto token = listener->attach(handler);
		_EventTokenMap[eventId] = token;
	}
	void EventHandlerRegistry::unregisterTarget(EventTarget const& target)
	{
		std::vector<EventId> removeEventIds;
		for (auto& [eventId, token] : _EventTokenMap)
		{
			if (eventId.eventTarget() == target)
			{
				auto listener = _EventDispatcher.getEventListener(eventId);
				if (listener)
				{
					listener->detach(token);
					if (listener->empty())
					{
						_EventDispatcher.unregisterEventListener(eventId);
					}
				}
				removeEventIds.push_back(eventId);
			}
		}
		for (const auto& eventId : removeEventIds)
		{
			_EventTokenMap.erase(eventId);
		}
	}
}





/////////////////////////////////////////////////////////////////////////////
//===========================================================================
namespace Test
{
	class ObjectEventData : public EventData
	{
	public:
		int value;

	public:
		explicit ObjectEventData(int val) : value(val)
		{
		}
	};

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

		void eventHandler_A(Event& event)
		{
			std::cout
				<< std::format("[{}] ", _Id)
				<< "eventHandler_A:"
				<< " type=" << event.eventType()
				<< " value=" << event.eventDataAs<ObjectEventData>()->value
				<< std::endl
				;

			event.handled(true);
		}

		void eventHandler_B(Event& event)
		{
			std::cout
				<< std::format("[{}] ", _Id)
				<< "eventHandler_B:"
				<< " type=" << event.eventType()
				<< " value=" << event.eventDataAs<ObjectEventData>()->value
				<< std::endl
				;

			event.handled(true);
		}
	};
}

/////////////////////////////////////////////////////////////////////////////
//===========================================================================
int main()
{
	using namespace Test;

	const EventType EventType_A = 1;
	const EventType EventType_B = 2;

	std::shared_ptr<Object> object1 = std::make_shared<Object>(1);
	std::shared_ptr<Object> object2 = std::make_shared<Object>(2);

	EventDispatcher eventDispatcher;
	EventHandlerRegistry eventHandlerRegistry(eventDispatcher);


	eventHandlerRegistry.registerEventHandler(
		EventType_A,
		object1,
		std::bind(&Object::eventHandler_A, object1, std::placeholders::_1)
	);
	eventHandlerRegistry.registerEventHandler(
		EventType_B,
		object1,
		std::bind(&Object::eventHandler_B, object1, std::placeholders::_1)
	);
	eventHandlerRegistry.registerEventHandler(
		EventType_A,
		object2,
		std::bind(&Object::eventHandler_A, object2, std::placeholders::_1)
	);
	eventHandlerRegistry.registerEventHandler(
		EventType_B,
		object2,
		std::bind(&Object::eventHandler_B, object2, std::placeholders::_1)
	);


	eventDispatcher.notifyEvent(object1, EventType_A, std::make_shared<ObjectEventData>(101));
	eventDispatcher.notifyEvent(object1, EventType_B, std::make_shared<ObjectEventData>(102));
	eventDispatcher.notifyEvent(object2, EventType_A, std::make_shared<ObjectEventData>(103));
	eventDispatcher.notifyEvent(object2, EventType_B, std::make_shared<ObjectEventData>(104));


	eventHandlerRegistry.unregisterTarget(object1);


	eventDispatcher.notifyEvent(object1, EventType_B, std::make_shared<ObjectEventData>(105));
	eventDispatcher.notifyEvent(object2, EventType_B, std::make_shared<ObjectEventData>(106));

	return 0;
}
