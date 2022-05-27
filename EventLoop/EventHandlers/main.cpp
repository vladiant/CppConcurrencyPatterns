#include <iostream>

#include "event_loop.hpp"

std::function<void(std::vector<char>)> OnNetworkEvent;

void emitNetworkEvent(EventLoop& loop, std::vector<char> data)
{
	if (!OnNetworkEvent) return;
	
	loop.enqueue(std::bind(std::ref(OnNetworkEvent), std::move(data)));
}

int main()
{
	//registering event handler
	OnNetworkEvent = [](std::vector<char> message)
	{
		std::cout << message.size() << ' ';
	};
	
	EventLoop loop;
	
	//let's trigger the event from different threads
	std::thread t1 = std::thread([](EventLoop& loop)
	{
		for (std::size_t i = 0; i < 10; ++i)
		{
			emitNetworkEvent(loop, std::vector<char>(i));
		}
	}, std::ref(loop));
	
	std::thread t2 = std::thread([](EventLoop& loop)
	{
		for (int i = 10; i < 20; ++i)
		{
			emitNetworkEvent(loop, std::vector<char>(i));
		}
	}, std::ref(loop));
	
	for (int i = 20; i < 30; ++i)
	{
		emitNetworkEvent(loop, std::vector<char>(i));
	}
	
	t1.join();
	t2.join();
	
	loop.enqueue([]
	{
		std::cout << std::endl;
	});
}
