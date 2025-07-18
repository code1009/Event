/////////////////////////////////////////////////////////////////////////////
//===========================================================================
namespace evt::keyed
{
	using Key = void*;
}

namespace evt::keyed
{
	class EventListener
	{
	private:
		std::unordered_map<Key, EventHandler> _EventHandlers;

	public:
		void attach(Key const& key, EventHandler const& eventHandler);
		void detach(Key const& key);

	public:
		void clear();
		bool empty() const;

	public:
		void notify(Event& event);
		void notify(EventType const eventType, std::shared_ptr<EventData> eventData);
	};

	void EventListener::attach(Key const& key, EventHandler const& eventHandler)
	{
		_EventHandlers[key] = eventHandler;
	}
	void EventListener::detach(Key const& key)
	{
		_EventHandlers.erase(key);
	}
	void EventListener::clear()
	{
		_EventHandlers.clear();
	}
	bool EventListener::empty() const
	{
		return _EventHandlers.empty();
	}
	void EventListener::notify(Event& event)
	{
		for (const auto& [eventTarget, eventHandler] : _EventHandlers)
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
namespace evt::keyed
{
	class EventDispatcher
	{
	private:
		std::map<EventType, std::shared_ptr<EventListener>> _EventListenerMap;

	public:
		void registerEventListener(EventType const eventType, std::shared_ptr<EventListener> eventListener);
		void unregisterEventListener(EventType const eventType);
		void unregisterEventHandler(Key const key);
		std::shared_ptr<EventListener> getEventListener(EventType const eventType);

	protected:
		void dispatchEvent(EventType const eventType, Event& event);

	public:
		void notifyEvent(EventType const eventType, Event& event);
		void notifyEvent(EventType const eventType, std::shared_ptr<EventData> eventData);
	};

	void EventDispatcher::registerEventListener(EventType const eventType, std::shared_ptr<EventListener> eventListener)
	{
		_EventListenerMap[eventType] = eventListener;
	}
	void EventDispatcher::unregisterEventListener(EventType const eventType)
	{
		_EventListenerMap.erase(eventType);
	}
	void EventDispatcher::unregisterEventHandler(Key const key)
	{
		std::vector<EventType> removeEventTypes;
		for (auto& [eventType, eventListener] : _EventListenerMap)
		{
			if (eventListener)
			{
				eventListener->detach(key);
				if (eventListener->empty())
				{
					removeEventTypes.push_back(eventType);
				}
			}
		}
		for (const auto& eventType : removeEventTypes)
		{
			_EventListenerMap.erase(eventType);
		}
	}
	std::shared_ptr<EventListener> EventDispatcher::getEventListener(EventType const eventType)
	{
		auto it = _EventListenerMap.find(eventType);
		if (it != _EventListenerMap.end())
		{
			return it->second;
		}
		return nullptr;
	}
	void EventDispatcher::dispatchEvent(EventType const eventType, Event& event)
	{
		auto eventListener = getEventListener(eventType);
		if (eventListener)
		{
			eventListener->notify(event);
		}
	}
	void EventDispatcher::notifyEvent(EventType const eventType, Event& event)
	{
		dispatchEvent(eventType, event);
	}
	void EventDispatcher::notifyEvent(EventType const eventType, std::shared_ptr<EventData> eventData)
	{
		Event event{ eventType, eventData };
		notifyEvent(eventType, event);
	}
}

/////////////////////////////////////////////////////////////////////////////
//===========================================================================
namespace evt::keyed
{
	class EventHandlerRegistry
	{
	private:
		EventDispatcher& _EventDispatcher;

	public:
		explicit EventHandlerRegistry(EventDispatcher& eventDispatcher);

		void registerEventHandler(
			Key const key,
			EventType const eventType,
			EventHandler const& eventHandler);

		void unregisterEventHandler(Key const key);
	};

	EventHandlerRegistry::EventHandlerRegistry(EventDispatcher& eventDispatcher) :
		_EventDispatcher(eventDispatcher)
	{
	}
	void EventHandlerRegistry::registerEventHandler(
		Key const key,
		EventType const eventType,
		EventHandler const& eventHandler)
	{
		auto eventListener = _EventDispatcher.getEventListener(eventType);
		if (!eventListener)
		{
			eventListener = std::make_shared<EventListener>();
			_EventDispatcher.registerEventListener(eventType, eventListener);
		}
		eventListener->attach(key, eventHandler);
	}
	void EventHandlerRegistry::unregisterEventHandler(Key const key)
	{
		_EventDispatcher.unregisterEventHandler(key);
	}
}
