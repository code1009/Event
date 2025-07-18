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
