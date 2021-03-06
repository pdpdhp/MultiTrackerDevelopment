/*
 * MultithreadedStepper.hh
 *
 *  Created on: 31/03/2011
 *      Author: Jean-Marie Aubry <jaubry@wetafx.co.nz>
 */

#ifndef MULTITHREADEDSTEPPER_HH_
#define MULTITHREADEDSTEPPER_HH_

#include "Mutex.hh"
#include <pthread.h>

/**
 * Class to handle parallel execution of a container's element. StepperContainerT must be an stl-style container
 * with a size(), a  const_iterator and a value_type which is a pointer to a class having an execute() method.
 * This execute() method is expected to be thread-safe and return a bool (true = success).
 * NB: It is not guaranteed that the container's elements will be executed in order.
 */
template<typename StepperContainerT>
class MultithreadedStepper
{
    typedef typename StepperContainerT::size_type SizeType;
    typedef typename StepperContainerT::const_iterator IteratorType;
    typedef typename StepperContainerT::value_type StepperType;
    typedef typename BASim::threads::Mutex MutexType;

    class ThreadArgs
    {
        IteratorType& m_it;
        const IteratorType& m_end;
        MutexType& m_it_mutex;
        bool m_success;

    public:
        ThreadArgs(IteratorType& it, const IteratorType& end, MutexType& it_mutex) :
            m_it(it), m_end(end), m_it_mutex(it_mutex), m_success(true)
        {
        }

        friend class MultithreadedStepper;
    };

    IteratorType m_it;
    const IteratorType m_end;
    MutexType m_it_mutex;
    SizeType m_num_threads;
    std::vector<ThreadArgs*> m_thread_args;
    bool m_success;

public:
    MultithreadedStepper(const StepperContainerT& stepper_container, SizeType num_threads) :
        m_it(stepper_container.begin()), m_end(stepper_container.end()), m_it_mutex(),
                m_num_threads(std::min(num_threads, stepper_container.size())), m_thread_args(m_num_threads), m_success(true)
    {
        //        std::cerr << "Number of threads = " << m_num_threads << std::endl;
        for (unsigned int i = 0; i < m_num_threads; i++)
            m_thread_args[i] = new ThreadArgs(m_it, m_end, m_it_mutex);
    }

    bool Execute()
    {
        std::vector < pthread_t > threads(m_num_threads);

        for (unsigned int t = 0; t < m_num_threads; t++)
            pthread_create(&threads[t], NULL, thread_step, static_cast<void*> (m_thread_args[t]));

        for (unsigned int t = 0; t < m_num_threads; t++)
        {
            pthread_join(threads[t], NULL);
            m_success = m_success && m_thread_args[t]->m_success;
            delete m_thread_args[t];
        }
        return m_success;
    }

private:
    static void* thread_step(void* vthread_args)
    {
        ThreadArgs* thread_args = static_cast<ThreadArgs*> (vthread_args);

        thread_args->m_it_mutex.Lock();
        while (thread_args->m_it != thread_args->m_end)
        {
            StepperType stepper = *thread_args->m_it++;
            thread_args->m_it_mutex.Unlock();
            thread_args->m_success = thread_args->m_success && stepper->execute();
            thread_args->m_it_mutex.Lock();
        }
        thread_args->m_it_mutex.Unlock();

        return NULL;
    }

};

#endif /* MULTITHREADEDSTEPPER_HH_ */
