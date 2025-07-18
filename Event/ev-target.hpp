/////////////////////////////////////////////////////////////////////////////
//===========================================================================
namespace ev::target
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

	public:
		Token attach(EventHandler const& eventHandler);
		void detach(Token const token);

	public:
		void clear();
		bool empty() const;

	public:
		void notify(Event& event);
		void notify(EventType const eventType, std::shared_ptr<EventData> eventData);
	};

	EventListener::EventListener()
		: _CurrentToken(0)
	{
	}
	EventListener::Token EventListener::attach(EventHandler const& eventHandler)
	{
		_CurrentToken++;
		_EventHandlers[_CurrentToken] = eventHandler;
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
	void EventListener::notify(EventType const eventType, std::shared_ptr<EventData> eventData)
	{
		Event event{ eventType, eventData };
		notify(event);
	}
}

/////////////////////////////////////////////////////////////////////////////
//===========================================================================
namespace app
{
	class Object;
}

namespace ev::target
{
	using EventTarget = std::shared_ptr<app::Object>;
}

/////////////////////////////////////////////////////////////////////////////
//===========================================================================
namespace ev::target
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
namespace ev::target
{
	class EventDispatcher
	{
	private:
		std::map<EventId, std::shared_ptr<EventListener>> _EventListenerMap;

	public:
		void registerEventListener(EventId const eventId, std::shared_ptr<EventListener> eventListener);
		void unregisterEventListener(EventId const eventId);
		std::shared_ptr<EventListener> getEventListener(EventId const eventId);

	protected:
		void dispatchEvent(EventId const& eventId, Event& event);

	public:
		void notifyEvent(EventId const& eventId, Event& event);
		void notifyEvent(EventTarget const eventTarget, EventType const eventType, std::shared_ptr<EventData> eventData);
	};

	void EventDispatcher::registerEventListener(EventId const eventId, std::shared_ptr<EventListener> eventListener)
	{
		_EventListenerMap[eventId] = eventListener;
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
namespace ev::target
{
	class EventHandlerRegistry
	{
	private:
		EventDispatcher& _EventDispatcher;
		std::map<EventId, EventListener::Token> _EventTokenMap;

	public:
		explicit EventHandlerRegistry(EventDispatcher& eventDispatcher);

		void registerEventHandler(
			EventTarget const& eventTarget,
			EventType const eventType,
			EventHandler const& eventHandler);

		void unregisterEventHandler(EventTarget const& eventTarget);
	};

	EventHandlerRegistry::EventHandlerRegistry(EventDispatcher & eventDispatcher) :
		_EventDispatcher(eventDispatcher)
	{
	}
	void EventHandlerRegistry::registerEventHandler(
		EventTarget const& eventTarget,
		EventType const eventType,
		EventHandler const& eventHandler)
	{
		EventId eventId{ eventType, eventTarget };

		auto eventListener = _EventDispatcher.getEventListener(eventId);
		if (!eventListener)
		{
			eventListener = std::make_shared<EventListener>();
			_EventDispatcher.registerEventListener(eventId, eventListener);
		}

		auto token = eventListener->attach(eventHandler);
		_EventTokenMap[eventId] = token;
	}
	void EventHandlerRegistry::unregisterEventHandler(EventTarget const& eventTarget)
	{
		std::vector<EventId> eventIds;
		for (auto& [eventId, token] : _EventTokenMap)
		{
			if (eventId.eventTarget() == eventTarget)
			{
				auto eventListener = _EventDispatcher.getEventListener(eventId);
				if (eventListener)
				{
					eventListener->detach(token);
					if (eventListener->empty())
					{
						_EventDispatcher.unregisterEventListener(eventId);
					}
				}
				eventIds.push_back(eventId);
			}
		}
		for (const auto& eventId : eventIds)
		{
			_EventTokenMap.erase(eventId);
		}
	}
}
