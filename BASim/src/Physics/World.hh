/**
 * \file World.hh
 *
 * \author miklos@cs.columbia.edu
 * \date 09/09/2009
 */

#ifndef WORLD_HH
#define WORLD_HH

#ifdef USING_INTEL_COMPILER

//#include <tbb/tbb.h>
//using namespace tbb;
// Disable warning about using a local type to declare a function. It's a c++98 error and 
// fine for the lambda usage we use which is from c++0x.
//#pragma warning(disable: 588)

#include <omp.h>

#endif

namespace BASim {

/** Class that collects all of the objects in the world. */
class World : public ObjectBase
{
public:

  typedef std::vector<ObjectBase*> Objects;
  typedef std::vector<ObjectControllerBase*> Controllers;

  World() {}
  ~World() {}

  void initialize(int argc, char** argv)
  {
    PetscUtils::initializePetsc(&argc, &argv);
  }

  void finalize()
  {
    PetscUtils::finalizePetsc();
  }

  ObjectHandle addObject(ObjectBase* object)
  {
    int idx = m_objects.size();
    m_objects.push_back(object);

    return ObjectHandle(idx);
  }

  ObjectBase& getObject(const ObjectHandle& oh)
  {
    assert(oh.isValid());
    assert(oh.idx() >= 0);
    assert((size_t) oh.idx() < m_objects.size());

    return *m_objects[oh.idx()];
  }

  const Objects& getObjects() const
  {
    return m_objects;
  }

  Objects& getObjects()
  {
    return m_objects;
  }

  ObjectControllerHandle addController(ObjectControllerBase* controller)
  {
    int idx = m_controllers.size();
    m_controllers.push_back(controller);

    return ObjectControllerHandle(idx);
  }

  ObjectControllerBase& getController(const ObjectControllerHandle& och)
  {
    assert(och.isValid());
    assert(och.idx() >= 0);
    assert(och.idx() < (int) m_controllers.size());

    return *m_controllers[och.idx()];
  }

  Controllers& getControllers()
  {
    return m_controllers;
  }

  void execute()
  {
    Controllers::iterator it;
    for (it = m_controllers.begin(); it != m_controllers.end(); ++it) {
      (*it)->execute();
    }
  }

#ifdef USING_INTEL_COMPILER
  void executeInParallel( int i_numberOfThreads )
  {
    size_t numControllers = m_controllers.size();
    
    #pragma omp parallel for num_threads( i_numberOfThreads )
    for ( int i=0; i<numControllers; ++i )
    {
      int threadID = omp_get_thread_num();
      /* only master node print the number of threads */
      if ( threadID == 0 )
      {
        int nthreads = omp_get_num_threads();
        printf("World::executeInParallel, using %d threads\n", nthreads );
      }

      m_controllers[ i ]->execute();
    }
  }
#endif
  
protected:

  Objects m_objects;
  Controllers m_controllers;
};

} // namespace BASim

#endif // WORLD_HH
