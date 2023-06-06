#include <cassert>
#include <iostream>
#define VERBOSE
#include "executive.h"

#include "rt/priority.h"
#include "rt/affinity.h"


/// @param num_tasks 
/// @param frame_length 
/// @param unit_duration 

Executive::Executive(size_t num_tasks, unsigned int frame_length, unsigned int unit_duration)
	: p_tasks(num_tasks), frame_length(frame_length), unit_time(unit_duration)
{
}

void Executive::set_periodic_task(size_t task_id, std::function<void()> periodic_task, unsigned int wcet) //wcet è il t di esec del task task_id
{
	assert(task_id < p_tasks.size()); // Fallisce in caso di task_id non corretto (fuori range)
	p_tasks[task_id].stato = IDLE;
	p_tasks[task_id].function = periodic_task;
	p_tasks[task_id].wcet = wcet;
}
		
void Executive::add_frame(std::vector<size_t> frame)
{
	for (auto & id: frame)
		assert(id < p_tasks.size()); // Fallisce in caso di task_id non corretto (fuori range)
	
	frames.push_back(frame);

}


void Executive::start()

{
	statistiche.cycle_count = 0;
	statistiche.exec_count = 0;
	statistiche.miss_count = 0;
	statistiche.canc_count = 0;

	for (size_t id = 0; id < p_tasks.size(); ++id)
	{
		assert(p_tasks[id].function); // Fallisce se set_periodic_task() non e' stato invocato per questo id
		
		//creo un thread per ogni task, il thread è attivo da questo momento. 

		p_tasks[id].thread = std::thread(&Executive::task_function, std::ref(p_tasks[id]), std::ref(nexec));

		rt::affinity aff("1");

		rt::set_affinity(p_tasks[id].thread, aff); //setto affinità per ogni task

		p_tasks[id].stats.task_id = id;
		p_tasks[id].stats.cycle_id = 0;
		p_tasks[id].stats.exec_count = 0;
		p_tasks[id].stats.miss_count = 0;
		p_tasks[id].stats.canc_count = 0;
		p_tasks[id].stats.avg_exec_time = 0;
		p_tasks[id].stats.max_exec_time = 0;
	}
	//master thread dell executive
	exec_thread = std::thread(&Executive::exec_function, this); //thread  monitor che deve avere priorità piu alta di tutti
	rt::set_priority(exec_thread, rt::priority::rt_max);
	rt::set_affinity(exec_thread, 1);

	if (stats_observer)//se esiste e quindi è definito, allora istanzio un thread stats_thread
		stats_thread = std::thread(&Executive::stats_function, this);
}
	
void Executive::wait()
{
	if (stats_thread.joinable())
		stats_thread.join();

	exec_thread.join();
	
	for (auto & pt: p_tasks)
		pt.thread.join();
}
//1 funzione attivata contemporanamente alle altre due 
void Executive::task_function(Executive::task_data & task, unsigned int & nexec ) //funzione che decide se eseguire il task o no 
{
	while (true) 
	{
		
		std::unique_lock<std::mutex> lock(task.mutex);

		while (task.stato != PENDING)
		{
			//si mette in coda, devo aspettare
			task.cond.wait(lock);
		}

		task.stato = task_state ::RUNNING;
		//libero dalla sezione critica
		lock.unlock();
		auto start_time = std::chrono::high_resolution_clock::now();
		task.function(); 
		auto end_time = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double, std:: milli> elapsed (end_time - start_time) ;
		if (task.stato == RUNNING){
			task.stats.avg_exec_time = (task.stats.avg_exec_time*task.stats.exec_count+elapsed.count())/++task.stats.exec_count;
			if(task.stats.max_exec_time == 0)
				task.stats.max_exec_time = elapsed.count();
			else if(elapsed.count() > task.stats.max_exec_time)
				task.stats.max_exec_time = elapsed.count();
		}

		//ogni volta che devo agire sullo stato ho bisogno di bloccare la sezione critica
		lock.lock();
		task.stato = task_state ::IDLE;

	}
}

//primo programma che viene eseguito
void Executive::exec_function()
{
	rt::priority varprio = (rt::priority::rt_max-1);

	auto point = std:: chrono::steady_clock::now();
	
	size_t frame_id = 0; // variabile da scorrere per capire quale frame sta lavorando
	while (true)
	{
		point+=std::chrono::milliseconds(frame_length * unit_time);

	#ifdef VERBOSE
		std::cout << "*** Frame n." << frame_id << (frame_id == 0 ? " ******" : "") << std::endl;
	#endif
		/*  Rilascio dei task periodici del frame corrente ...*/
		for (auto id:frames[frame_id])
		{
			std::unique_lock<std::mutex> lock(p_tasks[id].mutex);
			p_tasks[id].stato = PENDING;
			rt::set_priority(p_tasks[id].thread, --varprio); // ogni volta che setta la priorità decrementa la priorità dei vari task di 1
			p_tasks[id].cond.notify_one(); // invia la notifica però può continuare ad eseguire
		}
	
		/* Attesa fino al prossimo inizio frame ... */
	//l' exec si mette a dormire xkè non ha niente da fare :)
		std :: this_thread :: sleep_until(point);

		/* Controllo delle deadline ... */
		for (auto id : frames[frame_id])
		{
			std::unique_lock<std::mutex> lock(p_tasks[id].mutex);
			if (p_tasks[id].stato == RUNNING)
			{
				p_tasks[id].stato = MISS;
				rt::set_priority(p_tasks[id].thread, rt :: priority::rt_min);
				p_tasks[id].stats.miss_count +=1;
				
				std::cerr << "MISS NUMERO :  " << nmiss << std::endl;
				p_tasks[id].cond.notify_one();		
			}

			else if (p_tasks[id].stato == PENDING)
			{
				p_tasks[id].stats.canc_count +=1;
			}

			else if (p_tasks[id].stato == IDLE)
			{
				p_tasks[id].stats.exec_count +=1;
				p_tasks[id].stats.avg_exec_time = (p_tasks[id].stats.avg_exec_time )* p_tasks[id].stats.exec_count/ p_tasks[id].stats.exec_count ;
			}
		}
//fine H
		if (++frame_id == frames.size())
		{
			statistiche.cycle_count += 1;
//.size mi dice il numero di task 
			for (size_t id = 0; id < p_tasks.size(); ++id)
			{
				p_tasks[id].stats.cycle_id +=1;
				statistiche.miss_count += p_tasks[id].stats.miss_count;
				statistiche.exec_count+= p_tasks[id].stats.exec_count;
				statistiche.canc_count += p_tasks[id].stats.canc_count;
				std::unique_lock<std::mutex> lock(mutex1);
				buffer.push_back(p_tasks[id].stats);
				
				if (!buffer.empty())
				{
					cond.notify_all();
				}
				p_tasks[id].stats.canc_count = 0;
				p_tasks[id].stats.exec_count= 0;
				p_tasks[id].stats.miss_count= 0;
				p_tasks[id].stats.avg_exec_time=0;
				p_tasks[id].stats.max_exec_time=0;
			}

			frame_id = 0;
		}
	}
}

void Executive::set_stats_observer(std::function<void(task_stats const &)> obs)
{
	stats_observer = obs; //GIA' IMPLEMENTATA
}

global_stats Executive::get_global_stats()
{
	std::unique_lock<std::mutex> lock(mutex1);
	return statistiche;
}

void Executive::stats_function()
{
	while (true)
	{
	task_stats val;
			{
				std::unique_lock<std::mutex> lock(mutex1);
				while( buffer.empty())
					cond.wait(lock);
				val = buffer.front();//prendo il primo
				buffer.pop_front();
			}	
			if (stats_observer)
				stats_observer(val);
	}
}