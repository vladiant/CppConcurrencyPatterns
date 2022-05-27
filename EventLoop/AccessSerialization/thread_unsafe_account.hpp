#pragma once

#include "i_bank_account.hpp"

class ThreadUnsafeAccount : public IBankAccount
{
public:
	ThreadUnsafeAccount(long long balance) : m_balance(balance)
	{
	}
	void pay(unsigned amount) noexcept override
	{
		m_balance -= amount;
	}
	void acquire(unsigned amount) noexcept override
	{
		m_balance += amount;
	}
	long long balance() const noexcept override
	{
		return m_balance;
	}
private:
	long long m_balance;
};