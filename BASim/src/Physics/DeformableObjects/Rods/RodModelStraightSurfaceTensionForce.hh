/**
 * \file RodModelStraightSurfaceTensionForce.hh
 *
 * \author batty@cs.columbia.edu
 * \date Sept 18, 2012
 */

#ifndef RODMODELSTRAIGHTSURFACETENSIONFORCE_HH
#define RODMODELSTRAIGHTSURFACETENSIONFORCE_HH

#include "BASim/src/Physics/DeformableObjects/Rods/RodModelForce.hh"

namespace BASim 
{
  class RodModelStraightSurfaceTensionForce : public RodModelForce
  {
  public:
    typedef Eigen::Matrix<Scalar, 6, 1> ElementForce;
    typedef Eigen::Matrix<Scalar, 6, 6> ElementJacobian;

    struct Stencil : public ElasticRodModel::EdgeStencil
    {
      Stencil(const ElasticRodModel::EdgeStencil & s) : ElasticRodModel::EdgeStencil(s) { }
      
    };

  public:
    RodModelStraightSurfaceTensionForce  (ElasticRodModel & rod, const std::vector<ElasticRodModel::EdgeStencil> & stencils, Scalar surface_tension_coeff);
    virtual ~RodModelStraightSurfaceTensionForce  ();

  public:
    void addStencil(Stencil & s) { m_stencils.push_back(s); }
    std::vector<Stencil> & stencils() { return m_stencils; }
    const std::vector<Stencil> & stencils() const { return m_stencils; }
    
  public:
   
    Scalar globalEnergy();
    void globalForce(VecXd & force);
    void globalJacobian(Scalar scale, MatrixBase & Jacobian);

  protected:
    Scalar localEnergy(Stencil & s);
    void localForce(ElementForce & force, Stencil & s);
    void localJacobian(ElementJacobian & jacobian, Stencil & s);
    
  protected:
    std::vector<Stencil> m_stencils;
    
    Scalar m_surface_tension_coeff;
  };
  
}


#endif // RODMODELSTRAIGHTSURFACETENSIONFORCE_HH