#pragma once

#include <vector>
#include <memory>
#include <mutex>

namespace EventIntern {

	template<typename T1, typename T2>
	static bool IsSameType(T1 p1, T2 p2)
	{
		return (p1 && p2 && typeid(*p1) == typeid(*p2));
	}

	template<typename ...Args>
	class EventHandlerImpl {
	public:
		virtual bool IsBindedToSameFunctionAs(EventHandlerImpl<Args...>* pHandler2) = 0;
		virtual void OnEvent(Args... evt) = 0;
	};

	template<typename ...Args>
	class EventHandlerImplForNonMemberFunction : public EventHandlerImpl<Args...> {
	public:
		EventHandlerImplForNonMemberFunction(void(*functionToCall)(Args...))
			: m_pFunctionToCall(functionToCall)
		{ }

		virtual void OnEvent(Args... evt)
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

		virtual void OnEvent(Args... evt)
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

		EventBase<Args...>& Add(void(*nonMemberFunctionToCall)(Args...))
		{
			return operator+=(EventHandler::Bind(nonMemberFunctionToCall));
		}

		template <typename U>
		EventBase<Args...>& Add(void(U::* memberFunctionToCall)(Args...), U* thisPtr)
		{
			return operator+=(EventHandler::Bind(memberFunctionToCall, thisPtr));
		}

		EventBase<void>& Add(void(*nonMemberFunctionToCall)())
		{
			return operator+=(EventHandler::Bind(nonMemberFunctionToCall));
		}

		template <typename U>
		EventBase<void>& Add(void(U::* memberFunctionToCall)(), U* thisPtr)
		{
			return operator+=(EventHandler::Bind(memberFunctionToCall, thisPtr));
		}

		EventBase<Args...>& Remove(void(*nonMemberFunctionToCall)(Args...))
		{
			return operator-=(EventHandler::Bind(nonMemberFunctionToCall));
		}

		template <typename U>
		EventBase<Args...>& Remove(void(U::* memberFunctionToCall)(Args...), U* thisPtr)
		{
			return operator-=(EventHandler::Bind(memberFunctionToCall, thisPtr));
		}

		EventBase<void>& Remove(void(*nonMemberFunctionToCall)())
		{
			return operator-=(EventHandler::Bind(nonMemberFunctionToCall));
		}

		template <typename U>
		EventBase<void>& Remove(void(U::* memberFunctionToCall)(), U* thisPtr)
		{
			return operator-=(EventHandler::Bind(memberFunctionToCall, thisPtr));
		}


		EventBase<Args...>& operator += (void(*nonMemberFunctionToCall)())
		{
			return operator+=(EventHandler::Bind(nonMemberFunctionToCall));
		}

		EventBase<Args...>& operator += (EventHandlerImpl<Args...>* pHandlerToAdd)
		{
			m_addMutex.lock();

			if (pHandlerToAdd)
				m_eventHandlers.push_back(pHandlerToAdd);

			m_addMutex.unlock();

			return *this;
		}

		EventBase<Args...>& operator -= (void(*nonMemberFunctionToCall)())
		{
			return operator-=(EventHandler::Bind(nonMemberFunctionToCall));
		}

		EventBase<Args...>& operator -= (EventHandlerImpl<Args...>* pHandlerToRemove)
		{
			if (!pHandlerToRemove)
				return *this;

			m_addMutex.lock();

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

			m_addMutex.unlock();

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
		std::mutex m_addMutex;
		std::vector< EventHandlerImpl<Args...>* > m_eventHandlers;
	};
}

template<typename ...Args>
class Event : public EventIntern::EventBase<Args...> {
public:
	void operator()(Args... eventData)
	{
		m_updateMutex.lock();

		for (auto handler : this->m_eventHandlers)
			if (handler)
				handler->OnEvent(std::forward<Args>(eventData)...);

		m_updateMutex.unlock();
	}

private:
	std::mutex m_updateMutex;
};

template<>
class Event<void> : public EventIntern::EventBase<void> {
public:
	void operator()()
	{
		m_updateMutex.lock();

		for (auto handler : this->m_eventHandlers)
			if (handler)
				handler->OnEvent();

		m_updateMutex.unlock();
	}

private:
	std::mutex m_updateMutex;
};

class EventHandler {
public:
	template<typename ...Args>
	static EventIntern::EventHandlerImpl<Args...>* Bind(void(*nonMemberFunctionToCall)(Args...))
	{
		return new EventIntern::EventHandlerImplForNonMemberFunction<Args...>(nonMemberFunctionToCall);
	}

	template<typename U, typename ...Args>
	static EventIntern::EventHandlerImpl<Args...>* Bind(void(U::* memberFunctionToCall)(Args...), U* thisPtr)
	{
		return new EventIntern::EventHandlerImplForMemberFunction<U, Args...>(memberFunctionToCall, thisPtr);
	}

	static EventIntern::EventHandlerImpl<void>* Bind(void(*nonMemberFunctionToCall)())
	{
		return new EventIntern::EventHandlerImplForNonMemberFunction<void>(nonMemberFunctionToCall);
	}

	template<typename U>
	static EventIntern::EventHandlerImpl<void>* Bind(void(U::* memberFunctionToCall)(), U* thisPtr)
	{
		return new EventIntern::EventHandlerImplForMemberFunction<U, void>(memberFunctionToCall, thisPtr);
	}

private:
	EventHandler();
};