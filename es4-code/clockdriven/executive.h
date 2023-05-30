#ifndef EXECUTIVE_H
#define EXECUTIVE_H

#include <vector>
#include <functional>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <thread>

#include "statistics.h"

class Executive
{
	public:
		/* [INIT] Inizializza l'executive, impostando i parametri di scheduling:
			num_tasks: numero totale di task presenti nello schedule;
			frame_length: lunghezza del frame (in quanti temporali);
			unit_duration: durata dell'unita di tempo, in millisecondi (default 10ms).
		*/
		Executive(size_t num_tasks, unsigned int frame_length, unsigned int unit_duration = 10);

		/* [INIT] Imposta il task periodico di indice "task_id" (da invocare durante la creazione dello schedule):
			task_id: indice progressivo del task, nel range [0, num_tasks);
			periodic_task: funzione da eseguire al rilascio del task;
			wcet: tempo di esecuzione di caso peggiore (in quanti temporali).
		*/
		void set_periodic_task(size_t task_id, std::function<void()> periodic_task, unsigned int wcet);
		
		/* [INIT] Lista di task da eseguire in un dato frame (da invocare durante la creazione dello schedule):
			frame: lista degli id corrispondenti ai task da eseguire nel frame, in sequenza
		*/
		void add_frame(std::vector<size_t> frame);
		
		
		/* [STAT] Imposta la funzione da eseguire quanto è pronta una nuova statistica per un task */
		void set_stats_observer(std::function<void(const task_stats &)> obs);
		
		/* [STAT] Ritorna la statistica di funzionamento globale dell'applicazione */
		global_stats get_global_stats();
		

		/* [RUN] Lancia l'applicazione */
		void start();

		/* [RUN] Attende (all'infinito) finchè gira l'applicazione */
		void wait();

	private:
		enum task_state
		{
			PENDING,
			IDLE,
			RUNNING,
			MISS
		};
		//inizializzare gli stati pending hydle running e miss
		//ogni task è una struct
		// aggiungo gli stati in cui mi posso trovare
			struct task_data
		{
			std::function<void()> function;
			std::condition_variable cond;
			std::mutex mutex;
			std::thread thread;
			task_state stato;
			unsigned int wcet;
			//std::chrono::steady_clock deadline;
			// running che sta eseguendo
			// numero di task che devo eseguire in sequenza, l exec agisce solo allo scadere del frame raffaeee mokk a kiteviiiii

			/* ... */
		};
		
		std::vector<task_data> p_tasks;
		
		std::thread exec_thread;

		std::mutex mutex;

		std::vector< std::vector<size_t> > frames;
		
		const unsigned int frame_length; // lunghezza del frame (in quanti temporali)
		const std::chrono::milliseconds unit_time; // durata dell'unita di tempo (quanto temporale)
		
		/* ... */
		
		static void task_function(task_data & task);
		
		void exec_function();
		
		// statistiche ......
		//uso quello che è nel monitor simple
		std::function<void(const task_stats &)> stats_observer;
		std::thread stats_thread;
		
		void stats_function();

		/* ... */
};

#endif
