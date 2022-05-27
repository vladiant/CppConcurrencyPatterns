#pragma once

#include <memory>

#include "i_bank_account.hpp"
#include "event_loop.hpp"

class ThreadSafeAccount : public IBankAccount
{
public:
	ThreadSafeAccount(
		std::shared_ptr<EventLoop> eventLoop,
		std::shared_ptr<IBankAccount> unknownBankAccount) : 
		m_eventLoop(std::move(eventLoop)),
		m_unknownBankAccount(std::move(unknownBankAccount))
	{
	}
	
	void pay(unsigned amount) noexcept override
	{
		//don't use this alternative because [=] or [&] captures this,
		//but not std::shared_ptr.
		//m_eventLoop->enqueue([=]()
		//{
		//	m_unknownBankAccount->pay(amount);
		//});
		
		//use this alternative instead
		m_eventLoop->enqueue(std::bind(
			&IBankAccount::pay, m_unknownBankAccount, amount));
	}
	void acquire(unsigned amount) noexcept override
	{
		m_eventLoop->enqueue(std::bind(
			&IBankAccount::acquire, m_unknownBankAccount, amount));
	}
	long long balance() const noexcept override
	{
		//capturing via [&] is perfectly valid here
		return m_eventLoop->enqueueSync([&]
		{
			return m_unknownBankAccount->balance();
		});
		
		//or you can use this variant for consistency
		//return m_eventLoop->enqueueSync(
		//	&IBankAccount::balance, m_unknownBankAccount);
	}
private:
	std::shared_ptr<EventLoop> m_eventLoop;
	std::shared_ptr<IBankAccount> m_unknownBankAccount;
};