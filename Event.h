#pragma once

#include <list>
#include <utility>

namespace EventIntern {

	template<typename T1, typename T2>
	static bool IsSameType(T1 p1, T2 p2)
	{
		return (p1 && p2 && typeid(*p1) == typeid(*p2));
	}
}

template<typename ...Args>
class EventHandlerImpl {
public:
	virtual bool IsBindedToSameFunctionAs(EventHandlerImpl<Args...>* pHandler2) = 0;
	virtual void OnEvent(Args&& ...evt) = 0;
};

template<typename ...Args>
class EventHandlerImplForNonMemberFunction : public EventHandlerImpl<Args...> {
public:
	EventHandlerImplForNonMemberFunction(void(*functionToCall)(Args...))
		: m_pFunctionToCall(functionToCall)
	{ }
	
	virtual void OnEvent(Args&& ...evt)
	{
		m_pFunctionToCall(std::forward<Args>(evt)...);
	}

	virtual bool IsBindedToSameFunctionAs(EventHandlerImpl<Args...>* pHandler2)
	{
		if (!EventIntern::IsSameType(this, pHandler2))
			return false;

		EventHandlerImplForNonMemberFunction<Args...>* pHandlerCasted = dynamic_cast<EventHandlerImplForNonMemberFunction<Args...>*>(pHandler2);
		if (!pHandlerCasted)
			return false;

		return this->m_pFunctionToCall == pHandlerCasted->m_pFunctionToCall;
	}

private:
	void(*m_pFunctionToCall)(Args...);
};

template<typename U, typename ...Args>
class EventHandlerImplForMemberFunction : public EventHandlerImpl<Args...> {
public:
	EventHandlerImplForMemberFunction(void(U::* memberFunctionToCall)(Args...), U* thisPtr)
		: m_pCallerInstance(thisPtr)
		, m_pMemberFunction(memberFunctionToCall)
	{ }

	virtual void OnEvent(Args&& ...evt)
	{
		if (m_pCallerInstance)
			(m_pCallerInstance->*m_pMemberFunction)(std::forward<Args>(evt)...);
	}

	virtual bool IsBindedToSameFunctionAs(EventHandlerImpl<Args...>* pHandler2)
	{
		if (!EventIntern::IsSameType(this, pHandler2))
			return false;

		EventHandlerImplForMemberFunction<U, Args...>* pHandlerCasted = dynamic_cast<EventHandlerImplForMemberFunction<U, Args...>*>(pHandler2);
		if (!pHandlerCasted)
			return false;

		return this->m_pCallerInstance == pHandlerCasted->m_pCallerInstance && this->m_pMemberFunction == pHandlerCasted->m_pMemberFunction;
	}

private:
	U* m_pCallerInstance; 
	void(U::* m_pMemberFunction)(Args...);
};

template<>
class EventHandlerImpl<void> {
public:
	virtual bool IsBindedToSameFunctionAs(EventHandlerImpl<void>* pHandler2) = 0;
	virtual void OnEvent() = 0; 
};

template<>
class EventHandlerImplForNonMemberFunction<void> : public EventHandlerImpl<void> {
public:
	EventHandlerImplForNonMemberFunction(void(*functionToCall)())
		: m_pFunctionToCall(functionToCall)
	{ }

	virtual void OnEvent()
	{
		m_pFunctionToCall();
	}

	virtual bool IsBindedToSameFunctionAs(EventHandlerImpl<void>* pHandler2)
	{
		if (!EventIntern::IsSameType(this, pHandler2))
			return false;

		EventHandlerImplForNonMemberFunction<void>* pHandlerCasted = dynamic_cast<EventHandlerImplForNonMemberFunction<void>*>(pHandler2);
		if (!pHandlerCasted)
			return false;

		return this->m_pFunctionToCall == pHandlerCasted->m_pFunctionToCall;
	}

private:
	void(*m_pFunctionToCall)();
};

template<typename U>
class EventHandlerImplForMemberFunction<U, void> : public EventHandlerImpl<void> {
public:
	EventHandlerImplForMemberFunction(void(U::* memberFunctionToCall)(), U* thisPtr)
		: m_pCallerInstance(thisPtr)
		, m_pMemberFunction(memberFunctionToCall)
	{ }

	virtual void OnEvent()
	{
		if (m_pCallerInstance)
			(m_pCallerInstance->*m_pMemberFunction)();
	}

	virtual bool IsBindedToSameFunctionAs(EventHandlerImpl<void>* pHandler2)
	{
		if (!EventIntern::IsSameType(this, pHandler2))
			return false;

		EventHandlerImplForMemberFunction<U, void>* pHandlerCasted = dynamic_cast<EventHandlerImplForMemberFunction<U, void>*>(pHandler2);
		if (!pHandlerCasted)
			return false;

		return this->m_pCallerInstance == pHandlerCasted->m_pCallerInstance && this->m_pMemberFunction == pHandlerCasted->m_pMemberFunction;
	}

private:
	U* m_pCallerInstance; 
	void(U::* m_pMemberFunction)(); 
};

class EventHandler {
public:
	template<typename ...Args>
	static EventHandlerImpl<Args...>* Bind(void(*nonMemberFunctionToCall)(Args...))
	{
		return new EventHandlerImplForNonMemberFunction<Args...>(nonMemberFunctionToCall);
	}

	template<typename U, typename ...Args>
	static EventHandlerImpl<Args...>* Bind(void(U::*memberFunctionToCall)(Args...), U* thisPtr)
	{
		return new EventHandlerImplForMemberFunction<U, Args...>(memberFunctionToCall, thisPtr);
	}
	
	static EventHandlerImpl<void>* Bind(void(*nonMemberFunctionToCall)())
	{
		return new EventHandlerImplForNonMemberFunction<void>(nonMemberFunctionToCall);
	}

	template<typename U>
	static EventHandlerImpl<void>* Bind(void(U::* memberFunctionToCall)(), U* thisPtr)
	{
		return new EventHandlerImplForMemberFunction<U, void>(memberFunctionToCall, thisPtr);
	}

private:
	EventHandler(); 
};

template<typename ...Args>
class EventBase {
public:
	EventBase() {}

	virtual ~EventBase()
	{
		for (auto handler : m_eventHandlers)
			if (handler) {
				delete handler;
				handler = 0; 
			}

		m_eventHandlers.clear();
	}

	EventBase<Args...>& operator += (EventHandlerImpl<Args...>* pHandlerToAdd)
	{
		if (pHandlerToAdd)
			m_eventHandlers.push_back(pHandlerToAdd);

		return *this;
	}

	EventBase<Args...>& operator -= (EventHandlerImpl<Args...>* pHandlerToRemove)
	{
		if (!pHandlerToRemove)
			return *this;
	
		for (auto iter = m_eventHandlers.begin(); iter != m_eventHandlers.end(); ++iter)
		{
			EventHandlerImpl<Args...>* pHandler = *iter;
			if (pHandlerToRemove->IsBindedToSameFunctionAs(pHandler))
			{
				EventHandlerImpl<Args...>* pFoundHandler = *iter;
				if (pFoundHandler)
				{
					delete pFoundHandler;
					pFoundHandler = 0;
				}

				m_eventHandlers.erase(iter);
				break;
			}
		}
		
		if (pHandlerToRemove)
		{
			delete pHandlerToRemove;
			pHandlerToRemove = 0;
		}

		return *this;
	}

private:
	EventBase(const EventBase&); 
	EventBase& operator=(const EventBase&); 

protected:
	std::list< EventHandlerImpl<Args...>* > m_eventHandlers;  
};

template<typename ...Args>
class Event : public EventBase<Args...> {
public:
	void operator()(Args&& ...eventData)
	{
		for (auto handler : this->m_eventHandlers)
			if (handler)
				handler->OnEvent(std::forward<Args>(eventData)...);
	}
};

template<>
class Event<void> : public EventBase<void> {
public:
	void operator()()
	{
		for (auto handler : this->m_eventHandlers)
			if (handler)
				handler->OnEvent();
	}
};
