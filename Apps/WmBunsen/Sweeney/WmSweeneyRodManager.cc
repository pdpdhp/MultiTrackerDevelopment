#include "WmSweeneyRodManager.hh"

using namespace std;
using namespace BASim;

WmSweeneyRodManager::WmSweeneyRodManager()
{
    m_bAGroomingStepper = NULL;
    m_rods.clear();
    m_subsetNodes.clear();
    m_rodsPerSubset.clear();
    m_rodTimeSteppers.clear();
    m_triangleMeshes.clear();
    m_scriptingControllers.clear();
    m_rodRenderers.clear();

    m_drawOnlySolved = false;
    m_drawOnlyUnsolved = false;
}

WmSweeneyRodManager::~WmSweeneyRodManager()
{
    delete m_bAGroomingStepper;
}

void WmSweeneyRodManager::setRodsDrawDebugging( const bool i_shouldDrawStrands,
        const bool i_shouldDrawRootFrames, const bool i_shouldDrawVelocity,
        const bool i_shouldDrawOnlySolved, const bool i_shouldDrawOnlyUnsolved )
{

    m_drawOnlySolved = i_shouldDrawOnlySolved;
    m_drawOnlyUnsolved = i_shouldDrawOnlyUnsolved;

    for ( size_t r = 0; r < m_rodRenderers.size(); ++r )
    {
        m_rodRenderers[r]->drawRod() = i_shouldDrawStrands;
        m_rodRenderers[r]->drawRootMaterial() = i_shouldDrawRootFrames;
        m_rodRenderers[r]->drawVelocity() = i_shouldDrawVelocity;
    }
}

bool WmSweeneyRodManager::addRod( const std::vector<BASim::Vec3d>& i_vertices, const double i_time,
        const BASim::Vec3d& i_referenceDir1, const BASim::Vec3d& i_referenceDir2,
        const double i_youngsModulus, const double i_shearModulus, const double i_viscosity,
        const double i_density, const double i_radiusA, const double i_radiusB,
        const BASim::ElasticRod::RefFrameType i_referenceFrame, const double i_massDamping,
        const BASim::Vec3d i_gravity, const BASim::GroomingTimeStepper::Method i_solverType )
{
    RodOptions rodOptions;
    rodOptions.YoungsModulus = i_youngsModulus; /* megapascal */
    rodOptions.ShearModulus = i_shearModulus; /* megapascal */
    rodOptions.viscosity = i_viscosity; /* poise */
    rodOptions.density = i_density; /* grams per cubic centimeter */
    rodOptions.radiusA = i_radiusA; /* millimeter */
    rodOptions.radiusB = i_radiusB; /* millimeter */
    rodOptions.refFrame = i_referenceFrame;
    rodOptions.numVertices = ( int ) ( i_vertices.size() );

    // Use the rod helper function to build the rod
    ElasticRod* rod = setupRod( rodOptions, i_vertices, i_vertices, i_referenceDir1,
            i_referenceDir2 );

    // We need a rod renderer to draw the rod in OpenGL
    RodRenderer* rodRenderer = new RodRenderer( *rod );

    // Create a timeStepper to simulate the rod forward in time
    GroomingTimeStepper* stepper = new GroomingTimeStepper( *rod );
    stepper->setDiffEqSolver( i_solverType );

    // Add a damping force to the rod
    stepper->addExternalForce( new RodMassDamping( i_massDamping ) );

    // Lock the first two vertices of the rod
    //   RodBoundaryCondition* boundary = stepper->getBoundaryCondition();

    // Set the velocity to be zero as we're grooming static hair
    //  boundary->setDesiredVertexPosition( 0, i_time, rod->getVertex( 0 ), BASim::Vec3d( 0.0, 0.0, 0.0 ) );
    //  boundary->setDesiredVertexPosition( 1, i_time, rod->getVertex( 1 ), BASim::Vec3d( 0.0, 0.0, 0.0 ) );

    // If the magnitude of gravity is 0 then don't bother adding the force
    if ( i_gravity.norm() > 0 )
    {
        stepper->addExternalForce( new RodGravity( i_gravity ) );
    }

    // Add a force class that we will use to pass in forces from Maya
    RodMayaForces* rodMayaForces = new RodMayaForces( rod );
    stepper->addExternalForce( rodMayaForces );

    // Reverse hairdo is still experimental and optional...
    // if ( 0 )
    // {
    //     cerr << "Doing reverse hairdo!\n";
    //     rod->doReverseHairdo(stepper);
    // }

    // Arbitrarily scale the rod up so it can be seen
    rod->setRadiusScale( 10.0 );

    // Store all the things we need to control the rod or add it to a BAGroomingStepper
    m_rods.push_back( rod );
    m_rodTimeSteppers.push_back( stepper );
    m_rodRenderers.push_back( rodRenderer );

    return true;
}

void WmSweeneyRodManager::addCollisionMesh( BASim::TriangleMesh* i_triangleMesh,
        BASim::LevelSet* i_levelSet, WmFigMeshController* i_scriptingController )
{
    cerr << "WmSweeneyRodManager::addCollisionMesh levelSet = " << i_levelSet << "\n";
    assert( i_triangleMesh );
    assert( i_levelSet );
    assert( i_scriptingController );
    m_triangleMeshes.push_back( i_triangleMesh );
    m_levelSets.push_back( i_levelSet );
    m_scriptingControllers.push_back( i_scriptingController );
}

void WmSweeneyRodManager::setUseKineticDamping( bool i_useKinecticDamping )
{
    m_bAGroomingStepper->setUseKineticDamping( i_useKinecticDamping );
}

void WmSweeneyRodManager::initialiseSimulation( const double i_timeStep, const double i_startTime,
        PerformanceTuningParameters perfParams, double i_atol, double i_stol, double i_rtol,
        double i_inftol, int i_numLineSearchIters, int i_numberOfClumps )
{

    std::cout << "Performance Tuning Parameters " << std::endl;
    std::cout << "Penalty Response " << perfParams.m_enable_penalty_response << std::endl;
    std::cout << "Implicit Thickness " << perfParams.m_implicit_thickness << std::endl;
    std::cout << "Implicit Stiffness " << perfParams.m_implicit_stiffness << std::endl;
    std::cout << "Inextensibility Threshold " << perfParams.m_inextensibility_threshold
            << std::endl;
    std::cout << "Max Num Solver Iterations " << perfParams.m_solver.m_max_iterations << std::endl;
    std::cout << "Max Num Collision Iterations  " << perfParams.m_collision.m_max_iterations
            << std::endl;
    std::cout << "Explosion Detection  " << perfParams.m_enable_explosion_detection << std::endl;
    std::cout << "Explosion Dampening " << perfParams.m_explosion_damping << std::endl;
    std::cout << "Explosion Threshold " << perfParams.m_explosion_threshold << std::endl;
    std::cout << "Solver failure " << perfParams.m_solver.m_in_case_of << std::endl;
    std::cout << "Max Number of Solver Substeps " << perfParams.m_solver.m_max_substeps
            << std::endl;
    std::cout << "Collison Failure  " << perfParams.m_collision.m_in_case_of << std::endl;
    std::cout << "Max Number of Collision Substeps " << perfParams.m_collision.m_max_substeps
            << std::endl;
    std::cout << "Explosion  Failure  " << perfParams.m_explosion.m_in_case_of << std::endl;
    std::cout << "Max Number of Explosion Substeps " << perfParams.m_explosion.m_in_case_of
            << std::endl;
    std::cout << "Atol  " << i_atol << std::endl;
    std::cout << "Stol  " << i_stol << std::endl;
    std::cout << "Rtol  " << i_rtol << std::endl;
    std::cout << "Inftol  " << i_inftol << std::endl;
    std::cout << "Num of Line Search Iterations " << i_numLineSearchIters << std::endl;

    /*  PerformanceTuningParameters perfParams;

     // Definition of solver error
     perfParams.m_maximum_number_of_solver_iterations = 5;

     // Action on solver error
     perfParams.m_in_case_of_solver_failure = PerformanceTuningParameters::IgnoreError;
     perfParams.m_max_number_of_substeps_for_solver = 0;

     // Definition of collision response: no collision response
     perfParams.m_maximum_number_of_collisions_iterations = 0;

     // Action on collision failure: ignore collision failures
     perfParams.m_in_case_of_collision_failure = PerformanceTuningParameters::IgnoreError;
     perfParams.m_max_number_of_substeps_for_collision = 0;

     // Definition of explosion
     perfParams.m_explosion_threshold = .5;

     // Action on explosion: substep up to level 7
     perfParams.m_in_case_of_explosion_failure = PerformanceTuningParameters::IgnoreError;
     perfParams.m_max_number_of_substeps_for_explosion = 7;

     // Always run inextensibility
     perfParams.m_inextensibility_threshold = 0;

     // A very weak penalty force
     perfParams.m_implicit_thickness        = 0.1;
     perfParams.m_implicit_stiffness        = 1.0;
     */

    m_bAGroomingStepper = new BAGroomingStepper( m_rods, m_triangleMeshes, m_scriptingControllers,
            m_rodTimeSteppers, i_timeStep, i_startTime, -1, perfParams, m_levelSets,
            i_numberOfClumps, m_rodsPerSubset );
}

void WmSweeneyRodManager::updateSolverSettings( double i_atol, double i_stol, double i_rtol,
        double i_inftol, int i_numLineSearchIters, double i_penaltyStiffness )
{
    //  std::cout << "Updating solver settings:" << std::endl;
    //  std::cout << "Atol  " << i_atol << std::endl;
    //  std::cout << "Stol  " << i_stol << std::endl;
    //  std::cout << "Rtol  " << i_rtol << std::endl;
    //  std::cout << "Inftol  " << i_inftol << std::endl;
    //  std::cout << "Num of Line Search Iterations " << i_numLineSearchIters << std::endl;

    for ( size_t r = 0; r < m_rods.size(); ++r )
    {
        GroomingTimeStepper* stepper = m_rodTimeSteppers[r];
        assert( stepper );
        DiffEqSolver& solver = stepper->getDiffEqSolver();

        solver.set_stol( i_stol );
        solver.set_atol( i_atol );
        solver.set_rtol( i_rtol );
        solver.set_inftol( i_inftol );
        solver.set_maxlsit( i_numLineSearchIters );
    }

    assert( m_bAGroomingStepper );
    m_bAGroomingStepper->setPenaltyStiffness( i_penaltyStiffness );
}

void WmSweeneyRodManager::setClumpingParameters( const double charge, const double power,
        const double dist, const std::vector<double> vertexPowerMap ) const
{
    m_bAGroomingStepper->setClumpingParameters( charge, power, dist, vertexPowerMap );
}

void WmSweeneyRodManager::getClumpingParameters( double& charge, double& power, double& dist,
        std::vector<double>& vertexPowerMap ) const
{
    m_bAGroomingStepper->getClumpingParameters( charge, power, dist, vertexPowerMap );
}

void WmSweeneyRodManager::takeStep()
{
    // Check if anything has actually been initialised yet. We may still be being loaded by Maya.
    if ( m_bAGroomingStepper == NULL )
        return;

    // Force self collisions to be off whilst testing
    m_bAGroomingStepper->skipRodRodCollisions( true );

    // Ensure the rod stays stuck on the head
    // Set boundary conditions, they don't need set every frame as the rods don't move but
    // in theory we can use this same plugin for moving rods so leave this here as it's a 
    // small overhead for ease of future development.
    for ( size_t r = 0; r < m_rods.size(); ++r )
    {
        ElasticRod* rod = m_rods[r];

        // Lock the first two vertices of the rod
        RodBoundaryCondition* boundary = m_rodTimeSteppers[r]->getBoundaryCondition();

        for ( int v = 0; v < rod->nv(); ++v )
        {
            boundary->releaseVertex( v );
        }

        // Set the position of the vertices we want to be fixed
        boundary->setDesiredVertexPosition( 0, rod->getVertex( 0 ) );
        boundary->setDesiredVertexPosition( 1, rod->getVertex( 1 ) );

        // and very importantly, set the desired angle too
        boundary->setDesiredEdgeAngle( 0, rod->getTheta( 0 ) );
    }

    m_bAGroomingStepper->execute();
}

void WmSweeneyRodManager::drawAllRods()
{
    bool renderRod = true;

    for ( size_t r = 0; r < m_rodRenderers.size(); ++r )
    {
        renderRod = true;

        // check if rod is in hidden subset
        int subsetIdx = m_rods[r]->getSubsetIdx();

        if ( subsetIdx >= 0 )
        {
            renderRod = m_subsetNodes[subsetIdx]->isVisible();
        }

        bool hasSolved = m_rodTimeSteppers[r]->HasSolved();

        if ( m_drawOnlyUnsolved )
        {
            // check if rod is unsolved and meant to be hidden
            renderRod = !hasSolved;
        }
        else if ( m_drawOnlySolved )
        {
            // check if rod is solved and meant to be hidden
            renderRod = hasSolved;
        }

        if ( renderRod )
        {
            m_rodRenderers[r]->render();
        }
    }
}

void WmSweeneyRodManager::createClumpCenterLinesFromPelt( const MPointArray& centralArr )
{
    unsigned int numRoots = centralArr.length();
    VecXd centerRoots( 3 * numRoots );

    for ( int i = 0; i < numRoots; i++ ) // Convert the MPointArray into a VecXd
    {
        const MPoint point = centralArr[i];
        centerRoots.segment<3> ( 3 * i ) = Vec3d( point[0], point[1], point[2] );
    }

    m_bAGroomingStepper->setWmPeltPoints( centerRoots );
}

void WmSweeneyRodManager::createGaussianVolumetricForce( WmSweeneyVolumetricNode* volumeNode )
{
    m_volumetricNodes.push_back( volumeNode );

    Eigen::Quaternion<double> q = volumeNode->getQuaternion();

    Vec3d scale = volumeNode->getScale();

    Vec3d center = volumeNode->getCenter();

    Mat3d sigma;
    sigma.diagonal() = scale;
    sigma = q.matrix() * sigma * ( q.matrix().transpose() );

    m_bAGroomingStepper->createGaussianVolumetricForce( volumeNode->getCharge(), 1.0, center, sigma );
}

bool WmSweeneyRodManager::updateGaussianVolumetricForce( const int volIdx )
{
    Scalar currentCharge;
    Scalar currentScale;
    Vec3d currentCenter;
    Mat3d currentSigma;

    m_bAGroomingStepper->checkGaussianVolumetricForce( volIdx, currentCharge, currentScale,
            currentCenter, currentSigma );

    Eigen::Quaternion<double> q = m_volumetricNodes[volIdx]->getQuaternion();

    Vec3d scale = m_volumetricNodes[volIdx]->getScale();

    Vec3d center = m_volumetricNodes[volIdx]->getCenter();

    Mat3d sigma;
    sigma.diagonal() = scale;
    sigma = q.matrix() * sigma * ( q.matrix().transpose() );

    if ( currentCharge != m_volumetricNodes[volIdx]->getCharge() || currentCenter != center
            || currentSigma != sigma )
    {
        m_bAGroomingStepper->updateGaussianVolumetricForce( volIdx,
                m_volumetricNodes[volIdx]->getCharge(),  1.0, center, sigma );
        return true;
    }

    return false;
}

bool WmSweeneyRodManager::updateSubsetVolumetricForces( )
{
    Scalar currentCharge, newCharge;
    Scalar currentScale, newScale;
    Vec3d currentCenter;
    Mat3d currentSigma;

    for ( int i = 0; i < m_subsetNodes.size(); ++i )
    {
        m_bAGroomingStepper->checkGaussianVolumetricForce( i, currentCharge, currentScale,
                currentCenter, currentSigma );

        // Update the volumetric transform for rendering
        Eigen::SelfAdjointEigenSolver<Mat3d> eigenSolver( currentSigma );
        Vec3d eigenvalues = eigenSolver.eigenvalues();
        Vec3d scale( currentScale * sqrt( eigenvalues[0] ), currentScale * sqrt( eigenvalues[1] ),
                currentScale * sqrt( eigenvalues[2] ) );
        m_subsetNodes[ i ]->setVolumetricTransform( scale, eigenSolver.eigenvectors().transpose(), currentCenter );

        newCharge = m_subsetNodes[ i ]->getVolumetricForceCharge();
        newScale = m_subsetNodes[ i ]->getVolumetricForceScale();
        if ( currentCharge != newCharge || currentScale != newScale )
        {
            m_bAGroomingStepper->updateSubsetVolumetricForce( i, newCharge, newScale );
            return true;
        }
    }
    return false;
}
