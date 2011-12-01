/**
 * \file BentTwisting.hh
 *
 * \author batty@cs.columbia.edu
 * \date April 20, 2011
 */

#ifndef SHELLTEST_HH
#define SHELLTEST_HH

#include "ProblemBase.hh"

#include "BASim/src/Physics/DeformableObjects/DeformableObject.hh"
#include "BASim/src/Physics/DeformableObjects/Shells/ElasticShell.hh"
#include "BASim/src/Physics/DeformableObjects/DefoObjTimeStepper.hh"
/**
 * Test problem for the shells...
 */

class ShellTest : public Problem
{
public:

  ShellTest();
  virtual ~ShellTest();

  //virtual void serialize( std::ofstream& of );
  //virtual void resumeFromfile( std::ifstream& ifs );

protected:

  void Setup();
  void AtEachTimestep();

  DeformableObject* shellObj;
  ElasticShell* shell;
  DefoObjTimeStepper* stepper;
  
  void setupScene1(); //vertical flat sheet
  void setupScene2(); //vertical cylindrical sheet
  void setupScene3(); //spherical sheet
  void setupScene4(); //two-triangle bending test


};

#endif // SHELLTEST_HH