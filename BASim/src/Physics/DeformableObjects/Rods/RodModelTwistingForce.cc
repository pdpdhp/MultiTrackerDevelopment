#include "RodModelTwistingForce.hh"
#include "BASim/src/Math/MatrixBase.hh"

using namespace BASim;

RodModelTwistingForce::RodModelTwistingForce(ElasticRodModel & rod, Scalar shear_modulus, Scalar shear_modulus_damping, Scalar timestep) :
  RodModelForce(rod, "RodModelStretchingForce"),
  m_shear_modulus(shear_modulus),
  m_shear_modulus_damping(shear_modulus_damping),
  m_timestep(timestep)
{
  
}

RodModelTwistingForce::~RodModelTwistingForce()
{
  
}

Scalar RodModelTwistingForce::globalEnergy()
{
  Scalar energy = 0;
  for (size_t i = 0; i < m_stencils.size(); i++)
  {
    // energy only include non-viscous forces
    energy += localEnergy(m_stencils[i], false);
  }
  return energy;
}

void RodModelTwistingForce::globalForce(VecXd & force)
{
  ElementForce localforce;
  for (size_t i = 0; i < m_stencils.size(); i++)
  {
    // non-viscous force
    localForce(localforce, m_stencils[i], false);
    for (size_t j = 0; j < m_stencils[i].dofindices.size(); j++)
      force(m_stencils[i].dofindices[j]) += localforce(j);
    
    // viscous force
    localForce(localforce, m_stencils[i], true);
    for (size_t j = 0; j < m_stencils[i].dofindices.size(); j++)
      force(m_stencils[i].dofindices[j]) += localforce(j) / m_timestep;
  }
}

void RodModelTwistingForce::globalJacobian(Scalar scale, MatrixBase & Jacobian)
{
  ElementJacobian localjacobian;
  for (size_t i = 0; i < m_stencils.size(); i++)
  {
    // non-viscous force
    localJacobian(localjacobian, m_stencils[i], false);
    Jacobian.add(m_stencils[i].dofindices, m_stencils[i].dofindices, scale * localjacobian);
    
    // viscous force
    localJacobian(localjacobian, m_stencils[i], true);
    Jacobian.add(m_stencils[i].dofindices, m_stencils[i].dofindices, scale / m_timestep * localjacobian);
  }
}

Scalar RodModelTwistingForce::localEnergy(Stencil & s, bool viscous)
{
  
}

void RodModelTwistingForce::localForce(ElementForce & f, Stencil & s, bool viscous)
{
  
}

void RodModelTwistingForce::localJacobian(ElementJacobian & f, Stencil & s, bool viscous)
{
  
}
