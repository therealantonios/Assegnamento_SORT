#include "executive.h"
#include <iostream>

#include "busy_wait.h"

void task0()
{
	std::cout << "Sono il task n.0" << std::endl;
	busy_wait(90);
	std::cout << "Sono il task n.00 FINE" << std::endl;
}

void task1()
{
	std::cout << "Sono il task n.1" << std::endl;
	busy_wait(185);
	std::cout << "Sono il task n.1 FINE" << std::endl;
}
void task2()
{
	std::cout << "Sono il task n.2" << std::endl;
	busy_wait(88);
	std::cout << "Sono il task n.2 FINE" << std::endl;
}

void task3()
{
	std::cout << "Sono il task n.3" << std::endl;
	busy_wait(450);
	std::cout << "Sono il task n.3 FINE" << std::endl;
}

void task4()
{
	std::cout << "Sono il task n.4" << std::endl;
	busy_wait(80);
	std::cout << "Sono il task n.4 FINE" << std::endl;
}

int main()
{
	Executive exec(5, 4, 100);

	exec.set_periodic_task(0, task0, 1); // tau_1
	exec.set_periodic_task(1, task1, 2); // tau_2
	exec.set_periodic_task(2, task2, 1); // tau_3,1
	exec.set_periodic_task(3, task3, 3); // tau_3,2
	exec.set_periodic_task(4, task4, 1); // tau_3,3
	
	exec.add_frame({0,1,2});
	exec.add_frame({0,3});
	exec.add_frame({0,1});
	exec.add_frame({0,1});
	exec.add_frame({0,1,4});
	
	exec.start();
	exec.wait();
	
	return 0;
}
