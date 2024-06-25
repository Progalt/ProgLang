
#pragma once

#include <queue>
#include <map>
#include <functional>

namespace script
{
	
	enum EventType
	{
		// Pushes a fiber to the fiber stack
		EVENT_PUSH_FIBER, 

		// Pops a fiber from the fiber stack
		EVENT_POP_FIBER, 

		// Trigger the garbage collector
		EVENT_TRIGGER_GC, 
	};

	class ObjFiber;

	struct Event
	{
		EventType type;

		union
		{
			ObjFiber* fiber;
		};
	};

	class EventManager
	{
	public:

		void TranslateMessages();

		bool IsEmpty()
		{
			return m_EventQueue.empty();
		}

		Event Pop()
		{
			Event evnt = m_EventQueue.front();
			m_EventQueue.pop();
			size--;

			return evnt;
		}

		void Push(Event evnt)
		{
			m_EventQueue.push(evnt);
			size++;
		}


		std::queue<Event> m_EventQueue;
		size_t size = 0;

	};

	extern EventManager eventManager;


	class Timer
	{
	public:

		static Timer& GetInstance()
		{
			static Timer timer;

			return timer;
		}

		void StartTimer(uint64_t durationMs, std::function<void()> callback);

		std::function<void()> GetCallbackForID(size_t id) { return m_Callbacks[id]; }

		size_t GetActiveTimerCount() { return m_Count; }

		void Remove(size_t id) { m_Callbacks.erase(id); m_Count--; }

	private:

		Timer() { }

		Timer(Timer const&) = delete;
		void operator=(Timer const&) = delete;

		size_t m_Count = 0;

		std::map<size_t, std::function<void()>> m_Callbacks;
	};
}