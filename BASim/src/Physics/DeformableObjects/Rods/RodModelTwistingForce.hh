/**
 * \file RodModelTwistingForce.h
 *
 * \author fang@cs.columbia.edu
 * \date May 12, 2011
 */

#ifndef RODMODELTWISTINGFORCE_HH
#define RODMODELTWISTINGFORCE_HH

#include "BASim/src/Physics/DeformableObjects/Rods/RodModelForce.hh"

namespace BASim 
{
  class RodModelTwistingForce : public RodModelForce
  {
  public:
    typedef Eigen::Matrix<Scalar, 11, 1> ElementForce;
    typedef Eigen::Matrix<Scalar, 11, 11> ElementJacobian;
    
    struct Stencil
    {
      VertexHandle v;
      EdgeHandle e1;
      EdgeHandle e2;
      IntArray dofindices;
   };
    
  public:
    RodModelTwistingForce(ElasticRodModel & rod, Scalar shear_modulus, Scalar shear_modulus_damping, Scalar timestep);
    virtual ~RodModelTwistingForce();
    
  public:
    void addStencil(Stencil & s) { m_stencils.push_back(s); }
    std::vector<Stencil> & stencils() { return m_stencils; }
    const std::vector<Stencil> & stencils() const { return m_stencils; }
    
  public:
    Scalar globalEnergy();
    void globalForce(VecXd & force);
    void globalJacobian(Scalar scale, MatrixBase & Jacobian);
    
  protected:
    Scalar localEnergy(Stencil & s, bool viscous);
    void localForce(ElementForce & f, Stencil & s, bool viscous);
    void localJacobian(ElementJacobian & f, Stencil & s, bool viscous);
    
  protected:
    std::vector<Stencil> m_stencils;
    
    Scalar m_shear_modulus;
    Scalar m_shear_modulus_damping;
    
    Scalar m_timestep;
    
  };
  
}


#endif // RODMODELTWISTINGFORCE_HH
