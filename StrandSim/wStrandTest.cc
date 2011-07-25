/*
 * wStrandTest.cc
 *
 *  Created on: 18/07/2011
 *      Author: jaubry
 */

#include "wStrandTest.hh"
#include "ElasticStrand.hh"
#include "ElasticStrandStaticStepper.hh"
#include "../BASim/src/Physics/ElasticRods/ElasticRod.hh"
#include "../BASim/src/Physics/ElasticRods/RodUtils.hh"
#include "../BASim/src/Physics/ElasticRods/GroomingTimeStepper.hh"
#include "../BASim/src/Physics/ElasticRods/RodGravity.hh"

using namespace strandsim;

static const int nVertices = 3;
static const int nDOFs = 4 * nVertices - 1;
static const Scalar totalLength = nVertices - 1.0;
static const Scalar radius = 1.0;
static const Scalar YoungsModulus = 10000.0;
static const Scalar shearModulus = 100.0;
static const Scalar density = 0.1;
static const int nIterations = 10;

void testStrandSim( const std::vector<Vec3d>& i_vertices )
{
    ElasticStrandParameters params( radius, YoungsModulus, shearModulus, density );
    VecXd dofs( nDOFs );
    for ( int i = 0; i < dofs.size(); i += 4 )
        dofs.segment<3> ( i ) = i_vertices[i / 4];
    ElasticStrand strand( dofs, params );

    ElasticStrandStaticStepper stepper;

    for ( int i = 0; i < nIterations; ++i )
    {
     //        std::cout << "\nStrandSim Iteration number " << i << '\n';

        stepper.execute( strand );

        // std::cout << "Press ENTER to continue...\n";
        // std::cin.ignore( std::numeric_limits<std::streamsize>::max(), '\n' );
    }
    std::cout << "Vertices: " << strand << '\n';

    std::cout << std::endl;
}

void testBASim( const std::vector<Vec3d>& i_vertices )
{
    using namespace BASim;

    RodOptions rodOptions;
    rodOptions.YoungsModulus = YoungsModulus; /* megapascal */
    rodOptions.ShearModulus = shearModulus; /* megapascal */
    rodOptions.viscosity = 0; /* poise */
    rodOptions.density = density; /* grams per cubic centimeter */
    rodOptions.radiusA = radius; /* millimeter */
    rodOptions.radiusB = radius; /* millimeter */
    rodOptions.refFrame = BASim::ElasticRod::TimeParallel;
    rodOptions.numVertices = nVertices;

    // Use the rod helper function to build the rod
    ElasticRod* rod = setupRod( rodOptions, i_vertices, i_vertices,
            findNormal<3> ( i_vertices[1] - i_vertices[0] ) );

    // Create a timeStepper to simulate the rod forward in time
    GroomingTimeStepper* stepper = new GroomingTimeStepper( *rod );
    stepper->setDiffEqSolver( BASim::GroomingTimeStepper::STATICS );

    Vec3d i_gravity( 0.0, 0.0, -981.0 );
    stepper->addExternalForce( new RodGravity( i_gravity ) );

    // Set the rod's fixed vertices
    RodBoundaryCondition* boundary = stepper->getBoundaryCondition();
    boundary->setDesiredVertexPosition( 0, rod->getVertex( 0 ) );
    boundary->setDesiredVertexPosition( 1, rod->getVertex( 1 ) );
    boundary->setDesiredEdgeAngle( 0, rod->getTheta( 0 ) );

    for ( int i = 0; i < nIterations; ++i )
    {
      //    std::cout << "\nBASim iteration number " << i <<  '\n';

        rod->setIsInRestState( false );
        stepper->execute();

        //  std::cout << "Press ENTER to continue...\n";
        //  std::cin.ignore( std::numeric_limits<std::streamsize>::max(), '\n' );
    }
    std::cout << "Vertices: ";
    std::cout << '{';
    for ( int i = 0; i < rod->nv() - 1; i++ )
    {
        const Vec3d& vertex = rod->getVertex( i );
        std::cout << '{' << vertex[0] << ", " << vertex[1] << ", " << vertex[2] << "}, ";
    }
    const Vec3d& vertex = rod->getVertex( rod->nv() - 1 );
    std::cout << '{' << vertex[0] << ", " << vertex[1] << ", " << vertex[2] << '}';
    std::cout << '}';
    std::cout << '\n';

}

int main()
{
    g_log = new TextLog( std::cerr, MsgInfo::kDebug, true );

    static const int NF = Length<BuiltInForcesList>::value;

    std::cout << "Number of built-in forces = " << NF << '\n';

    std::vector<Vec3d> i_vertices;
    for ( int i = 0; i < nVertices; i++ )
        i_vertices.push_back(
                Vec3d( i * totalLength / ( nVertices - 1 ), sin( 2.0 * i * M_PI / ( nVertices ) ),
                        0.0 ) );

    std::cout << "This is StrandSim\n";
    testStrandSim( i_vertices );

    std::cout << "This is BASim\n";
    testBASim( i_vertices );

    return 0;
}
