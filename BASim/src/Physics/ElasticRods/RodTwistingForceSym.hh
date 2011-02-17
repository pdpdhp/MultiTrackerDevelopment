/**
 * \file RodTwistingForceSym.hh
 *
 * \author miklos@cs.columbia.edu
 * \date 02/18/2010
 */

#ifndef RODTWISTINGFORCESYM_HH
#define RODTWISTINGFORCESYM_HH

#ifdef WETA
#include "RodForce.hh"
#include "VertexStencil.hh"
#endif

namespace BASim {

class RodTwistingForceSym : public RodForceT<VertexStencil>
{
public:

  explicit RodTwistingForceSym(ElasticRod& rod, bool vscs = false, bool runinit = true);

  virtual Scalar globalEnergy();
  virtual void globalForce(VecXd& F);
  virtual void globalJacobian(int baseidx, Scalar scale, MatrixBase& J);

  virtual void globalReverseJacobian(MatrixBase& Jacobian);
  virtual void updateReverseUndeformedStrain(const VecXd& e);
  
  Scalar localEnergy(const vertex_handle& vh);
  void localForce(VecXd& F, const vertex_handle& vh);
  void localJacobian(MatXd& J, const vertex_handle& vh);

  Scalar getKt(const vertex_handle& vh) const;
  void setKt(const vertex_handle& vh, const Scalar& kt);

  Scalar getTwist(const vertex_handle& vh) const;
  void setTwist(const vertex_handle& vh, const Scalar& twist);

  Scalar getUndeformedTwist(const vertex_handle& vh) const;
  void setUndeformedTwist(const vertex_handle& vh,
                          const Scalar& undeformedTwist);

  Scalar getRefVertexLength(const vertex_handle& vh) const;
  void setRefVertexLength(const vertex_handle& vh, const Scalar& length);

  virtual void updateProperties();
  virtual void updateStiffness();
  virtual void updateUndeformedStrain();
  virtual void updateReferenceDomain();

  virtual void updateUndeformedConfiguration(std::vector<Scalar>& vals);

  virtual void reattatchProperties();

private:

  const VecXd& getGradTwist(const vertex_handle& vh) const;
  const MatXd& getHessTwist(const vertex_handle& vh) const;

  void computeGradTwist();
  void computeHessTwist();

  VPropHandle<Scalar> m_kt;              ///< twist stiffness
  VPropHandle<Scalar> m_twist;           ///< twist at a vertex
  VPropHandle<Scalar> m_undeformedTwist; ///< undeformed twist
  VPropHandle<Scalar> m_refVertexLength; ///< length of domain of integration

  ObjPropHandle<bool> m_gradTwistValid;
  ObjPropHandle<bool> m_hessTwistValid;
  VPropHandle<VecXd> m_gradTwist; ///< gradient of twist
  VPropHandle<MatXd> m_hessTwist; ///< Hessian of twist
};

} // namespace BASim

#endif // RODTWISTINGFORCESYM_HH
