/**
 * \file ShellSolidVertexJointCouplingForce.h
 *
 * \author fang@cs.columbia.edu
 * \date 10/01/2012
 */

#ifndef SHELLSOLIDVERTEXJOINTCOUPLINGFORCE_HH
#define SHELLSOLIDVERTEXJOINTCOUPLINGFORCE_HH

#include "BASim/src/Physics/DeformableObjects/DefoObjForce.hh"
#include "BASim/src/Physics/DeformableObjects/DeformableObject.hh"
#include "BASim/src/Physics/DeformableObjects/Rods/ElasticRodModel.hh"
#include "BASim/src/Physics/DeformableObjects/Shells/ElasticShell.hh"
#include "BASim/src/Physics/DeformableObjects/Solids/ElasticSolid.hh"

#include "BASim/src/Math/ADT/adreal.h"
#include "BASim/src/Math/ADT/advec.h"
#include "BASim/src/Math/ADT/mat3t.h"

namespace BASim 
{
  class ShellSolidVertexJointCouplingForce : public DefoObjForce
  {
  public:
    const static int NumDof = 18;

    typedef Eigen::Matrix<Scalar, NumDof, 1>      ElementForce;
    typedef Eigen::Matrix<Scalar, NumDof, NumDof> ElementJacobian;

    typedef CVec3T<Scalar> Vector3d;
    
    struct Stencil
    {
      Stencil() { }
      
      IntArray dofindices;

      // stencil coverage
      FaceHandle f; // shell face AEF
      TetHandle t;  // solid tet ABCD
      
//      // cached stiffness
//      Scalar stiffness;
//      Scalar viscous_stiffness;
      
      // reference strain
      Vec3d undeformed_AP;
      Scalar undeformed_delta;
      Vec3d damping_undeformed_AP;
      Scalar damping_undeformed_delta;
      
      // cached properties
      Vec3d AP;      // solid tet axis AP in shell face's frame (P = (B+C+D)/3)
      Scalar delta;  // solid tet's rotation about AP
    };

  public:
    ShellSolidVertexJointCouplingForce(ElasticShell & shell, ElasticSolid & solid, const std::vector<Stencil> & stencils, Scalar stiffness, Scalar stiffness_damp, Scalar timestep);
    virtual ~ShellSolidVertexJointCouplingForce();

  public:
    void addStencil(Stencil & s) { m_stencils.push_back(s); }
    std::vector<Stencil> & stencils() { return m_stencils; }
    const std::vector<Stencil> & stencils() const { return m_stencils; }
    
  public:
    void updateStiffness();
    void updateViscousReferenceStrain();
    void updateProperties();
    
  public:
    void startStep(Scalar time, Scalar timestep);
    void endStep(Scalar time, Scalar timestep);
    void startIteration(Scalar time, Scalar timestep);
    void endIteration(Scalar time, Scalar timestep);
    
  public:
    Scalar globalEnergy();
    void globalForce(VecXd & force);
    void globalJacobian(Scalar scale, MatrixBase & Jacobian);

  public:
    ElasticShell & shell() { assert(m_shell); return *m_shell; }
    ElasticSolid & solid() { assert(m_solid); return *m_solid; }
    
    DeformableObject & defoObj() { assert(m_shell); return m_shell->getDefoObj(); }
    
  protected:
    template <int DO_HESS>
    adreal<NumDof, DO_HESS, Scalar> adEnergy(const ShellSolidVertexJointCouplingForce & mn, const Vec3d & A, const Vec3d & B, const Vec3d & C, const Vec3d & D, const Vec3d & E, const Vec3d & F, Scalar delta, const Vec3d & undeformed_AP, Scalar undeformed_delta, Scalar stiffness);

  protected:
    Scalar localEnergy(Stencil & s, bool viscous);
    void localForce(ElementForce & force, Stencil & s, bool viscous);
    void localJacobian(ElementJacobian & jacobian, Stencil & s, bool viscous);

    void computeReferenceStrain();
    
    Vector3d vec2vector(const Vec3d & v);
    std::vector<VertexHandle> getVertices(const Stencil & s);
    
  protected:
    ElasticShell * m_shell;
    ElasticSolid * m_solid;
    
    std::vector<Stencil> m_stencils;
    
    Scalar m_stiffness;
    Scalar m_stiffness_damp;
  };
  
}


#endif // SHELLSOLIDVERTEXJOINTCOUPLINGFORCE_HH