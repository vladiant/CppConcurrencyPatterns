#pragma once

struct IBankAccount
{
	virtual ~IBankAccount() = default;
	virtual void pay(unsigned amount) noexcept = 0;
	virtual void acquire(unsigned amount) noexcept = 0;
	virtual long long balance() const noexcept = 0;
};