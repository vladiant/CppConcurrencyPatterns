// threadSafeInterface.cpp

#include <iostream>
#include <mutex>
#include <thread>

std::mutex mut;

class Critical{

   Critical() {
       std::cout << "Critical this: " << (void*) this << std::endl;
   }
public:
    void interface1() {
        std::lock_guard<std::mutex> lockGuard(mut);
        implementation1();
    }
  
    void interface2(){
        std::lock_guard<std::mutex> lockGuard(mut);
        implementation2();
        implementation3();
        implementation1();
    }
   
private: 
    void implementation1() const {
        std::cout << "  implementation1: " 
                  << std::this_thread::get_id() << '\n';
    }
    void implementation2() const {
        std::cout << "    implementation2: " 
                  << std::this_thread::get_id() << '\n';
        std::this_thread::sleep_for(10ms);

    }
    void implementation3() const{    
        std::cout << "        implementation3: " 
                  << std::this_thread::get_id() << '\n';
    }
};

int main(){
    
    std::cout << '\n';
    
    std::thread t1([]{ 
        Critical crit;
        crit.interface1();
    });
    
    std::thread t2([]{
        Critical crit;
        crit.interface2();
        crit.interface1();
    });
    
    Critical crit;
    crit.interface1();
    crit.interface2();
    
    t1.join();
    t2.join();    
    
    std::cout << '\n';
    
}

