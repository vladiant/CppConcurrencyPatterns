#include <iostream>

#include "event_loop.hpp"
#include "thread_safe_account.hpp"
#include "thread_unsafe_account.hpp"

int main()
{
	auto eventLoop = std::make_shared<EventLoop>();
	auto bankAccount = std::make_shared<ThreadUnsafeAccount>(100'000);

	std::thread buy = std::thread([](std::unique_ptr<IBankAccount> account)
	{
		for (int i = 1; i <= 10; ++i)
		{
			account->pay(i);
		}
	}, std::make_unique<ThreadSafeAccount>(eventLoop, bankAccount));

	std::thread sell = std::thread([](std::unique_ptr<IBankAccount> account)
	{
		for (int i = 1; i <= 10; ++i)
		{
			account->acquire(i);
		}
	}, std::make_unique<ThreadSafeAccount>(eventLoop, bankAccount));

	buy.join();
	sell.join();
	
	std::cout << bankAccount->balance() << '\n';
}
