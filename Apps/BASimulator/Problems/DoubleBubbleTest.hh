/**
 * \file DoubleBubbleTest.hh
 *
 * \author fang@cs.columbia.edu
 * \date Nov 20, 2012
 */

#ifndef DOUBLEBUBBLETEST_HH
#define DOUBLEBUBBLETEST_HH

#include "ProblemBase.hh"

#include "BASim/src/Physics/DeformableObjects/DeformableObject.hh"
#include "BASim/src/Physics/DeformableObjects/Shells/ElasticShell.hh"
#include "BASim/src/Physics/DeformableObjects/DefoObjTimeStepper.hh"
#include "BASim/src/Physics/DeformableObjects/Shells/ShellVolumeForce.hh"

class DoubleBubbleTest : public Problem, public ElasticShell::SteppingCallback
{
public:
  DoubleBubbleTest();
  virtual ~DoubleBubbleTest();

  virtual void serialize( std::ofstream& of ) { assert(!"Not implemented"); }
  virtual void resumeFromfile( std::ifstream& ifs ) { assert(!"Not implemented"); }

  void beforeEndStep();
  
protected:
  void Setup();
  void AtEachTimestep();
  void AfterStep();
  
  DeformableObject * shellObj;
  ElasticShell * shell;
  DefoObjTimeStepper * stepper;

  Scalar m_timestep;
//  Scalar m_initial_thickness;
  int m_active_scene;

//  int m_s4_nbubble;
  
  std::vector<VertexHandle> triangulation_added_vertices;
  std::vector<EdgeHandle>   triangulation_added_edges;
  std::vector<FaceHandle>   triangulation_added_faces;
  ShellVolumeForce * svf;
  
  int onBBWall(const Vec3d & pos) const;
  void updateBBWallConstraints();
    
public:
  void setupScene1(); // VIIM test: single film in cube
  void setupScene2(); // T1 transition
//  void setupScene3(); // double bubble collision
//  void setupScene4(); // n bubble collision
  void setupScene5(); // VIIM figure 17
  void setupScene6(); // VIIM multiphase cube test (figure 24)
    
  void setupScene7(); // Enright test with a sphere
  void setupScene8(); // Reauleux tetrahedron test VIIM figure 18
  

  void s7_enright_velocity(double t, const Vec3d & pos, Vec3d & out);

  int m_nregion;

};

#endif // DOUBLEBUBBLETEST_HH

