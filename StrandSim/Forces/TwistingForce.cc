/*
 * TwistingForce.cc
 *
 *  Created on: 12/07/2011
 *      Author: Jean-Marie Aubry <jaubry@wetafx.co.nz>
 */

#include "TwistingForce.hh"

namespace strandsim
{

TwistingForce::TwistingForce()
{
    // TODO Auto-generated constructor stub

}

TwistingForce::~TwistingForce()
{
    // TODO Auto-generated destructor stub
}

Scalar TwistingForce::localEnergy( const ElasticStrand& strand, const StrandGeometry& geometry,
        const IndexType vtx )
{
    const Scalar kt = strand.m_parameters.m_kt;
    const Scalar ilen = strand.m_invVoronoiLengths[vtx];
    const Scalar undefTwist = strand.m_restTwists[vtx];
    const Scalar twist = geometry.m_twists[vtx];

    return 0.5 * kt * square( twist - undefTwist ) * ilen;
}

void TwistingForce::computeLocalForce( TwistingForce::LocalForceType& localF,
        const ElasticStrand& strand, const StrandGeometry& geometry, const IndexType vtx )
{
    const Scalar kt = strand.m_parameters.m_kt;
    const Scalar ilen = strand.m_invVoronoiLengths[vtx];
    const Scalar undefTwist = strand.m_restTwists[vtx];
    const Scalar twist = geometry.m_twists[vtx];// std::cout << "twist = " << twist << '\n';

    localF = -kt * ilen * ( twist - undefTwist ) * geometry.m_gradTwists[vtx];
}

void TwistingForce::computeLocalJacobian( TwistingForce::LocalJacobianType& localJ,
        const ElasticStrand& strand, const StrandGeometry& geometry, const IndexType vtx )
{
    const Scalar kt = strand.m_parameters.m_kt;// std::cout << "kt = " << kt << '\n';
    const Scalar ilen = strand.m_invVoronoiLengths[vtx];// std::cout << "ilen = " << ilen << '\n';
    const Scalar twist = geometry.m_twists[vtx];// std::cout << "twist = " << twist << '\n';
    const Scalar undeformedTwist = strand.m_restTwists[vtx];// std::cout << "undefTwist = " << undeformedTwist << '\n';

    const LocalForceType& gradTwist = geometry.m_gradTwists[vtx];// std::cout << "gradTwist = " << gradTwist << '\n';
    LocalJacobianType hessTwist;
    geometry.computeHessTwist( hessTwist, vtx );

    // std::cout << hessTwist << '\n';
    // std::cout << -kt * ilen * ((twist - undeformedTwist) * hessTwist + gradTwist * gradTwist.transpose()) << '\n';

    localJ = -kt * ilen * ( ( twist - undeformedTwist ) * hessTwist + gradTwist
            * gradTwist.transpose() );
}

void TwistingForce::addInPosition( ForceVectorType& globalForce, const IndexType vtx,
        const LocalForceType& localForce )
{
    globalForce.segment<11> ( 4 * ( vtx - 1 ) ) += localForce;
}

void TwistingForce::addInPosition( JacobianMatrixType& globalJacobian, const IndexType vtx,
        const LocalJacobianType& localJacobian )
{
    globalJacobian.localStencilAdd<11> ( 4 * ( vtx - 1 ), localJacobian );
}

}
