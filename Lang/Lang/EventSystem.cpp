
#include "EventSystem.h"

namespace script
{
	EventManager eventManager;

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

	static void CALLBACK TimerCallback(HWND _hWnd, UINT _msg, UINT_PTR _idTimer, DWORD _dwTime)
	{
		// Timer is done

		std::function<void()> callback = Timer::GetInstance().GetCallbackForID(_idTimer);

		if (callback)
			callback();

		KillTimer(_hWnd, _idTimer);
		Timer::GetInstance().Remove(_idTimer);
	}


#else
#error Only Windows is supported. 
#endif

	void EventManager::TranslateMessages()
	{
#ifdef _WIN32
		MSG msg;

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
#endif
	}



	void Timer::StartTimer(uint64_t durationMs, std::function<void()> callback)
	{
#ifdef _WIN32
		m_Callbacks[SetTimer(NULL, 0, durationMs, TimerCallback)] = callback;
#endif
		m_Count++;
	}
}