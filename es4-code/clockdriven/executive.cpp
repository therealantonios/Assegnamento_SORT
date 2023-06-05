#include <cassert>
#include <iostream>
//CIAO TONI
//CIAO KEKKA
#define VERBOSE

#include "executive.h"

#include "rt/priority.h"
#include "rt/affinity.h"


Executive::Executive(size_t num_tasks, unsigned int frame_length, unsigned int unit_duration)
	: p_tasks(num_tasks), frame_length(frame_length), unit_time(unit_duration)
{
}

//contatori per statistiche globbali

void Executive::set_periodic_task(size_t task_id, std::function<void()> periodic_task, unsigned int wcet) //wcet è il t di esec del task task_id
{
	assert(task_id < p_tasks.size()); // Fallisce in caso di task_id non corretto (fuori range)
	p_tasks[task_id].stato = IDLE;
	p_tasks[task_id].function = periodic_task;
	p_tasks[task_id].wcet = wcet;
	//p_tasks[task_id].deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(wcet);
	//p_tasks[task_id].deadline = //faccio partire un chrono ;
}
		
void Executive::add_frame(std::vector<size_t> frame)
{
	for (auto & id: frame)
		assert(id < p_tasks.size()); // Fallisce in caso di task_id non corretto (fuori range)
	
	frames.push_back(frame);

	/* ... */ //ma dove cazzo gli avete presi a questi 2 ?????????????????
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
		
		p_tasks[id].thread = std::thread(&Executive::task_function, std::ref(p_tasks[id]), std::ref(nexec));

		rt::affinity aff("1");

		rt::set_affinity(p_tasks[id].thread, aff); //setto affinità per ogni task
		//rt::set_priority(p_tasks[id].thread, rt::priority::rt_max -1); // setto priorità per ogni task

		p_tasks[id].stats.task_id = id;
		p_tasks[id].stats.cycle_id = 0;
		p_tasks[id].stats.exec_count = 0;
		p_tasks[id].stats.miss_count = 0;
		p_tasks[id].stats.canc_count = 0;
		p_tasks[id].stats.avg_exec_time = 0;
		p_tasks[id].stats.max_exec_time = 0;

		
		

	}
	
	exec_thread = std::thread(&Executive::exec_function, this); //thread  monitor che deve avere priorità piu alta di tutti
	rt::set_priority(exec_thread, rt::priority::rt_max);
	rt::set_affinity(exec_thread, 1);


	if (stats_observer)
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

void Executive::task_function(Executive::task_data & task, unsigned int & nexec ) //funzione che decide se eseguire il task o no 
{
	while (true) 
	{
		//metto fuori per proteggere
		std::unique_lock<std::mutex> lock(task.mutex);

		while (task.stato != PENDING)
		{
			task.cond.wait(lock);
		}

		task.stato = task_state ::RUNNING;
		//libero dalla sezione critica
		lock.unlock();
		auto start_time = std::chrono::high_resolution_clock::now();
		task.function(); //sleep
		auto end_time = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double, std:: milli> elapsed (end_time - start_time) ;
		task.stats.avg_exec_time += elapsed.count();

		//_tasks[id].stats.miss_count +=1;

		//std::cerr << "task eseguito NUMERO :  " << nexec << std::endl;
		//ogni volta che devo agire sullo stato ho bisogno di bloccare la sezione critica
		lock.lock();
		task.stato = task_state ::IDLE;

		if(task.stats.max_exec_time == 0)
			task.stats.max_exec_time = elapsed.count();
		else if(elapsed.count() > task.stats.max_exec_time)
			task.stats.max_exec_time = elapsed.count();

		//task.cond.notify_one();//invia la notifica xò può continuare ad eseguire
		//lock.unlock();//per sicurezza la metto, anche se alla fine lo fa in automatico
		//task_function SEMPRE FUORI DALLA REGIONE CRITICA!!! Se fosse in regione cirtica mi serializzarebbe 
	}
	// for (auto id: task.frame)
	// NON PUO' ACCEDERE ALLA VAR DELLA CLASSE, DEVO PASSARLE COME PARMA
	// mi attiva il task
	// ci serve x quando rilasciamo i frame.

	// quando inizia il task, devo attivare il thread, loack etc
	// ciclionwhilen
	/* ... */
}

void Executive::exec_function()
{
	rt::priority varprio = (rt::priority::rt_max-1);
	
	//chrono che mi serve per la sleep
	auto point = std:: chrono::steady_clock::now();
	
	size_t frame_id = 0; // variabile da scorrere per capire quale frame sta lavorando
	// std::unique_lock<std::mutex> lock(mutex);
	// std::function<void(const task_stats &)> stats_observer;
	// std::thread stats_thread;
	//  dischiaro subito il muetx e ho dichiarato anche il .h
	//  mi dice che frame sta lavorando
	//  attualmente sul primo
	//  dichiaro un mutex
	//  rilascio i frame e
	/* ... */
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
			p_tasks[id].cond.notify_one(); // invia la notifica xò può continuare ad eseguire
			//UNLOCK SEMPRE ALLA FINE , ALTRIEMNTI MORTE CERTA
			//lock.unlock();
		}
	
		/* Attesa fino al prossimo inizio frame ... */

		std :: this_thread :: sleep_until(point);
		//std::cout << point << std::endl;
		//auto next = std::chrono::high_resolution_clock::now();
/* 		std::chrono::duration<double,std::milli>elapsed(next-lastpeppe);
		std::cout << "tempo trascorso: " << elapsed.count() << std::endl;
		lastpeppe = next; */

		/* Controllo delle deadline ... */
		for (auto id : frames[frame_id])
		{
			std::unique_lock<std::mutex> lock(p_tasks[id].mutex);
			if (p_tasks[id].stato == RUNNING)
			{
				p_tasks[id].stato = MISS;
				rt::set_priority(p_tasks[id].thread, rt :: priority::rt_min);
				//std::cerr << "--------------------------------------------ho fatto una deadline miss " << id << std::endl;
				//nmiss +=1;
				p_tasks[id].stats.miss_count +=1;
				
				std::cerr << "MISS NUMERO :  " << nmiss << std::endl;
				p_tasks[id].cond.notify_one();
				//"scorrere la lista dei task di quel frame e aggiungere in un for i vari numeri di task ancora da eseguire			
			}
			else if (p_tasks[id].stato == PENDING)
			{
				//ncanc+=1;
				p_tasks[id].stats.canc_count +=1;
				//std::cerr << "task cancellato NUMERO :  " << ncanc << std::endl;
			}
			else if (p_tasks[id].stato == IDLE)
			{
				p_tasks[id].stats.exec_count +=1;
				
				
				p_tasks[id].stats.avg_exec_time = p_tasks[id].stats.avg_exec_time / p_tasks[id].stats.exec_count ;		//nexec+=1;
				//p_tasks[id].stats.max_exec_time = p_tasks[id].stats;

			}



		}

		if (++frame_id == frames.size()) //forse per le stats singole 
		{
			statistiche.cycle_count += 1;

			for (size_t id = 0; id < p_tasks.size(); ++id)
			{
				p_tasks[id].stats.cycle_id +=1;
				
				

				statistiche.miss_count += p_tasks[id].stats.miss_count;
				statistiche.exec_count+= p_tasks[id].stats.exec_count;
				statistiche.canc_count += p_tasks[id].stats.canc_count;

				
				//ncycle +=1;
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
	
				/* p_stats[id].exec_count = 0;
				p_stats[id].miss_count = 0;
				p_stats[id].canc_count = 0;
				p_stats[id].avg_exec_time = 0;
				p_stats[id].max_exec_time = 0;  */

				

			}
			
			frame_id = 0;

			//std::cerr << "ciclo numero:  " << ncycle << std::endl;
			
			// qui devo aggiornare le statistiche
			//	stat_oserver (...);



			// mettere subito questa funzione, è deleterio
			// in questa porzione preparo le statistiche
			// una volta preparato il dato, realizzo uno scambio dati, x avere che la statistica sia accodata
			////produco una nuova statstica e la metto in coda meccanismo di monitori visto
			/* Update Statistiche, accodamento e notifica... */
		}

	}
}

void Executive::set_stats_observer(std::function<void(task_stats const &)> obs)
{
	stats_observer = obs; //GIA' IMPLEMENTATA
}

global_stats Executive::get_global_stats()
{
	
	return statistiche;
}

void Executive::stats_function()
{
	while (true)
	{

	task_stats val;
			
			{
				std::unique_lock<std::mutex> lock(mutex1);	// mutex acquired here

				while( buffer.empty())
					cond.wait(lock);

												// mutex released here

				val = buffer.front();
				buffer.pop_front();
			}	
			if (stats_observer)
				stats_observer(val);
		// reaalizzo applicazione, quando la coda non è vuota la consuma chiama la funzione observer
		// e consuma. comunica solo tramite a coda
		/* Consumo statistiche di task accodate (invocando stats_observer) ...*/
	}
}