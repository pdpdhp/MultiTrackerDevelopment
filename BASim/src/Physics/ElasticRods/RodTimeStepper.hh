/**
 * \file RodTimeStepper.hh
 *
 * \author miklos@cs.columbia.edu
 * \date 09/03/2009
 */

#ifndef RODTIMESTEPPER_HH
#define RODTIMESTEPPER_HH

//#include <BASim/src/Core/ObjectControllerBase.hh>
#include <BASim/Core>

namespace BASim {

/** Class to time step a rod. */
class RodTimeStepper : public ObjectControllerBase
{
public:

  enum Method { SYMPL_EULER, IMPL_EULER, NONE };

  RodTimeStepper(ElasticRod& rod)
    : m_rod(rod)
    , m_method(NONE)
    , m_diffEqSolver(NULL)
    , m_boundaryCondition(NULL)
  {
    setDiffEqSolver(SYMPL_EULER);
  }

  ~RodTimeStepper()
  {
    if (m_diffEqSolver != NULL) delete m_diffEqSolver;
    if (m_boundaryCondition != NULL) delete m_boundaryCondition;
  }

  void execute()
  {
    if ( m_enabled )
      m_diffEqSolver->execute();
  }

  void setTime(Scalar time)
  {
    m_diffEqSolver->setTime(time);
  }

  Scalar getTime() const
  {
    return m_diffEqSolver->getTime();
  }

  void setTimeStep(Scalar dt)
  {
    m_diffEqSolver->setTimeStep(dt);
  }

  Scalar getTimeStep() const
  {
    return m_diffEqSolver->getTimeStep();
  }

  const DiffEqSolver& getDiffEqSolver() const
  {
    assert(m_diffEqSolver != NULL);

    return *m_diffEqSolver;
  }

  void setDiffEqSolver(Method method)
  {
    if (method == m_method) return;

    m_method = method;
    if (m_diffEqSolver != NULL) delete m_diffEqSolver;
    m_diffEqSolver = NULL;

    if (method == SYMPL_EULER) {
      m_diffEqSolver = new SymplecticEuler<RodTimeStepper>(*this);

    } else if (method == IMPL_EULER) {
      m_diffEqSolver = new ImplicitEuler<RodTimeStepper>(*this);

    } else if (method == NONE) {
      m_diffEqSolver = NULL;

    } else {
      std::cout << "Unknown method specified" << std::endl;
      m_diffEqSolver = NULL;

    }
  }

  int ndof() const
  {
    return m_rod.ndof();
  }

  /**
   * This function computes the force on each degree of freedom
   * associated to the rod.
   *
   * \param[out] f The vector of accelerations on the rod.
   */
  void evaluatePDot(VecXd& f)
  {
    m_forces = f;
    // add internal forces
    m_rod.computeForces(f);

    if (m_rod.viscous()) f /= m_diffEqSolver->getTimeStep();

    // add external forces
    for (size_t i = 0; i < m_externalForces.size(); ++i) {
      m_externalForces[i]->computeForce(m_rod, f);
    }
  
    m_forces = f - m_forces;
  }

  /**
   * Evaluates the Jacobian of the forces on the rod.
   *
   * \param[out] J The Jacobian of the forces on the rod.
   */
  void evaluatePDotDX(MatrixBase& J)
  {
    m_rod.computeJacobian(J);

    if (m_rod.viscous()) {
      J.finalize();
      J.scale(1.0 / m_diffEqSolver->getTimeStep());
    }

    for (size_t i = 0; i < m_externalForces.size(); ++i) {
      m_externalForces[i]->computeForceDX(m_rod, J);
    }
  }

  void evaluatePDotDV(MatrixBase& J)
  {
    for (size_t i = 0; i < m_externalForces.size(); ++i) {
      m_externalForces[i]->computeForceDV(m_rod, J);
    }
  }

  /**
   * Returns an array with the indices of the fixed degrees of freedom.
   */
  const IntArray& getFixedDofs()
  {
    return m_rod.fixed();
  }

  /**
   * This function returns the mass associated with a degree of
   * freedom of the rod.
   *
   * \param[in] i Which degree of freedom.
   * \return The mass of the degree of freedom
   */
  Scalar getMass(int i)
  {
    assert(i >= 0);
    assert(i < m_rod.ndof());

    return m_rod.getMass(i);
  }

  Scalar getX(int i) const
  {
    return m_rod.getDof(i);
  }

  void setX(int i, Scalar x)
  {
    m_rod.setDof(i, x);
  }

  Scalar getV(int i)
  {
    return m_rod.getVel(i);
  }

  void setV(int i, Scalar v)
  {
    m_rod.setVel(i, v);
  }

  /**
   * Adds an external force to be applied to the rod. On destruction,
   * this class will be responsible for de-allocating the memory
   * associated to the force.
   *
   * \param[in] force The external force to be applied to rods.
   */
  void addExternalForce(RodExternalForce* force)
  {
    m_externalForces.push_back(force);
  }

  std::vector<RodExternalForce*>& getExternalForces()
  {
    return m_externalForces;
  }

  void flush()
  {
    m_rod.updateProperties();
  }

  int getMaxIterations() const
  {
    return m_diffEqSolver->getMaxIterations();
  }

  void setMaxIterations(int iterations)
  {
    m_diffEqSolver->setMaxIterations(iterations);
  }

  MatrixBase* createMatrix() const
  {
    SolverUtils* s = SolverUtils::instance();
    return s->createBandMatrix(m_rod.ndof(), m_rod.ndof(), 10, 10);
  }

  RodBoundaryCondition* getBoundaryCondition()
  {
    if (m_boundaryCondition == NULL) {
      m_boundaryCondition = new RodBoundaryCondition(m_rod);
    }
    return m_boundaryCondition;
  }

  void setBoundaryCondition(RodBoundaryCondition* bc)
  {
    m_boundaryCondition = bc;
  }

  void getScriptedDofs(IntArray& indices, std::vector<Scalar>& desired)
  {
    if (m_boundaryCondition == NULL) {
      indices.resize(0);
      desired.resize(0);
      return;
    }

    const RodBoundaryCondition::BCList& verts
      = m_boundaryCondition->scriptedVertices();
    const RodBoundaryCondition::BCList& edges
      = m_boundaryCondition->scriptedEdges();

    int nb = 3 * verts.size() + edges.size(); // # of scripted dofs
    indices.resize(nb);
    desired.resize(nb);

    for (size_t i = 0; i < verts.size(); ++i) {
      for (int k = 0; k < 3; ++k) {
        indices[3 * i + k] = m_rod.vertIdx(verts[i], k);
        desired[3 * i + k]
          = m_boundaryCondition->getDesiredVertexPosition(verts[i])[k];
      }
    }

    for (size_t i = 0; i < edges.size(); ++i) {
      indices[3 * verts.size() + i] = m_rod.edgeIdx(edges[i]);
      desired[3 * verts.size() + i]
        = m_boundaryCondition->getDesiredEdgeAngle(edges[i]);
    }
  }

  VecXd& getForcesAtLastStep()
  {
    return m_forces;
  }

protected:

  ElasticRod& m_rod;
  std::vector<RodExternalForce*> m_externalForces;
  Method m_method;
  DiffEqSolver* m_diffEqSolver;
  RodBoundaryCondition* m_boundaryCondition;

  // Copy of the forces on the rod, used in Beaker 
  // to check if the sim is going to explode.
  VecXd m_forces;
};

} // namespace BASim

#endif // RODTIMESTEPPER_HH
