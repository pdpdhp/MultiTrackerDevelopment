/**
 * \file BentTwisting.cc
 *
 * \author miklos@cs.columbia.edu
 * \date 09/09/2009
 */

#include "BentTwisting.hh"

BentTwisting::BentTwisting()
  : Problem("Bent Twisting", "A bent anisotropic rod is twisted")
  , rod(NULL)
  , stepper(NULL)
  , m_maxTwist(10)
  , m_twistRate(1)
  , m_currentTwist(0)
{
  addDynamicsProps();
  addRodOptions();
  addRodTimeStepperOptions();

  AddOption("shape-radius", "radius of circular arc defining shape of rod",
            20.0);
  AddOption("twist-rate", "rate of twisting of end of rod", m_twistRate);
  AddOption("twist-max", "maximum amount of twist", m_maxTwist);

  // default to no gravity
  GetVecOpt("gravity") = Vec3d::Zero();
}

BentTwisting::~BentTwisting()
{
  if (rod != NULL) delete rod;
  if (stepper != NULL) delete stepper;
}

void BentTwisting::Setup()
{
  loadDynamicsProps();

  RodOptions opts;
  getRodOptions(opts);

  m_twistRate = GetScalarOpt("twist-rate");
  m_maxTwist = GetScalarOpt("twist-max");

  Scalar radius = GetScalarOpt("shape-radius");
  std::vector<Vec3d> vertices, undeformed;
  for (int i = 0; i < opts.numVertices; ++i) {
    vertices.push_back(Vec3d(radius * cos(i * M_PI / (opts.numVertices - 1)),
                             radius * sin(i * M_PI / (opts.numVertices - 1)),
                             0));
  }

  rod = setupRod(opts, vertices, vertices);
  int nv = rod->nv();
  int ne = rod->ne();

  stepper = getRodTimeStepper(*rod);

  RodBoundaryCondition* boundary = stepper->getBoundaryCondition();
  boundary->setDesiredVertexPosition(0, rod->getVertex(0));
  boundary->setDesiredVertexPosition(1, rod->getVertex(1));
  boundary->setDesiredEdgeAngle(0, rod->getTheta(0));
  boundary->setDesiredVertexPosition(nv - 2, rod->getVertex(nv - 2));
  boundary->setDesiredVertexPosition(nv - 1, rod->getVertex(nv - 1));
  boundary->setDesiredEdgeAngle(ne - 1, rod->getTheta(ne - 1));

  m_world->addObject(rod);
  m_world->addController(stepper);
}

void BentTwisting::AtEachTimestep()
{
  // Test of object serializaiton
  //RodState origrodstate;
  //origrodstate.copyState(*rod);
  //rodstate.print(*rod);
  
  //TopologicalObjectSerializer testserializer;
  //testserializer.saveTopologicalObject( *rod, "testoutput.bin" );

  //ElasticRod* testrod = NULL;
  //testserializer.loadTopologicalObject( &testrod, "testoutput.bin" );
  //assert( testrod != NULL );

  //origrodstate.compareProperties(*testrod);
  //delete[] testrod;
  
  if (m_maxTwist > 0 && m_currentTwist >= m_maxTwist) return;

  Scalar twistIncrement = getDt() * m_twistRate;
  int edge = rod->ne() - 1;
  Scalar theta = rod->getTheta(edge) - twistIncrement;
  stepper->getBoundaryCondition()->setDesiredEdgeAngle(edge, theta);
  m_currentTwist += twistIncrement;
}