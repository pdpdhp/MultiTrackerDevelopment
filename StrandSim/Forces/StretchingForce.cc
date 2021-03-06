/*
 * StretchingForce.cc
 *
 *  Created on: 12/07/2011
 *      Author: Jean-Marie Aubry <jaubry@wetafx.co.nz>
 */

#include "StretchingForce.hh"

namespace strandsim
{

StretchingForce::StretchingForce()
{
    // TODO Auto-generated constructor stub

}

StretchingForce::~StretchingForce()
{
    // TODO Auto-generated destructor stub
}

Scalar StretchingForce::localEnergy( const ElasticStrand& strand, const StrandGeometry& geometry,
        const IndexType vtx )
{
    const Scalar ks = strand.m_parameters.m_ks;
    const Scalar restLength = strand.m_restLengths[vtx];
    const Scalar length = geometry.m_lengths[vtx];

    return 0.5 * ks * square( length / restLength - 1.0 ) * restLength;
}

void StretchingForce::computeLocalForce( StretchingForce::LocalForceType& localF,
        const ElasticStrand& strand, const StrandGeometry& geometry, const IndexType vtx )
{
    const Scalar ks = strand.m_parameters.m_ks;// std::cout << "ks = " << ks << '\n';
    const Scalar restLength = strand.m_restLengths[vtx];// std::cout << "restLength = " << restLength << '\n';
    const Scalar length = geometry.m_lengths[vtx];// std::cout << "length = " << length << '\n';

    Vec3d f = ks * ( length / restLength - 1.0 ) * geometry.getEdgeVector( vtx ).normalized();
    localF.segment<3> ( 0 ) = f;
    localF.segment<3> ( 3 ) = -f;

    // std::cout << "Local stretching force (vertex " << vtx << "): " << force << '\n';
}

void StretchingForce::computeLocalJacobian( StretchingForce::LocalJacobianType& localJ,
        const ElasticStrand& strand, const StrandGeometry& geometry, const IndexType vtx )
{
    const Scalar ks = strand.m_parameters.m_ks;// std::cout << "ks = " << ks << '\n';
    const Scalar restLength = strand.m_restLengths[vtx];// std::cout << "restLength = " << restLength << '\n';
    const Scalar length = geometry.m_lengths[vtx];// std::cout << "length = " << length << '\n';
    const Vec3d& edge = geometry.getEdgeVector( vtx );
    const Mat3d M = ks * ( ( 1.0 / restLength - 1.0 / length ) * Mat3d::Identity() + 1.0 / length
            * edge * edge.transpose() / square( length ) );

    localJ.block<3, 3> ( 0, 0 ) = localJ.block<3, 3> ( 3, 3 ) = -M;
    localJ.block<3, 3> ( 0, 3 ) = localJ.block<3, 3> ( 3, 0 ) = M;
    //  assert( isSymmetric( Jacobian ) ); STRANGE FIXME

    // std::cout << "Local stretching Jacobian (vertex " << vtx << "): " << Jacobian << '\n';
}

void StretchingForce::addInPosition( ForceVectorType& globalForce, const IndexType vtx,
        const LocalForceType& localForce )
{
    globalForce.segment<3> ( 4 * vtx ) += localForce.segment<3> ( 0 );
    globalForce.segment<3> ( 4 * ( vtx + 1 ) ) += localForce.segment<3> ( 3 );
}

void StretchingForce::addInPosition( JacobianMatrixType& globalJacobian, const IndexType vtx,
        const LocalJacobianType& localJacobian )
{
    globalJacobian.edgeStencilAdd<6> ( 4 * vtx, localJacobian );
}

}
