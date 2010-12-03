/**
 * \file RodRodExternalForce.hh
 *
 * \author smith@cs.columbia.edu
 * \date 06/02/2010
 */

#ifndef RODRODEXTERNALFORCE_HH
#define RODRODEXTERNALFORCE_HH


#ifdef WETA
#include "../../Core/Definitions.hh"
#include "../../Math/MatrixBase.hh"
#include "ElasticRod.hh"
#else
#include "BASim/src/Core/Definitions.hh"
#include "BASim/src/Math/MatrixBase.hh"
#include "BASim/src/Physics/ElasticRods/ElasticRod.hh"
#endif


namespace BASim
{

/** Base class for forces between two rods. */
class RodRodExternalForce
{
public:

  explicit RodRodExternalForce( bool implicit = true )
  : m_implicit(implicit), m_active(true)
  {}

  virtual ~RodRodExternalForce() {}

  virtual void computeForce( VecXd& force ) = 0;
  virtual void computeForceDX( Scalar scale, MatrixBase& J ) = 0;
  virtual void computeForceDV( Scalar scale, MatrixBase& J ) = 0;

  virtual void checkActivatingCondition() {}
  
  bool isImplicit() const { return m_implicit; }
  void setImplicit(bool implicit) { m_implicit = implicit; }

protected:

  bool m_implicit;
  
  bool m_active;
  
};

} // namespace BASim

#endif // RODEXTERNALFORCE_HH
