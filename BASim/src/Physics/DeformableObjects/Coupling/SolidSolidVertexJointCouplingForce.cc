#include "SolidSolidVertexJointCouplingForce.hh"
#include "BASim/src/Math/MatrixBase.hh"
#include "BASim/src/Math/Math.hh"

using namespace BASim;

SolidSolidVertexJointCouplingForce::SolidSolidVertexJointCouplingForce(ElasticSolid & solid, const std::vector<Stencil> & stencils, Scalar stiffness, Scalar stiffness_damp, Scalar timestep) :
  DefoObjForce(solid.getDefoObj(), timestep, "SolidSolidVertexJointCouplingForce"),
  m_solid(&solid),
  m_stencils(),
  m_stiffness(stiffness),
  m_stiffness_damp(stiffness_damp)
{
  for (size_t i = 0; i < stencils.size(); i++)
  {
    Stencil s(stencils[i]);
//    s.stiffness = 0;
//    s.viscous_stiffness = 0;
    s.undeformed_AB.setZero();
    s.undeformed_AC.setZero();
    s.undeformed_AD.setZero();
    s.damping_undeformed_AB.setZero();
    s.damping_undeformed_AC.setZero();
    s.damping_undeformed_AD.setZero();
    
    std::vector<VertexHandle> vh = getVertices(s);
    s.dofindices.resize(NumDof);
    int dofbase = defoObj().getPositionDofBase(vh[0]);
    s.dofindices[0] = dofbase;
    s.dofindices[1] = dofbase + 1;
    s.dofindices[2] = dofbase + 2;
    dofbase = defoObj().getPositionDofBase(vh[1]);
    s.dofindices[3] = dofbase;
    s.dofindices[4] = dofbase + 1;
    s.dofindices[5] = dofbase + 2;
    dofbase = defoObj().getPositionDofBase(vh[2]);
    s.dofindices[6] = dofbase;
    s.dofindices[7] = dofbase + 1;
    s.dofindices[8] = dofbase + 2;
    dofbase = defoObj().getPositionDofBase(vh[3]);
    s.dofindices[9] = dofbase;
    s.dofindices[10] = dofbase + 1;
    s.dofindices[11] = dofbase + 2;
    dofbase = defoObj().getPositionDofBase(vh[4]);
    s.dofindices[12] = dofbase;
    s.dofindices[13] = dofbase + 1;
    s.dofindices[14] = dofbase + 2;
    dofbase = defoObj().getPositionDofBase(vh[5]);
    s.dofindices[15] = dofbase;
    s.dofindices[16] = dofbase + 1;
    s.dofindices[17] = dofbase + 2;
    dofbase = defoObj().getPositionDofBase(vh[6]);
    s.dofindices[18] = dofbase;
    s.dofindices[19] = dofbase + 1;
    s.dofindices[20] = dofbase + 2;
        
    s.AB.setZero();  // will be computed by updateProperties() below
    s.AC.setZero();
    s.AD.setZero();
    
    m_stencils.push_back(s);
  }

  updateProperties();
  updateStiffness();
  updateViscousReferenceStrain();
  computeReferenceStrain();
}

SolidSolidVertexJointCouplingForce::~SolidSolidVertexJointCouplingForce()
{
  
}

std::vector<VertexHandle> SolidSolidVertexJointCouplingForce::getVertices(const Stencil & s)
{
  std::vector<VertexHandle> vh(7);
  TetVertexIterator tvit = defoObj().tv_iter(s.t1);
  VertexHandle vt11 = *tvit; ++tvit; assert(tvit);
  VertexHandle vt12 = *tvit; ++tvit; assert(tvit);
  VertexHandle vt13 = *tvit; ++tvit; assert(tvit);
  VertexHandle vt14 = *tvit; ++tvit; assert(!tvit);
  TetVertexIterator tvit2 = defoObj().tv_iter(s.t2);
  VertexHandle vt21 = *tvit2; ++tvit2; assert(tvit2);
  VertexHandle vt22 = *tvit2; ++tvit2; assert(tvit2);
  VertexHandle vt23 = *tvit2; ++tvit2; assert(tvit2);
  VertexHandle vt24 = *tvit2; ++tvit2; assert(!tvit2);
  if (vt11 == vt21 || vt11 == vt22 || vt11 == vt23 || vt11 == vt24) vh[0] = vt11, vh[4] = vt12, vh[5] = vt13, vh[6] = vt14;
  if (vt12 == vt21 || vt12 == vt22 || vt12 == vt23 || vt12 == vt24) vh[0] = vt12, vh[4] = vt13, vh[5] = vt14, vh[6] = vt11;
  if (vt13 == vt21 || vt13 == vt22 || vt13 == vt23 || vt13 == vt24) vh[0] = vt13, vh[4] = vt14, vh[5] = vt11, vh[6] = vt12;
  if (vt14 == vt21 || vt14 == vt22 || vt14 == vt23 || vt14 == vt24) vh[0] = vt14, vh[4] = vt11, vh[5] = vt12, vh[6] = vt13;
  if (vh[0] == vt21) vh[1] = vt22, vh[2] = vt23, vh[3] = vt24;
  if (vh[0] == vt22) vh[1] = vt23, vh[2] = vt24, vh[3] = vt21;
  if (vh[0] == vt23) vh[1] = vt24, vh[2] = vt21, vh[3] = vt22;
  if (vh[0] == vt24) vh[1] = vt21, vh[2] = vt22, vh[3] = vt23;
  return vh;
}

SolidSolidVertexJointCouplingForce::Vector3d SolidSolidVertexJointCouplingForce::vec2vector(const Vec3d & input)
{
  Vector3d output;
  output.x() = input.x();
  output.y() = input.y();
  output.z() = input.z();
  return output;
}

template <int DO_HESS>
adreal<SolidSolidVertexJointCouplingForce::NumDof, DO_HESS, Scalar> 
SolidSolidVertexJointCouplingForce::adEnergy(const SolidSolidVertexJointCouplingForce & mn, const Vec3d & A, const Vec3d & B, const Vec3d & C, const Vec3d & D, const Vec3d & E, const Vec3d & F, const Vec3d & G, const Vec3d & undeformed_AB, const Vec3d & undeformed_AC, const Vec3d & undeformed_AD, Scalar stiffness) 
{  
  // typedefs to simplify code below
  typedef adreal<SolidSolidVertexJointCouplingForce::NumDof, DO_HESS, Scalar> adrealElast;
  typedef CVec3T<adrealElast> advecElast;
  Mat3T<adrealElast> temp;
  typedef Mat3T<adrealElast> admatElast;

  Vector3d vA = vec2vector(A);
  Vector3d vB = vec2vector(B);
  Vector3d vC = vec2vector(C);
  Vector3d vD = vec2vector(D);
  Vector3d vE = vec2vector(E);
  Vector3d vF = vec2vector(F);
  Vector3d vG = vec2vector(G);
  Vector3d vUndeformedAB = vec2vector(undeformed_AB);
  Vector3d vUndeformedAC = vec2vector(undeformed_AC);
  Vector3d vUndeformedAD = vec2vector(undeformed_AD);

  advecElast p[7];
  set_independent(p[0], vA, 0);
  set_independent(p[1], vB, 3);
  set_independent(p[2], vC, 6);
  set_independent(p[3], vD, 9);
  set_independent(p[4], vE, 12);
  set_independent(p[5], vF, 15);
  set_independent(p[6], vG, 18);

  adrealElast e(0);
  
  advecElast md1 = (p[4] + p[5] - p[0] * 2.0); md1 /= len(md1);
  advecElast md2 = (p[4] - p[5] - dot(p[4] - p[5], md1) * md1); md2 /= len(md2);
  advecElast md3 = cross(md1, md2);
  
  advecElast vAB = advecElast(dot(p[1] - p[0], md1), dot(p[1] - p[0], md2), dot(p[1] - p[0], md3));
  advecElast vAC = advecElast(dot(p[2] - p[0], md1), dot(p[2] - p[0], md2), dot(p[2] - p[0], md3));
  advecElast vAD = advecElast(dot(p[3] - p[0], md1), dot(p[3] - p[0], md2), dot(p[3] - p[0], md3));

  e = stiffness * (dot(vAB - vUndeformedAB, vAB - vUndeformedAB) + dot(vAC - vUndeformedAC, vAC - vUndeformedAC) + dot(vAD - vUndeformedAD, vAD - vUndeformedAD));

  return e;
}

Scalar SolidSolidVertexJointCouplingForce::globalEnergy()
{
  Scalar energy = 0;
  for (size_t i = 0; i < m_stencils.size(); i++)
  {
    // energy only include non-viscous forces
    energy += localEnergy(m_stencils[i], false);
  }
  return energy;
}

void SolidSolidVertexJointCouplingForce::globalForce(VecXd & force)
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
      force(m_stencils[i].dofindices[j]) += localforce(j) / timeStep();
  }
}

void SolidSolidVertexJointCouplingForce::globalJacobian(Scalar scale, MatrixBase & Jacobian)
{
  ElementJacobian localjacobian;
  for (size_t i = 0; i < m_stencils.size(); i++)
  {
    // non-viscous force
    localJacobian(localjacobian, m_stencils[i], false);
    Jacobian.add(m_stencils[i].dofindices, m_stencils[i].dofindices, scale * localjacobian);
    
    // viscous force
    localJacobian(localjacobian, m_stencils[i], true);
    Jacobian.add(m_stencils[i].dofindices, m_stencils[i].dofindices, scale / timeStep() * localjacobian);
  }
}

Scalar SolidSolidVertexJointCouplingForce::localEnergy(Stencil & s, bool viscous)
{
  std::vector<VertexHandle> vh = getVertices(s);
  Vec3d A = defoObj().getVertexPosition(vh[0]);
  Vec3d B = defoObj().getVertexPosition(vh[1]);
  Vec3d C = defoObj().getVertexPosition(vh[2]);
  Vec3d D = defoObj().getVertexPosition(vh[3]);
  Vec3d E = defoObj().getVertexPosition(vh[4]);
  Vec3d F = defoObj().getVertexPosition(vh[5]);
  Vec3d G = defoObj().getVertexPosition(vh[6]);
  
  adreal<NumDof, 1, Scalar> e = adEnergy<1>(*this, A, B, C, D, E, F, G, (viscous ? s.damping_undeformed_AB : s.undeformed_AB), (viscous ? s.damping_undeformed_AC : s.undeformed_AC), (viscous ? s.damping_undeformed_AD : s.undeformed_AD), (viscous ? m_stiffness_damp : m_stiffness));
  Scalar energy = e.value();

  return energy;
}

void SolidSolidVertexJointCouplingForce::localForce(ElementForce & force, Stencil & s, bool viscous)
{
  std::vector<VertexHandle> vh = getVertices(s);
  Vec3d A = defoObj().getVertexPosition(vh[0]);
  Vec3d B = defoObj().getVertexPosition(vh[1]);
  Vec3d C = defoObj().getVertexPosition(vh[2]);
  Vec3d D = defoObj().getVertexPosition(vh[3]);
  Vec3d E = defoObj().getVertexPosition(vh[4]);
  Vec3d F = defoObj().getVertexPosition(vh[5]);
  Vec3d G = defoObj().getVertexPosition(vh[6]);
  
  adreal<NumDof, 1, Scalar> e = adEnergy<1>(*this, A, B, C, D, E, F, G, (viscous ? s.damping_undeformed_AB : s.undeformed_AB), (viscous ? s.damping_undeformed_AC : s.undeformed_AC), (viscous ? s.damping_undeformed_AD : s.undeformed_AD), (viscous ? m_stiffness_damp : m_stiffness));
  for (int i = 0; i < NumDof; i++)
  {
    force[i] = -e.gradient(i);
  }
}

void SolidSolidVertexJointCouplingForce::localJacobian(ElementJacobian & jacobian, Stencil & s, bool viscous)
{
  std::vector<VertexHandle> vh = getVertices(s);
  Vec3d A = defoObj().getVertexPosition(vh[0]);
  Vec3d B = defoObj().getVertexPosition(vh[1]);
  Vec3d C = defoObj().getVertexPosition(vh[2]);
  Vec3d D = defoObj().getVertexPosition(vh[3]);
  Vec3d E = defoObj().getVertexPosition(vh[4]);
  Vec3d F = defoObj().getVertexPosition(vh[5]);
  Vec3d G = defoObj().getVertexPosition(vh[6]);
  
  adreal<NumDof, 1, Scalar> e = adEnergy<1>(*this, A, B, C, D, E, F, G, (viscous ? s.damping_undeformed_AB : s.undeformed_AB), (viscous ? s.damping_undeformed_AC : s.undeformed_AC), (viscous ? s.damping_undeformed_AD : s.undeformed_AD), (viscous ? m_stiffness_damp : m_stiffness));
  for (int i = 0; i < NumDof; i++)
    for (int j = 0; j < NumDof; j++)
    {
      jacobian(i, j) = -e.hessian(i, j);
    }
}

void SolidSolidVertexJointCouplingForce::updateStiffness()
{

}

void SolidSolidVertexJointCouplingForce::updateViscousReferenceStrain()
{
  for (size_t i = 0; i < m_stencils.size(); i++)
  {
    Stencil & s = m_stencils[i];
    s.damping_undeformed_AB = s.AB;
    s.damping_undeformed_AC = s.AC;
    s.damping_undeformed_AD = s.AD;
  }
}

void SolidSolidVertexJointCouplingForce::updateProperties()
{
  for (size_t i = 0; i < m_stencils.size(); i++)
  {
    Stencil & s = m_stencils[i];
    
    std::vector<VertexHandle> vh = getVertices(s);
    Vec3d A = defoObj().getVertexPosition(vh[0]);
    Vec3d B = defoObj().getVertexPosition(vh[1]);
    Vec3d C = defoObj().getVertexPosition(vh[2]);
    Vec3d D = defoObj().getVertexPosition(vh[3]);
    Vec3d E = defoObj().getVertexPosition(vh[4]);
    Vec3d F = defoObj().getVertexPosition(vh[5]);
    Vec3d G = defoObj().getVertexPosition(vh[6]);
    Vec3d md1 = (E + F - A * 2).normalized();
    Vec3d md2 = (E - F - (E - F).dot(md1) * md1).normalized();
    Vec3d md3 = md1.cross(md2);
    Vec3d AB = B - A;
    Vec3d AC = C - A;
    Vec3d AD = D - A;
    s.AB = Vec3d(AB.dot(md1), AB.dot(md2), AB.dot(md3));
    s.AC = Vec3d(AC.dot(md1), AC.dot(md2), AC.dot(md3));
    s.AD = Vec3d(AD.dot(md1), AD.dot(md2), AD.dot(md3));
  }
}

void SolidSolidVertexJointCouplingForce::computeReferenceStrain()
{
  for (size_t i = 0; i < m_stencils.size(); i++)
  {
    Stencil & s = m_stencils[i];
    s.undeformed_AB = s.AB;
    s.undeformed_AC = s.AC;
    s.undeformed_AD = s.AD;
  }
}

void SolidSolidVertexJointCouplingForce::startStep(Scalar time, Scalar timestep)
{
  updateViscousReferenceStrain();
}

void SolidSolidVertexJointCouplingForce::endStep(Scalar time, Scalar timestep)
{
  updateStiffness();
}

void SolidSolidVertexJointCouplingForce::startIteration(Scalar time, Scalar timestep)
{
  
}

void SolidSolidVertexJointCouplingForce::endIteration(Scalar time, Scalar timestep)
{
  updateProperties();
}
