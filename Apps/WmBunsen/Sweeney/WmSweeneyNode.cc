#include "WmSweeneyNode.hh"
#include "WmFigConnectionNode.hh"
#include "../WmBunsenCollisionMeshNode.hh"

#include "../../../BASim/src/Physics/ElasticRods/RodBendingForceSym.hh"
#include "../../../BASim/src/Physics/ElasticRods/RodStretchingForce.hh"
#include "../../../BASim/src/Physics/ElasticRods/RodTwistingForceSym.hh"

using namespace BASim;

// Required by Maya to identify the node
/* static */MTypeId WmSweeneyNode::typeID(0x001135, 0xF6);
/* static */MString WmSweeneyNode::typeName("wmSweeneyNode");

// Input attributes
/* static */MObject WmSweeneyNode::ia_time;
/* static */MObject WmSweeneyNode::ia_startTime;

// Hair Property Attributes
/* static */ MObject WmSweeneyNode::ia_length;
/* static */ MObject WmSweeneyNode::ia_edgeLength;
/* static */ MObject WmSweeneyNode::ia_verticesPerRod;
/* static */ MObject WmSweeneyNode::ia_rodsPerClump;
/* static */ MObject WmSweeneyNode::ia_rodRadius;
/* static */ MObject WmSweeneyNode::ia_rodAspectRatio;
/* static */ MObject WmSweeneyNode::ia_rodRotation;
/* static */ MObject WmSweeneyNode::ia_curlTightness;
/* static */ MObject WmSweeneyNode::ia_curlRadius;
/* static */ MObject WmSweeneyNode::ia_curlCount;
/* static */ MObject WmSweeneyNode::ia_curlStart;
/* static */ MObject WmSweeneyNode::ia_rodPitch;
/* static */ MObject WmSweeneyNode::ia_fixCurlCount;
/* static */ MObject WmSweeneyNode::ia_curlInXFrame;
/* static */ MObject WmSweeneyNode::ia_preserveLengthVariation;
/* static */ MObject WmSweeneyNode::ia_rodDamping;
/* static */MObject WmSweeneyNode::ia_rodCharge;
/* static */MObject WmSweeneyNode::ia_rodPower;
/* static */MObject WmSweeneyNode::ia_rodClumpSeparation;

// Barbershop specific inputs
/*static*/MObject WmSweeneyNode::ia_strandVertices;
/*static*/MObject WmSweeneyNode::ia_strandRootFrames;
/*static*/MObject WmSweeneyNode::ia_verticesPerStrand;

// Output to the Barbershop guide curve deformer
/*static*/MObject WmSweeneyNode::oa_simulatedNurbs;

// Sync attributes
/* static */MObject WmSweeneyNode::ca_rodPropertiesSync;

// Collision meshes
/* static */MObject WmSweeneyNode::ia_collisionMeshes;

//Solver Tolerances
/* static */MObject WmSweeneyNode::ia_stol;
/* static */MObject WmSweeneyNode::ia_atol;
/* static */MObject WmSweeneyNode::ia_rtol;
/* static */MObject WmSweeneyNode::ia_inftol;
/* static */MObject WmSweeneyNode::ia_numLineSearchIters;

// Performance Tuning
//GeneralParameters
/* static */MObject WmSweeneyNode::ia_enablePenaltyResponse;
/* static */MObject WmSweeneyNode::ia_implicitThickness;
/* static */MObject WmSweeneyNode::ia_implicitStiffness;
/* static */MObject WmSweeneyNode::ia_levelsetSubsampling;
/* static */MObject WmSweeneyNode::ia_inextensibilityThreshold;

//Failuredetection
/* static */MObject WmSweeneyNode::ia_maxNumOfSolverIters;
/* static */MObject WmSweeneyNode::ia_maxNumOfCollisionIters;
/* static */MObject WmSweeneyNode::ia_enableExplosionDetection;
/* static */MObject WmSweeneyNode::ia_explosionDampening;
/* static */MObject WmSweeneyNode::ia_explosionThreshold;
/* static */MObject WmSweeneyNode::ia_stretchingThreshold;

//FailureResponse
/* static */MObject WmSweeneyNode::ia_solverFailure;
/* static */MObject WmSweeneyNode::ia_collisionFailure;
/* static */MObject WmSweeneyNode::ia_explosionFailure;
/* static */MObject WmSweeneyNode::ia_stretchingFailure;
/* static */MObject WmSweeneyNode::ia_maxNumSolverSubsteps;
/* static */MObject WmSweeneyNode::ia_maxNumCollisionSubsteps;
/* static */MObject WmSweeneyNode::ia_maxNumExplosionSubsteps;
/* static */MObject WmSweeneyNode::ia_maxNumStretchingSubsteps;

// Debug drawing
/* static */MObject WmSweeneyNode::ia_shouldDrawStrands;
/* static */MObject WmSweeneyNode::ia_shouldDrawRootFrames;
/* static */MObject WmSweeneyNode::ia_shouldDrawVelocity;

WmSweeneyNode::WmSweeneyNode() :
    m_rodManager(NULL)
{
}

WmSweeneyNode::~WmSweeneyNode()
{
}

WmSweeneyRodManager* WmSweeneyNode::rodManager()
{
    return m_rodManager;
}

MStatus WmSweeneyNode::compute(const MPlug& i_plug, MDataBlock& i_dataBlock)
{
	MStatus status;

	if ( i_plug == ca_rodPropertiesSync )
	{
	    m_currentTime = i_dataBlock.inputValue( ia_time ).asTime().value();
		m_startTime = i_dataBlock.inputValue( ia_startTime ).asDouble();

		// Hair properties
		m_length = i_dataBlock.inputValue( ia_length ).asDouble();
		m_edgeLength = i_dataBlock.inputValue( ia_edgeLength ).asDouble();
	    m_rodRadius = i_dataBlock.inputValue( ia_rodRadius ).asDouble();
		m_rodAspectRatio = i_dataBlock.inputValue( ia_rodAspectRatio ).asDouble();
		m_rodRotation = i_dataBlock.inputValue( ia_rodRotation ).asDouble();
		m_curlTightness = i_dataBlock.inputValue( ia_curlTightness ).asDouble();
	    m_curlRadius = i_dataBlock.inputValue( ia_curlRadius ).asDouble();
		m_curlCount = i_dataBlock.inputValue( ia_curlCount ).asDouble();
		m_curlStart = i_dataBlock.inputValue( ia_curlStart ).asDouble();
		m_rodPitch = i_dataBlock.inputValue( ia_rodPitch ).asDouble();
		m_fixCurlCount = i_dataBlock.inputValue( ia_fixCurlCount ).asBool();
		m_curlInXFrame = i_dataBlock.inputValue( ia_curlInXFrame ).asBool();
		m_preserveLengthVariation = i_dataBlock.inputValue( ia_preserveLengthVariation ).asBool();
		m_rodDamping = i_dataBlock.inputValue( ia_rodDamping ).asBool();
        m_rodCharge = i_dataBlock.inputValue(ia_rodCharge).asDouble();
        m_rodPower = i_dataBlock.inputValue(ia_rodPower).asDouble();
        m_rodClumpSeparation = i_dataBlock.inputValue(ia_rodClumpSeparation).asDouble();

        bool shouldDrawStrands = i_dataBlock.inputValue( ia_shouldDrawStrands ).asBool();
        bool shouldDrawRootFrames = i_dataBlock.inputValue( ia_shouldDrawRootFrames ).asBool();
        bool shouldDrawVelocity = i_dataBlock.inputValue( ia_shouldDrawVelocity ).asBool();
        if ( m_rodManager != NULL )
        {
            m_rodManager->setRodsDrawDebugging( shouldDrawStrands, shouldDrawRootFrames, shouldDrawVelocity );
            m_rodManager->drawAllRods();
        }     

		MObject strandVerticesObj = i_dataBlock.inputValue( ia_strandVertices ).data();
		MFnVectorArrayData strandVerticesArrayData( strandVerticesObj, &status );
		CHECK_MSTATUS( status );

		MVectorArray strandVertices = strandVerticesArrayData.array( &status );
		CHECK_MSTATUS( status );

		MDataHandle strandRootFramesHandle = i_dataBlock.inputValue( ia_strandRootFrames, & status );
		CHECK_MSTATUS( status );
		MObject strandRootFramesObj = strandRootFramesHandle.data();
		MFnVectorArrayData strandRootFramesArrayData( strandRootFramesObj, &status );
		CHECK_MSTATUS( status );

		MVectorArray strandRootFrames = strandRootFramesArrayData.array( &status );
		CHECK_MSTATUS( status );

		//for ( unsigned int i = 0 ; i < strandRootFrames.length() ; i++ )
		//	cout << "ROD FRAME # " << i << " " << strandRootFrames[ i ].x << " " << strandRootFrames[ i ].y << " " << strandRootFrames[ i ].z << endl;

		int verticesPerRod = i_dataBlock.inputValue( ia_verticesPerRod ).asInt();

		int rodsPerClump = i_dataBlock.inputValue( ia_rodsPerClump ).asInt();

		int numberOfVerticesPerStrand = i_dataBlock.inputValue( ia_verticesPerStrand ).asInt();

		if ( m_currentTime == m_startTime )
		{
			// We can't use the assignment operator because strandVertices is actually
			// a reference to the MVectorArrayData array and it will go out of scope
			// and we'll be left with a reference to nothing and obviously a crash.
			status = m_strandVertices.copy( strandVertices );
			CHECK_MSTATUS( status );

			status = m_strandRootFrames.copy( strandRootFrames );
			CHECK_MSTATUS( status );

			m_numberOfVerticesPerStrand = numberOfVerticesPerStrand;

			m_verticesPerRod = verticesPerRod;

			m_rodsPerClump = rodsPerClump;

			initialiseRodFromBarberShopInput( i_dataBlock );
		}
		else
		{
		    if ( m_rodManager != NULL )
		    {
                static double oldCharge = 0.0;
                static double oldPower = 1.0;
                static double oldClumpDist = 0.0;

                bool update_all_rods = false;

                if ((m_rodCharge != oldCharge)
                        || m_rodPower != oldPower
                        || m_rodClumpSeparation != oldClumpDist)
                {
                    m_rodManager->setClumpingParameters(oldCharge = m_rodCharge,
                            oldPower = m_rodPower, oldClumpDist = m_rodClumpSeparation);
                    update_all_rods = true;
                }

				// Apply kinetic damping
				m_rodManager->setUseKineticDamping( m_rodDamping );

				ElasticRod *current_rod;
				bool update_rod = update_all_rods;

				// for each rod, check if any parameters need to be updated,
				// perform necessary updates, and flag the rod accordingly
				for (size_t i = 0; i < m_rodManager->m_rods.size(); ++i)
				{

					update_rod = false;
					current_rod = m_rodManager->m_rods[i];

					// get total rod length for scaling
                    Scalar curl_length = ( 1.0 - m_curlStart )*m_length;
                    int curl_resolution = ( 1.0 - m_curlStart )*m_verticesPerRod;

                    // Compute the rod helix properties
                    Scalar curvature = 0.0;
                    Scalar torsion = 0.0;

                    //Scalar absolute_length = m_length * m_strandLengths[i];

                    if(  m_fixCurlCount && m_curlRadius != 0.0 && m_curlCount != 0.0 )
                    {
                        // compute total length of the helix
                        // TODO(sainsley): rename m_curlCount m_curlCount
                        // rename m_length to m_height
                        Scalar radius = m_curlRadius;
                        Scalar curl_height = m_length / m_curlCount;
                        Scalar arc_length = sqrt( curl_height * curl_height +
                                4 * M_PI * M_PI * radius * radius );
                        curl_length = arc_length * m_curlCount;

                        // compute helix curvature and torsion
                        Scalar pitch = curl_height / ( 2 * M_PI );
                        Scalar denom = radius  * radius  + pitch * pitch;
                        curvature = radius  /  denom;
                        torsion = pitch  / denom;
                        curvature *= curl_length / curl_resolution;
                        torsion *= curl_length / curl_resolution;
                        /*if ( i == 0 )
                            cout << "Curl Params: Curl_H " << curl_height << " arc_len " << arc_length
                            << " abs_len " << absolute_length << " curl_radius " << m_curlRadius << " curl_pitch " << pitch << " curvature "
                            << curvature << " torsion " << torsion << " resolutoin " << m_verticesPerRod << endl;*/
                    }

                    if (  !m_fixCurlCount && m_curlTightness != 0.0 )
                    {
                        curvature = m_curlTightness; // * m_length / m_verticesPerRod;
                    }

                    // length before curl
                    Scalar total_length = m_curlStart*m_length + curl_length;
                    if ( m_preserveLengthVariation )
                    {
                        total_length *= m_strandLengths[i];
                    }

					// Update rod configuration
					updateStrandLength( current_rod, update_rod, total_length );
					updateStrandCurl( current_rod, update_rod, curvature, torsion );
					updateStrandCrossSection( current_rod, update_rod );
					updateStrandRotation( current_rod, update_rod );

					current_rod->updateStiffness();

					// Check if rod is in rest state
					// cout << "WmSweeneyNode::check for rod update::Rod Idx: " << i <<
                       //     " update rod: " << update_rod << endl;
					current_rod->setIsInRestState( update_rod );
				}

				 updateCollisionMeshes( i_dataBlock );
				 updateSolverSettings( i_dataBlock );
				 m_rodManager->takeStep();

		    }
		}
		i_dataBlock.setClean( i_plug );
	}
	else if ( i_plug == oa_simulatedNurbs )
	{
		compute_oa_simulatedNurbs( i_plug, i_dataBlock );
	}
	else
	{
		return MS::kUnknownParameter;
	}
	return MS::kSuccess;
}

// todo CONST
void WmSweeneyNode::updateStrandLength( ElasticRod* current_rod, bool& update_rod, Scalar stand_length )
{

    assert( current_rod->m_stretchingForce != NULL );

    // Build vector of new edge lengths
    // NOTE(sainsley) : this is code assumes we may want non-uniform
    // edge lengths in the future. If this isn't the case, we want
    // a method to set the rest lengths of all edges to a constant
    // in order to avoid iterating over the edges twice
    Scalar updated_edge_length = stand_length / m_verticesPerRod;
    std::vector<Scalar> rest_lengths;

    // Adjust edge lengths and compute the resulting curl length
    for ( ElasticRod::edge_iter eh = current_rod->edges_begin();
          eh != current_rod->edges_end(); ++eh )
    {
        // check if the edge length needs to be updated
        // (ignoring the first edge as it is fixed)
        if ( eh != current_rod->edges_begin() && updated_edge_length !=
                current_rod->m_stretchingForce->getRefLength( *eh ) )
        {
            update_rod = true;
        }
        rest_lengths.push_back( updated_edge_length );

        // TODO(sainsley) : factor this back in
        // compute the total length and number of vertices in the curl
        //if ( eh->idx() >= m_curlStart*( m_verticesPerRod - 1 ) )
        //{
          //  curl_len += current_rod->getEdgeLength( *eh );
           // curl_resolution++;
        //}
    }

    //assert( m_curlStart == 1.0 || curl_len != 0 );

    // return if the rod is unchanged
    if( !update_rod ) return;

    // pass new rest lengths to the forces
    current_rod->setRestLengths( rest_lengths );
}

void WmSweeneyNode::updateStrandCrossSection( ElasticRod* current_rod, bool& update_rod )
{

    // adjust the rod size
    Scalar radius_a = m_rodRadius;
    Scalar radius_b = radius_a;
    // apply apsect ratio : flip axis if aspect ratio is less than 1
    // to preserve radius scale
    if ( m_rodAspectRatio > 1.0 )
    {
        radius_b *= m_rodAspectRatio;
    }
    else
    {
        radius_a *= 1.0/m_rodAspectRatio;
    }

    // check for a change in radius
    if ( radius_a == current_rod->radiusA( 0 ) && radius_b == current_rod->radiusB( 0 ) ) return;

    // update radius
    current_rod->setRadius( radius_a, radius_b );
    update_rod = true;

}

void WmSweeneyNode::updateStrandRotation( ElasticRod* current_rod, bool& update_rod )
{

    // default rotation is 180 degrees
    Scalar theta = m_rodRotation * M_PI + M_PI;

    // check for update
    if ( current_rod->getTheta( 0 ) ==  theta ) return;

    // set initial rotation
    current_rod->setTheta( 0,  theta );
    Scalar c = cos( current_rod->getTheta( 0 ) );
    Scalar s = sin( current_rod->getTheta( 0 ) );
    const Vec3d& u = current_rod->getReferenceDirector1( 0 );
    const Vec3d& v = current_rod->getReferenceDirector2( 0 );
    current_rod->setMaterial1( 0,  c * u + s * v );
    current_rod->setMaterial2( 0, -s * u + c * v );

}

void WmSweeneyNode::updateStrandCurl( ElasticRod* current_rod, bool& update_rod,
        Scalar curvature, Scalar torsion)
{

    assert( current_rod->m_bendingForce != NULL );
    assert( current_rod->m_twistingForce != NULL );

    for ( ElasticRod::vertex_iter vh = current_rod->vertices_begin();
              vh != current_rod->vertices_end(); ++vh )
    {

        if ( vh->idx() >= m_curlStart*m_verticesPerRod )
        {
            if ( current_rod->m_twistingForce->getUndeformedTwist( *vh ) != torsion )
            {
                current_rod->m_twistingForce->setUndeformedTwist( *vh, torsion );
                update_rod = true;
            }

            // curl along material frame axis of choice
            if ( m_curlInXFrame ) {
                if ( current_rod->m_bendingForce->getKappaBar( *vh )[0] != curvature )
                {
                    current_rod->m_bendingForce->setKappaBar( *vh, Vec2d( curvature, 0 ) );
                    update_rod = true;
                }
            }
            else
            {
                if ( current_rod->m_bendingForce->getKappaBar( *vh )[1] != curvature )
                {
                    current_rod->m_bendingForce->setKappaBar( *vh, Vec2d( 0,  curvature ) );
                    update_rod = true;
                }
            }
        }
    }
 }

void WmSweeneyNode::updateSolverSettings( MDataBlock &i_dataBlock )
{
    double atol = powf( 10, -i_dataBlock.inputValue( ia_atol ).asDouble() );
    double stol = powf( 10, -i_dataBlock.inputValue( ia_stol ).asDouble() );
    double rtol  = powf( 10, -i_dataBlock.inputValue( ia_rtol ).asDouble() );
    double inftol  = powf( 10, -i_dataBlock.inputValue( ia_inftol ).asDouble() );
    int numLineSearchIters = i_dataBlock.inputValue( ia_numLineSearchIters ).asInt();

    double stiffness = i_dataBlock.inputValue( ia_implicitStiffness ).asDouble();
    m_rodManager->updateSolverSettings( atol, stol, rtol, inftol, numLineSearchIters, stiffness );
}

void WmSweeneyNode::initialiseCollisionMeshes(MDataBlock &i_data)
{
    MStatus status;

    MArrayDataHandle inArrayH = i_data.inputArrayValue(ia_collisionMeshes, &status);
    CHECK_MSTATUS(status);
    size_t numMeshesConnected = inArrayH.elementCount();

    for (unsigned int i = 0; i < numMeshesConnected; i++)
    {
        // Even if we don't use it, grab the data so Maya knows to evaluate the node
        inArrayH.jumpToElement(i);
        MDataHandle collisionMeshH = inArrayH.inputValue(&status);
        CHECK_MSTATUS(status);

        MPlug plug(thisMObject(), ia_collisionMeshes);
        CHECK_MSTATUS(status);
        if (plug.isArray(&status))
        {
            MPlug indxPlug = plug.elementByLogicalIndex(i, &status);
            CHECK_MSTATUS(status);
            if (indxPlug.isConnected(&status))
            {
                MPlugArray inPlugArr;
                indxPlug.connectedTo(inPlugArr, true, false, &status);
                CHECK_MSTATUS(status);

                // Since we asked for the destination there can only be one plug in the array
                MPlug meshPlug = inPlugArr[0];
                MObject collisionMeshNodeObj = meshPlug.node(&status);
                CHECK_MSTATUS(status);
                MFnDependencyNode collisionMeshNodeFn(collisionMeshNodeObj);
                WmBunsenCollisionMeshNode* collisionMeshNode = (WmBunsenCollisionMeshNode*) collisionMeshNodeFn.userNode();

                TriangleMesh* triangleMesh = NULL;
                WmFigMeshController* figMeshController = NULL;

                collisionMeshNode->initialise(NULL, i, &triangleMesh, &figMeshController);

                // Now add the mesh to the rod manager
                m_rodManager->addCollisionMesh(triangleMesh, figMeshController->currentLevelSet(), figMeshController);
            }
            else
            {
                CHECK_MSTATUS(status);
            }
        }
    }
}

void WmSweeneyNode::updateCollisionMeshes(MDataBlock& i_dataBlock)
{
    MStatus status;

    MArrayDataHandle inArrayH = i_dataBlock.inputArrayValue(ia_collisionMeshes, &status);
    CHECK_MSTATUS(status);
    size_t numMeshesConnected = inArrayH.elementCount();

    for (unsigned int i = 0; i < numMeshesConnected; i++)
    {
        // All we need to do is ask Maya for the data and it will pull the attr,
        // causing a compute in the collision mesh node which will directly
        // update the collision mesh data in the RodManager.
        inArrayH.jumpToElement(i);
        MDataHandle collisionMeshH = inArrayH.inputValue(&status);
        CHECK_MSTATUS(status);
    }
}

void WmSweeneyNode::initialiseRodFromBarberShopInput(MDataBlock& i_dataBlock)
{
    // We need to be able to run a sim from pre-groomed barbershop strands too. That seems
    // pretty simple as we just take the vertices from the barbershop input and create the
    // rod from that. Then when taking a time step we set the first edge to match the barbershop
    // input. It looks like Sweeney code easily work for grooming or for dynamic sims.

    cerr << "initialiseRodFromBarberShopInput() - About to create rods from Barbershop input\n";

    // Reset the manager and remove all rods before adding more
    delete m_rodManager;

    m_rodManager = new WmSweeneyRodManager();

    cerr << "initialiseRodFromBarberShopInput() - Deleted and created a new WmSweeneyRodManager\n";

    if (m_strandVertices.length() == 0)
    {
        cerr << "initialiseRodFromBarberShopInput() - no input strands so can't create any rods";
        return;
    }

    // First, get all the collision mesh data organised
    initialiseCollisionMeshes(i_dataBlock);

    // Create one rod for each barbershop strand. Ignore the strand shape or length but do
    // take its initial direction as a follicle angle
    unsigned int currentVertexIndex = 0;
    unsigned int numberOfStrands = m_strandVertices.length() / m_numberOfVerticesPerStrand;

    cerr << "initialiseRodFromBarberShopInput() - m_strandVertices.length() = " << m_strandVertices.length() << endl;
    cerr << "initialiseRodFromBarberShopInput() - number of barberShop strands = " << numberOfStrands << endl;
    cerr << "initialiseRodFromBarberShopInput() - number of vertices per barberShop strand = " << m_numberOfVerticesPerStrand
            << endl;

    vector<BASim::Vec3d> vertices;
    m_strandLengths.clear();

    bool useRootFrames = ( m_strandRootFrames.length() != 0 );
    vector<Vec3d> scalpTangents;
    if ( useRootFrames )
    {
        useRootFrames = getScalpTangents( scalpTangents );
    }

    for (unsigned int inputStrandNumber = 0; inputStrandNumber < numberOfStrands; ++inputStrandNumber)
    {
        MVector direction = m_strandVertices[currentVertexIndex + 1] - m_strandVertices[currentVertexIndex];
        //direction.normalize();

        constructRodVertices(vertices, direction, m_strandVertices[currentVertexIndex]);

        cout << "initialiseRodFromBarberShopInput() - check for root frames for " << inputStrandNumber << endl;
        cerr << "initialiseRodFromBarberShopInput() - useRootFrames = " << useRootFrames << endl;
        if ( useRootFrames )
        {

        	/*BASim::Vec3d m1 = Vec3d(  m_strandRootFrames[ 3*inputStrandNumber ].x,
       			m_strandRootFrames[ 3*inputStrandNumber ].y,
       			m_strandRootFrames[ 3*inputStrandNumber ].z  );

        	BASim::Vec3d m2 = Vec3d(  m_strandRootFrames[ 3*inputStrandNumber + 2 ].x,
        	                m_strandRootFrames[ 3*inputStrandNumber + 2 ].y,
        	                m_strandRootFrames[ 3*inputStrandNumber + 2 ].z  );

        	BASim::Vec3d tan = Vec3d(  m_strandRootFrames[ 3*inputStrandNumber + 1 ].x,
        	                            m_strandRootFrames[ 3*inputStrandNumber + 1 ].y,
        	                            m_strandRootFrames[ 3*inputStrandNumber + 1 ].z  );
        	m1.normalize();
        	m2.normalize();
        	tan.normalize();*/

            BASim::Vec3d tan = Vec3d(  m_strandRootFrames[ 3*inputStrandNumber + 1 ].x,
                                                    m_strandRootFrames[ 3*inputStrandNumber + 1 ].y,
                                                    m_strandRootFrames[ 3*inputStrandNumber + 1 ].z  );
            tan.normalize();
            // take cross product of scalp tangent and strand tangent
            // for first material frame vector
            BASim::Vec3d m1 = tan.cross(scalpTangents[ inputStrandNumber ]);
            m1.normalize();
            if ( !approxEq(m1.dot(tan), 0.0, 1e-6) )
            {
               cerr << "initialiseRodFromBarberShopInput() : ERROR strand " << inputStrandNumber
                << " had improper root frame" << endl;
            }
        	m_rodManager->addRod( vertices, m_startTime, m1 );
        	// hack to fix scalp orientation bug
        	if ( scalpTangents[ inputStrandNumber ].x() < 0 )
        	  m_rodManager->m_rods[inputStrandNumber]->setIsLeftStrand(true);
        }
        else
        {
            m_rodManager->addRod(vertices, m_startTime);
        }

        cerr << "Creating rod at time " << m_startTime << endl;

        currentVertexIndex += m_numberOfVerticesPerStrand;
    }

    //Set performancetuningparameters to pass through
    PerformanceTuningParameters perfParams;
    perfParams.m_enable_penalty_response = i_dataBlock.inputValue(ia_enablePenaltyResponse).asBool();
    perfParams.m_implicit_thickness = i_dataBlock.inputValue(ia_implicitThickness).asDouble();
    perfParams.m_implicit_stiffness = i_dataBlock.inputValue(ia_implicitStiffness).asDouble();
    perfParams.m_levelset_subsampling = i_dataBlock.inputValue(ia_levelsetSubsampling).asInt();
    perfParams.m_inextensibility_threshold = i_dataBlock.inputValue(ia_inextensibilityThreshold).asInt();
    perfParams.m_solver.m_max_iterations = i_dataBlock.inputValue(ia_maxNumOfSolverIters).asInt();
    perfParams.m_collision.m_max_iterations = i_dataBlock.inputValue(ia_maxNumOfCollisionIters).asInt();
    perfParams.m_enable_explosion_detection = i_dataBlock.inputValue(ia_enableExplosionDetection).asBool();
    perfParams.m_explosion_damping = i_dataBlock.inputValue(ia_explosionDampening).asDouble();
    perfParams.m_explosion_threshold = i_dataBlock.inputValue(ia_explosionThreshold).asDouble();
    perfParams.m_stretching_threshold = i_dataBlock.inputValue(ia_stretchingThreshold).asDouble();
    perfParams.m_solver.m_in_case_of = (BASim::FailureMode::ResponseSeverity) i_dataBlock.inputValue(ia_solverFailure).asInt();
    perfParams.m_collision.m_in_case_of
            = (BASim::FailureMode::ResponseSeverity) i_dataBlock.inputValue(ia_collisionFailure).asInt();
    perfParams.m_explosion.m_in_case_of
            = (BASim::FailureMode::ResponseSeverity) i_dataBlock.inputValue(ia_explosionFailure).asInt();
    perfParams.m_stretching.m_in_case_of
            = (BASim::FailureMode::ResponseSeverity) i_dataBlock.inputValue(ia_stretchingFailure).asInt();
    perfParams.m_solver.m_max_substeps = i_dataBlock.inputValue(ia_maxNumSolverSubsteps).asInt();
    perfParams.m_collision.m_max_substeps = i_dataBlock.inputValue(ia_maxNumCollisionSubsteps).asInt();
    perfParams.m_explosion.m_max_substeps = i_dataBlock.inputValue(ia_maxNumExplosionSubsteps).asInt();
    perfParams.m_stretching.m_max_substeps = i_dataBlock.inputValue(ia_maxNumStretchingSubsteps).asInt();

    double m_atol = powf(10, -i_dataBlock.inputValue(ia_atol).asDouble());
    double m_stol = powf(10, -i_dataBlock.inputValue(ia_stol).asDouble());
    double m_rtol = powf(10, -i_dataBlock.inputValue(ia_rtol).asDouble());
    double m_inftol = powf(10, -i_dataBlock.inputValue(ia_inftol).asDouble());
    int m_numLineSearchIters = i_dataBlock.inputValue(ia_numLineSearchIters).asInt();

    cerr << "initialiseRodFromBarberShopInput() - About to initialise simulation\n";
    m_rodManager->initialiseSimulation(1 / 24.0, m_startTime, perfParams, m_atol, m_stol, m_rtol, m_inftol,
            m_numLineSearchIters, m_rodsPerClump);
    cerr << "initialiseRodFromBarberShopInput() - Simulation initialised at time " << m_startTime << endl;
}

void WmSweeneyNode::constructRodVertices( std::vector<BASim::Vec3d>& o_rodVertices, const MVector& i_direction,
        const MVector& i_rootPosition)
{

    // Construct a list of vertices for a rod with its root at i_rootPosition and going in direction
    // i_direction

    o_rodVertices.clear();

    MVector edge = i_direction;

    m_strandLengths.push_back(edge.length());

    cerr << "constructRodVertices(): m_length = " << m_length << endl;
    cerr << "constructRodVertices(): m_verticesPerRod = " << m_verticesPerRod << endl;
    cerr << "constructRodVertices(): i_direction = " << i_direction << endl;

    edge *= m_length / m_verticesPerRod;

    cerr << "constructRodVertices(): edgeLength = " << edge.length() << "\n";

    MVector currentVertex(i_rootPosition);

	for ( int v = 0; v < m_verticesPerRod; ++v )
    {
        // Straight rods as twist is controlled by the rod properties

        o_rodVertices.push_back(BASim::Vec3d(currentVertex.x, currentVertex.y, currentVertex.z));

        currentVertex += edge;

        //MVector newPoint( a * cos( (double)v ),
        //		b * (double)v, a * sin( (double)v ) );
        //

        // For testing, force a straight rod
        // MVector newPoint( 0.0, v, 0.0 );

        // The helix is created with the y-axis as the centre, rotate it
        // so that it has i_direction as the centre
        //MQuaternion rotationQ( MVector( 0.0, 1.0, 0.0 ), i_direction );
        //newPoint = newPoint.rotateBy( rotationQ );

        // Now move the point to sit where the Barbershop input strand comes from
        //newPoint += i_rootPosition;

        //o_rodVertices.push_back( BASim::Vec3d( newPoint.x, newPoint.y, newPoint.z ) );
    }

    cerr << "constructRodVertices(): Finished constructing rod vertices\n";
}

void WmSweeneyNode::compute_oa_simulatedNurbs(const MPlug& i_plug, MDataBlock& i_dataBlock)
{
    MStatus status;

    cerr << "compute_oa_simulatedNurbs()\n";

    // First pull all the inputs to make sure we're up to date.
    i_dataBlock.inputValue(ca_rodPropertiesSync, &status).asBool();
    CHECK_MSTATUS(status);

    // The above may have been clean so just make sure we actually read time
    i_dataBlock.inputValue(ia_time, &status).asTime().value();
    CHECK_MSTATUS(status);

    MArrayDataHandle simulatedNurbsArrayHandle = i_dataBlock.outputArrayValue(oa_simulatedNurbs, &status);
    CHECK_MSTATUS(status);

    MArrayDataBuilder simulatedNurbsArrayDataBuilder(&i_dataBlock, oa_simulatedNurbs,
            (unsigned int) m_rodManager->numberOfRods(), &status);
    CHECK_MSTATUS(status);

    for (size_t r = 0; r < m_rodManager->numberOfRods(); ++r)
    {
        MPointArray nurbsEditPoints;

        ElasticRod* rod = m_rodManager->rod(r);

        for (unsigned int v = 0; v < rod->nv(); v++)
        {
            BASim::Vec3d vertex = rod->getVertex(v);
            MPoint nurbsEditPoint(vertex[0], vertex[1], vertex[2]);
            nurbsEditPoints.append(nurbsEditPoint);
        }

        MFnNurbsCurveData nurbsDataFn;
        MObject nurbsDataObj = nurbsDataFn.create();
        MFnNurbsCurve nurbsFn;
        MObject nurbsObj = nurbsFn.createWithEditPoints(nurbsEditPoints, 1, MFnNurbsCurve::kOpen, false /*not 2d*/,
                false /*not rational*/, true /*uniform params*/, nurbsDataObj, &status);
        CHECK_MSTATUS(status);

        MDataHandle simulatedNurbsHandle = simulatedNurbsArrayDataBuilder.addElement((unsigned int) r, &status);
        CHECK_MSTATUS(status);

        status = simulatedNurbsHandle.set(nurbsDataObj);
        CHECK_MSTATUS(status);
    }

    simulatedNurbsArrayHandle.set(simulatedNurbsArrayDataBuilder);
    simulatedNurbsArrayHandle.setAllClean();

    i_dataBlock.setClean(i_plug);
}

void WmSweeneyNode::draw(M3dView& i_view, const MDagPath& i_path, M3dView::DisplayStyle i_style,
        M3dView::DisplayStatus i_status)
{
    MStatus status;

    // Pull on the sync plugs to cause compute() to be called if any
    // of the rod properties or time has changed.
    double d;

    MPlug propertiesSyncPlug(thisMObject(), ca_rodPropertiesSync);
    propertiesSyncPlug.getValue(d);

    i_view.beginGL();
    //glPushAttrib( GL_CURRENT_BIT | GL_POINT_BIT | GL_LINE_BIT );
    glPushAttrib( GL_ALL_ATTRIB_BITS);

    // draw dynamic Hair
    if (m_rodManager != NULL)
    {
        m_rodManager->drawAllRods();
    }

    glPopAttrib();

    i_view.endGL();
}

MStatus WmSweeneyNode::connectionMade(const MPlug & plug, const MPlug & otherPlug, bool asSrc)
{
    MStatus stat;
    MStatus retVal(MS::kUnknownParameter);

    return retVal;
}

MStatus WmSweeneyNode::connectionBroken(const MPlug & plug, const MPlug & otherPlug, bool asSrc)
{
    MStatus stat;
    MStatus retVal(MS::kUnknownParameter);

    return retVal;
}

bool WmSweeneyNode::isBounded() const
{
    return false;
}

void* WmSweeneyNode::creator()
{
    return new WmSweeneyNode();
}

/*static */MStatus WmSweeneyNode::addNumericAttribute(MObject& i_attribute, MString i_longName, MString i_shortName,
        MFnNumericData::Type i_type, double i_defaultValue, bool i_isInput, bool i_isArray)
{
    // Creates a numeric attribute with default attributes
    MStatus stat = MS::kSuccess;

    MFnNumericAttribute nAttr;
    i_attribute = nAttr.create(i_longName, i_shortName, i_type, i_defaultValue, &stat);
    if (!stat)
    {
        cerr << "Failed to create attribute " << i_longName << endl;
        return stat;
    }
    if (i_isInput)
    {
        nAttr.setWritable(true);
    }
    else
    {
        nAttr.setWritable(false);
    }

    if (i_isArray)
    {
        nAttr.setArray(true);
    }

    stat = addAttribute(i_attribute);
    if (!stat)
    {
        stat.perror("addAttribute " + i_longName);
        return stat;
    }

    return stat;
}

/* static */MStatus WmSweeneyNode::initialize()
{
    MStatus status;

    addNumericAttribute(ca_rodPropertiesSync, "rodPropertiesSync", "rps", MFnNumericData::kBoolean, false, false);

    {
        MFnUnitAttribute uAttr;
        ia_time = uAttr.create("time", "t", MTime(0.0), &status);
        if (!status)
        {
            status.perror("create ia_time attribute");
            return status;
        }
        CHECK_MSTATUS(uAttr.setWritable(true));
        CHECK_MSTATUS(uAttr.setConnectable(true));
        CHECK_MSTATUS(uAttr.setStorable(false));
        status = addAttribute(ia_time);
        if (!status)
        {
            status.perror("addAttribute ia_time");
            return status;
        }
    }
    status = attributeAffects(ia_time, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_time->ca_rodPropertiesSync");
        return status;
    }

    addNumericAttribute(ia_startTime, "startTime", "stt", MFnNumericData::kDouble, 1.0, true);
    status = attributeAffects(ia_startTime, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_startTime->ca_rodPropertiesSync");
        return status;
    }

    //addNumericAttribute( ia_length, "length", "len", MFnNumericData::kDouble, 10.0, true );
    //status = attributeAffects( ia_length, ca_rodPropertiesSync );
    //if ( !status ) { status.perror( "attributeAffects ia_length->ca_rodPropertiesSync" ); return status; }

    {
        MFnNumericAttribute numericAttr;
        ia_length = numericAttr.create("length", "len", MFnNumericData::kDouble, 10.0, &status);
        CHECK_MSTATUS(status);
        CHECK_MSTATUS(numericAttr.setReadable(true));
        CHECK_MSTATUS(numericAttr.setWritable(true));
        CHECK_MSTATUS(numericAttr.setMin(1.0));
        CHECK_MSTATUS(numericAttr.setMax(100.0));
        status = addAttribute(ia_length);
        CHECK_MSTATUS(status);

        status = attributeAffects(ia_length, ca_rodPropertiesSync);
        if (!status)
        {
            status.perror("attributeAffects ia_length->ca_rodPropertiesSync");
            return status;
        }
    }

    // TODO : remove this? i don't think we are using it
    addNumericAttribute(ia_edgeLength, "edgeLength", "ele", MFnNumericData::kDouble, 1.0, true);
    status = attributeAffects(ia_edgeLength, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_edgeLength->ca_rodPropertiesSync");
        return status;
    }

    //addNumericAttribute( ia_rodRadius, "rodRadius", "ror", MFnNumericData::kDouble, 0.0, true );
    {
        MFnNumericAttribute numericAttr;
        ia_rodRadius = numericAttr.create("rodRadius", "ror", MFnNumericData::kDouble, 0.005, &status);
        CHECK_MSTATUS(status);
        CHECK_MSTATUS(numericAttr.setReadable(true));
        CHECK_MSTATUS(numericAttr.setWritable(true));
        CHECK_MSTATUS(numericAttr.setMin(0.001));
        CHECK_MSTATUS(numericAttr.setMax(0.1));
        status = addAttribute(ia_rodRadius);
        CHECK_MSTATUS(status);

        status = attributeAffects(ia_rodRadius, ca_rodPropertiesSync);
        if (!status)
        {
            status.perror("attributeAffects ia_rodRadius->ca_rodPropertiesSync");
            return status;
        }
    }

    {
        MFnNumericAttribute numericAttr;
        ia_rodAspectRatio = numericAttr.create("rodAspectRatio", "roar", MFnNumericData::kDouble, 1.0, &status);
        CHECK_MSTATUS(status);
        CHECK_MSTATUS(numericAttr.setReadable(true));
        CHECK_MSTATUS(numericAttr.setWritable(true));
        CHECK_MSTATUS(numericAttr.setMin(0.1));
        CHECK_MSTATUS(numericAttr.setMax(10.0));
        status = addAttribute(ia_rodAspectRatio);
        CHECK_MSTATUS(status);

        status = attributeAffects(ia_rodAspectRatio, ca_rodPropertiesSync);
        if (!status)
        {
            status.perror("attributeAffects ia_rodAspectRatio->ca_rodPropertiesSync");
            return status;
        }
    }

    {
        MFnNumericAttribute numericAttr;
        ia_rodRotation = numericAttr.create("rodRotation", "rorot", MFnNumericData::kDouble, 0.0, &status);
        CHECK_MSTATUS(status);
        CHECK_MSTATUS(numericAttr.setReadable(true));
        CHECK_MSTATUS(numericAttr.setWritable(true));
        CHECK_MSTATUS(numericAttr.setMin(-1.0));
        CHECK_MSTATUS(numericAttr.setMax(1.0));
        status = addAttribute(ia_rodRotation);
        CHECK_MSTATUS(status);

        status = attributeAffects(ia_rodRotation, ca_rodPropertiesSync);
        if (!status)
        {
            status.perror("attributeAffects ia_rodRotation->ca_rodPropertiesSync");
            return status;
        }
    }

    addNumericAttribute(ia_rodCharge, "rodCharge", "rcg", MFnNumericData::kDouble, 0.0, true);
    status = attributeAffects(ia_rodCharge, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_rodCharge->ca_rodPropertiesSync");
        return status;
    }

    addNumericAttribute(ia_rodPower, "rodPower", "rpw", MFnNumericData::kDouble, 1.0, true);
    status = attributeAffects(ia_rodPower, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_rodPower->ca_rodPropertiesSync");
        return status;
    }


    addNumericAttribute(ia_rodClumpSeparation, "rodClumpSeparation", "rcdst", MFnNumericData::kDouble, 0.001, true);
    status = attributeAffects(ia_rodClumpSeparation, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_rodClumpSeparation->ca_rodPropertiesSync");
        return status;
    }

    {
           MFnNumericAttribute numericAttr;
            ia_curlTightness = numericAttr.create( "globalCurlTightness", "crltight", MFnNumericData::kDouble, 0.0, &status );
            CHECK_MSTATUS( status );
            CHECK_MSTATUS( numericAttr.setReadable( true ) );
            CHECK_MSTATUS( numericAttr.setWritable( true ) );
            CHECK_MSTATUS( numericAttr.setMin( -2.0 ) );
            CHECK_MSTATUS( numericAttr.setMax( 2.0 ) );
            status = addAttribute( ia_curlTightness );
            CHECK_MSTATUS( status );

            status = attributeAffects( ia_curlTightness, ca_rodPropertiesSync );
            if ( !status ) { status.perror( "attributeAffects ia_curlTightness->ca_rodPropertiesSync" ); return status; }
    }

    {
        MFnNumericAttribute numericAttr;
        ia_curlRadius = numericAttr.create( "curlRadius", "crlrad", MFnNumericData::kDouble, 0.0, &status );
        CHECK_MSTATUS( status );
        CHECK_MSTATUS( numericAttr.setReadable( true ) );
        CHECK_MSTATUS( numericAttr.setWritable( true ) );
        CHECK_MSTATUS( numericAttr.setMin( -2.0 ) );
        CHECK_MSTATUS( numericAttr.setMax( 2.0 ) );
        status = addAttribute( ia_curlRadius );
        CHECK_MSTATUS( status );

        status = attributeAffects( ia_curlRadius, ca_rodPropertiesSync );
        if ( !status ) { status.perror( "attributeAffects ia_curlRadius->ca_rodPropertiesSync" ); return status; }
    }

    {
    	MFnNumericAttribute numericAttr;
        ia_curlCount = numericAttr.create( "curlCount", "crlcnt", MFnNumericData::kDouble, 0.0, &status );
        CHECK_MSTATUS( status );
        CHECK_MSTATUS( numericAttr.setReadable( true ) );
        CHECK_MSTATUS( numericAttr.setWritable( true ) );
        CHECK_MSTATUS( numericAttr.setMin( 0.0 ) );
        CHECK_MSTATUS( numericAttr.setMax( 10.0 ) );
        status = addAttribute( ia_curlCount );
        CHECK_MSTATUS( status );

        status = attributeAffects( ia_curlCount, ca_rodPropertiesSync );
        if ( !status ) { status.perror( "attributeAffects ia_curlCount->ca_rodPropertiesSync" ); return status; }
    }

    {
    	MFnNumericAttribute numericAttr;
        ia_curlStart = numericAttr.create( "curlStart", "crlstrt", MFnNumericData::kDouble, 0.0, &status );
        CHECK_MSTATUS( status );
        CHECK_MSTATUS( numericAttr.setReadable( true ) );
        CHECK_MSTATUS( numericAttr.setWritable( true ) );
        CHECK_MSTATUS( numericAttr.setMin( 0.0 ) );
        CHECK_MSTATUS( numericAttr.setMax( 1.0 ) );
        status = addAttribute( ia_curlStart );
        CHECK_MSTATUS( status );

        status = attributeAffects( ia_curlStart, ca_rodPropertiesSync );
        if ( !status ) { status.perror( "attributeAffects ia_curlStart->ca_rodPropertiesSync" ); return status; }
    }

    addNumericAttribute( ia_fixCurlCount, "fixCurlCount", "fixcurlc", MFnNumericData::kBoolean, false, true );
    status = attributeAffects( ia_fixCurlCount, ca_rodPropertiesSync );
    if ( !status ) { status.perror( "attributeAffects ia_fixCurlCount->ca_rodPropertiesSync" ); return status; }

    addNumericAttribute( ia_curlInXFrame, "curlInXFrame", "curinx", MFnNumericData::kBoolean, true, true );
    status = attributeAffects( ia_curlInXFrame, ca_rodPropertiesSync );
    if ( !status ) { status.perror( "attributeAffects ia_curlInXFrame->ca_rodPropertiesSync" ); return status; }

    addNumericAttribute( ia_preserveLengthVariation, "preserveLengthVariation", "plenvar", MFnNumericData::kBoolean, true, true );
    status = attributeAffects( ia_preserveLengthVariation, ca_rodPropertiesSync );
    if ( !status ) { status.perror( "attributeAffects ia_preserveLengthVariation->ca_rodPropertiesSync" ); return status; }

    addNumericAttribute( ia_rodDamping, "rodDamping", "roddamp", MFnNumericData::kBoolean, true, true );
    status = attributeAffects( ia_rodDamping, ca_rodPropertiesSync );
    if ( !status ) { status.perror( "attributeAffects ia_rodDamping->ca_rodPropertiesSync" ); return status; }


    addNumericAttribute( ia_rodPitch, "rodPitch", "rop", MFnNumericData::kDouble, 0.5, true );
    status = attributeAffects( ia_rodPitch, ca_rodPropertiesSync );
    if ( !status ) { status.perror( "attributeAffects ia_rodPitch->ca_rodPropertiesSync" ); return status; }

    {
    	MFnTypedAttribute tAttr;
    	ia_strandVertices = tAttr.create( "strandVertices", "sve",
                                          MFnData::kVectorArray, & status );
    	CHECK_MSTATUS( status );
    	CHECK_MSTATUS( tAttr.setReadable( false ) );
    	CHECK_MSTATUS( tAttr.setWritable( true ) );
    	CHECK_MSTATUS( tAttr.setStorable( false ) );
    	status = addAttribute( ia_strandVertices );
    	CHECK_MSTATUS( status );
    }
    status = attributeAffects( ia_strandVertices, ca_rodPropertiesSync );
    if ( !status ) { status.perror( "attributeAffects ia_strandVertices->ca_rodPropertiesSync" ); return status; }

	{
		MFnTypedAttribute tAttr;
		ia_strandRootFrames = tAttr.create( "strandRootFrames", "srf",
											MFnData::kVectorArray, &status );
		CHECK_MSTATUS( status );
		CHECK_MSTATUS( tAttr.setReadable( false ) );
		CHECK_MSTATUS( tAttr.setWritable( true ) );
		CHECK_MSTATUS( tAttr.setStorable( false ) );
		status = addAttribute( ia_strandRootFrames );
		CHECK_MSTATUS( status );
	}
	status = attributeAffects( ia_strandRootFrames, ca_rodPropertiesSync );
	if ( !status ) { status.perror( "attributeAffects ia_strandRootFrames->ca_rodPropertiesSync" ); return status; }

	{
		MFnTypedAttribute tAttr;
		oa_simulatedNurbs = tAttr.create( "simulatedNurbs", "sns",
			MFnData::kNurbsCurve, & status );
		CHECK_MSTATUS( status );
	    CHECK_MSTATUS( tAttr.setArray( true ) );
	    CHECK_MSTATUS( tAttr.setReadable( true ) );
	    CHECK_MSTATUS( tAttr.setWritable( false ) );
	    CHECK_MSTATUS( tAttr.setConnectable( true ) );
	    CHECK_MSTATUS( tAttr.setUsesArrayDataBuilder( true ) );
	    status = addAttribute( oa_simulatedNurbs );
	    CHECK_MSTATUS( status );
	}
	status = attributeAffects( ia_time, oa_simulatedNurbs );
	if ( !status ) { status.perror( "attributeAffects ia_time->oa_simulatedNurbs" ); return status; }
	status = attributeAffects( ca_rodPropertiesSync, oa_simulatedNurbs );
	if ( !status ) { status.perror( "attributeAffects ca_rodPropertiesSync->oa_simulatedNurbs" ); return status; }

	{
		MFnNumericAttribute nAttr;
		ia_collisionMeshes = nAttr.create( "collisionMeshes", "com", MFnNumericData::kBoolean, false, &status );
		if (!status)
		{
			status.perror( "create ia_collisionMeshes attribute" );
		    return status;
		}
		nAttr.setWritable( true );
		nAttr.setReadable( false );
		nAttr.setConnectable( true );
		nAttr.setDisconnectBehavior( MFnAttribute::kDelete );
		nAttr.setArray( true );
		status = addAttribute( ia_collisionMeshes );
		if ( !status ) { status.perror( "addAttribute ia_collisionMeshes" ); return status; }
	}
	status = attributeAffects( ia_collisionMeshes, ca_rodPropertiesSync );
	if ( !status ) { status.perror( "attributeAffects ia_collisionMeshes->ca_rodPropertiesSync" ); return status; }

	//Solver settings
	addNumericAttribute( ia_stol, "stol", "stl", MFnNumericData::kDouble, 99, true );
		status = attributeAffects( ia_stol, ca_rodPropertiesSync );
	    if ( !status ) { status.perror( "attributeAffects ia_stol->ca_rodPropertiesSync" ); return status; }

	addNumericAttribute( ia_atol, "atol", "atl", MFnNumericData::kDouble, 8, true );
		status = attributeAffects( ia_atol, ca_rodPropertiesSync );
	    if ( !status ) { status.perror( "attributeAffects ia_atol->ca_rodPropertiesSync" ); return status; }

	addNumericAttribute( ia_rtol, "rtol", "rtl", MFnNumericData::kDouble, 99, true );
		status = attributeAffects( ia_rtol, ca_rodPropertiesSync );
	    if ( !status ) { status.perror( "attributeAffects ia_rtol->ca_rodPropertiesSync" ); return status; }

	addNumericAttribute( ia_inftol, "inftol", "itl", MFnNumericData::kDouble, 8, true );
	status = attributeAffects( ia_inftol, ca_rodPropertiesSync );
	if ( !status ) { status.perror( "attributeAffects ia_inftol->ca_rodPropertiesSync" ); return status; }

	addNumericAttribute( ia_numLineSearchIters, "numLineSearchIters", "nlsi", MFnNumericData::kInt, 2, true );
	status = attributeAffects( ia_numLineSearchIters, ca_rodPropertiesSync );
	if ( !status ) { status.perror( "attributeAffects ia_numLineSearchIters->ca_rodPropertiesSync" ); return status; }

	//General parameters
	addNumericAttribute( ia_enablePenaltyResponse, "enablePenaltyResponse", "epr", MFnNumericData::kBoolean, true, true );
	status = attributeAffects( ia_enablePenaltyResponse, ca_rodPropertiesSync );
	if ( !status ) { status.perror( "attributeAffects ia_enablePenaltyResponse->ca_rodPropertiesSync" ); return status; }

	addNumericAttribute( ia_implicitThickness, "implicitThickness", "imt", MFnNumericData::kDouble, 0.10, true );
	status = attributeAffects( ia_implicitThickness, ca_rodPropertiesSync );
	if ( !status ) { status.perror( "attributeAffects ia_implicitThickness->ca_rodPropertiesSync" ); return status; }

	addNumericAttribute( ia_implicitStiffness, "implicitStiffness", "ims", MFnNumericData::kDouble, 1.0, true );
	status = attributeAffects( ia_implicitStiffness, ca_rodPropertiesSync );
	if ( !status ) { status.perror( "attributeAffects ia_implicitStiffness->ca_rodPropertiesSync" ); return status; }

	addNumericAttribute( ia_levelsetSubsampling, "levelsetSubsampling", "lsss", MFnNumericData::kInt, 0, true );
	status = attributeAffects( ia_levelsetSubsampling, ca_rodPropertiesSync );
	if ( !status ) { status.perror( "attributeAffects ia_levelsetSubsampling->ca_rodPropertiesSync" ); return status; }

	addNumericAttribute( ia_inextensibilityThreshold, "inextensibilityThreshold", "ixf", MFnNumericData::kInt, 0, true );
	status = attributeAffects( ia_inextensibilityThreshold, ca_rodPropertiesSync );
	if ( !status ) { status.perror( "attributeAffects ia_inextensibilityThreshold->ca_rodPropertiesSync" ); return status; }
    // Debug drawing
	// strands
    addNumericAttribute(ia_shouldDrawStrands, "shouldDrawStrands", "sdstrds", MFnNumericData::kBoolean, true, true);
    status = attributeAffects(ia_shouldDrawStrands, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_shouldDrawStrands->ca_rodPropertiesSync");
        return status;
    }
	// root frames
    addNumericAttribute(ia_shouldDrawRootFrames, "shouldDrawRootFrames", "sdrf", MFnNumericData::kBoolean, true, true);
    status = attributeAffects(ia_shouldDrawRootFrames, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_shouldDrawRootFrames->ca_rodPropertiesSync");
        return status;
    }
	// velocity
    addNumericAttribute(ia_shouldDrawVelocity, "shouldDrawVelocity", "sdv", MFnNumericData::kBoolean, false, true);
    status = attributeAffects(ia_shouldDrawVelocity, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_shouldDrawVelocity->ca_rodPropertiesSync");
        return status;
    }


    //Failure  Detection
    addNumericAttribute(ia_maxNumOfSolverIters, "maxNumOfSolverIters", "mnsi", MFnNumericData::kInt, 250, true);
    status = attributeAffects(ia_maxNumOfSolverIters, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_maxNumOfSolverIters->ca_rodPropertiesSync");
        return status;
    }

    addNumericAttribute(ia_maxNumOfCollisionIters, "maxNumOfCollisionIters", "mnci", MFnNumericData::kInt, 0, true);
    status = attributeAffects(ia_maxNumOfCollisionIters, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_maxNumOfCollisionIters->ca_rodPropertiesSync");
        return status;
    }

    addNumericAttribute(ia_enableExplosionDetection, "enableExplosionDetection", "eex", MFnNumericData::kBoolean, true, true);
    status = attributeAffects(ia_enableExplosionDetection, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_enableExplosionDetection->ca_rodPropertiesSync");
        return status;
    }

    addNumericAttribute(ia_explosionDampening, "explosionDampening", "exd", MFnNumericData::kDouble, 100.0, true);
    status = attributeAffects(ia_explosionDampening, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_iexplosionDampening->ca_rodPropertiesSync");
        return status;
    }

    addNumericAttribute(ia_explosionThreshold, "explosionThreshold", "ext", MFnNumericData::kDouble, 0.5, true);
    status = attributeAffects(ia_explosionThreshold, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_explosionThreshold->ca_rodPropertiesSync");
        return status;
    }

    addNumericAttribute(ia_stretchingThreshold, "stretchingThreshold", "sxt", MFnNumericData::kDouble, 2.0, true);
    status = attributeAffects(ia_stretchingThreshold, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_stretchingThreshold->ca_rodPropertiesSync");
        return status;
    }

    {
        MFnEnumAttribute enumAttrFn;
        ia_solverFailure = enumAttrFn.create("ifSolverStillFails", "svf", (short) FailureMode::IgnoreError, &status);
        CHECK_MSTATUS(status);
        enumAttrFn.addField("Ignore error", (short) FailureMode::IgnoreError);
        enumAttrFn.addField("Kill the rod", (short) FailureMode::KillTheRod);
        enumAttrFn.addField("Halt simulation", (short) FailureMode::HaltSimulation);
        enumAttrFn.setKeyable(false);
        enumAttrFn.setStorable(true);
        enumAttrFn.setWritable(true);
        enumAttrFn.setReadable(true);
        status = addAttribute(ia_solverFailure);
        CHECK_MSTATUS(status);
    }
    status = attributeAffects(ia_solverFailure, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_solverFailure->ca_rodPropertiesSync");
        return status;
    }

    addNumericAttribute(ia_maxNumSolverSubsteps, "maxNumSolverSubsteps", "mnss", MFnNumericData::kInt, 0, true);
    status = attributeAffects(ia_maxNumSolverSubsteps, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_maxNumSolverSubsteps->ca_rodPropertiesSync");
        return status;
    }

    {
        MFnEnumAttribute enumAttrFn;
        ia_collisionFailure = enumAttrFn.create("ifCollisionStillFails", "clf", (short) FailureMode::IgnoreError, &status);
        CHECK_MSTATUS(status);
        enumAttrFn.addField("Ignore error", (short) FailureMode::IgnoreError);
        enumAttrFn.addField("Kill the rod", (short) FailureMode::KillTheRod);
        enumAttrFn.addField("Halt simulation", (short) FailureMode::HaltSimulation);
        enumAttrFn.setKeyable(false);
        enumAttrFn.setStorable(true);
        enumAttrFn.setWritable(true);
        enumAttrFn.setReadable(true);
        status = addAttribute(ia_collisionFailure);
        CHECK_MSTATUS(status);
    }
    status = attributeAffects(ia_collisionFailure, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_collisionFailure->ca_rodPropertiesSync");
        return status;
    }

    addNumericAttribute(ia_maxNumCollisionSubsteps, "maxNumCollisionSubsteps", "mncs", MFnNumericData::kInt, 0, true);
    status = attributeAffects(ia_maxNumCollisionSubsteps, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_maxNumCollisionSubsteps->ca_rodPropertiesSync");
        return status;
    }

    {
        MFnEnumAttribute enumAttrFn;
        ia_explosionFailure = enumAttrFn.create("ifExplosionStillFails", "exf", (short) FailureMode::IgnoreError, &status);
        CHECK_MSTATUS(status);
        enumAttrFn.addField("Ignore error", (short) FailureMode::IgnoreError);
        enumAttrFn.addField("Kill the rod", (short) FailureMode::KillTheRod);
        enumAttrFn.addField("Halt simulation", (short) FailureMode::HaltSimulation);
        enumAttrFn.setKeyable(false);
        enumAttrFn.setStorable(true);
        enumAttrFn.setWritable(true);
        enumAttrFn.setReadable(true);
        status = addAttribute(ia_explosionFailure);
        CHECK_MSTATUS(status);
    }
    status = attributeAffects(ia_explosionFailure, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_explosionFailure->ca_rodPropertiesSync");
        return status;
    }

    addNumericAttribute(ia_maxNumExplosionSubsteps, "maxNumExplosionSubsteps", "mnes", MFnNumericData::kInt, 7, true);
    status = attributeAffects(ia_maxNumExplosionSubsteps, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_maxNumExplosionSubsteps->ca_rodPropertiesSync");
        return status;
    }

    {
        MFnEnumAttribute enumAttrFn;
        ia_stretchingFailure = enumAttrFn.create("ifStretchingStillFails", "sxf", (short) FailureMode::KillTheRod, &status);
        CHECK_MSTATUS(status);
        enumAttrFn.addField("Ignore error", (short) FailureMode::IgnoreError);
        enumAttrFn.addField("Kill the rod", (short) FailureMode::KillTheRod);
        enumAttrFn.addField("Halt simulation", (short) FailureMode::HaltSimulation);
        enumAttrFn.setKeyable(false);
        enumAttrFn.setStorable(true);
        enumAttrFn.setWritable(true);
        enumAttrFn.setReadable(true);
        status = addAttribute(ia_stretchingFailure);
        CHECK_MSTATUS(status);
    }
    status = attributeAffects(ia_stretchingFailure, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_stretchingFailure->ca_rodPropertiesSync");
        return status;
    }

    addNumericAttribute(ia_maxNumStretchingSubsteps, "maxNumStretchingSubsteps", "mnts", MFnNumericData::kInt, 0, true);
    status = attributeAffects(ia_maxNumStretchingSubsteps, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_maxNumStretchingSubsteps->ca_rodPropertiesSync");
        return status;
    }

    addNumericAttribute(ia_verticesPerStrand, "verticesPerStrand", "vps", MFnNumericData::kInt, 12, true);
    status = attributeAffects(ia_verticesPerStrand, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects verticesPerStrand->ca_rodPropertiesSync");
        return status;
    }

    addNumericAttribute(ia_verticesPerRod, "verticesPerRod", "cpr", MFnNumericData::kInt, 10, true);
    status = attributeAffects(ia_verticesPerRod, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_verticesPerRod->ca_rodPropertiesSync");
        return status;
    }

    addNumericAttribute(ia_rodsPerClump, "rodsPerClump", "rpc", MFnNumericData::kInt, 5, true);
    status = attributeAffects(ia_rodsPerClump, ca_rodPropertiesSync);
    if (!status)
    {
        status.perror("attributeAffects ia_rodsPerClump->ca_rodPropertiesSync");
        return status;
    }

    return MS::kSuccess;
}

void WmSweeneyNode::getSurfaceTangent(MFnMesh& scalp, BASim::Vec3d& surface_tan, MPoint root, const BASim::Vec3d strand_tan)
{
    // find closest point on scalp to vertex along first edge of the strand
    MPoint closestPoint;
    MPoint pointOnStrand = root + MPoint( surface_tan.x(), surface_tan.y(), surface_tan.z() );
    MStatus status = scalp.getClosestPoint( root , closestPoint);
    surface_tan = Vec3d ( closestPoint.x - root.x,  closestPoint.y - root.y, closestPoint.z - root.z );
    surface_tan.normalize();
    // MStatus     getClosestPoint (const MPoint &toThisPoint, MPoint &theClosestPoint, MSpace::Space space=MSpace::kObject, int *closestPolygon=NULL) cons
    // create vector from root closest point and normalize
    // use cross product of this "tangent" vector with the strand tangent as the new material frame m1 vector
}

bool WmSweeneyNode::getScalpTangents(vector<BASim::Vec3d>& i_scalpTangents)
{
    MStatus status;
    MPlugArray connectedPlugs;
    MPlug currentPlug;

    MFnDagNode sweeneyNode = MFnDagNode(thisMObject());
    // grab strand array plug from sweeney
    currentPlug = sweeneyNode.findPlug("strandVertices", true, &status);
    if (!status)
    {
        status.perror("cannot locate plug strandVertices for WmSweeneyNode");
        return false;
    }
    // grab connected strand array plug from barber shop
    bool foundPlugs = currentPlug.connectedTo(connectedPlugs, true, false, &status);
    if (!status || !foundPlugs)
    {
        status.perror("cannot locate wmBarbFurSetNode plug strandVertices->WmSweeneyNode");
        return false;
    }
    // grab furset node
    MFnDagNode fursetNode = MFnDagNode(connectedPlugs[0].node(&status));
    if (!status)
    {
        status.perror("cannot locate wmBarbFurSetNode parent node from wmBarbFurSetNode strandVertices plug");
        return false;
    }
    // grab regrow plug from fur set
    currentPlug = fursetNode.findPlug("regrow", true, &status);
    if (!status)
    {
        status.perror("cannot locate plug regrow from wmBarbFurSetNode");
        return false;
    }
    // grab connected regrow plug from subd node
    foundPlugs = currentPlug.connectedTo(connectedPlugs, true, false, &status);
    if (!status || !foundPlugs)
    {
        status.perror("cannot locate WmBarbSubdNode plug rebuild->WmBarbFurSetNode");
        return false;
    }
    // grab subd node node
    MFnDagNode subdNode = MFnDagNode(connectedPlugs[0].node(&status));
    if (!status)
    {
        status.perror("cannot locate WmBarbSubdNod parent node from WmBarbSubdNod rebuild plug");
        return false;
    }
    // grab inputMesh plug from subd node
    currentPlug = subdNode.findPlug("inputMesh", true, &status);
    if (!status)
    {
        status.perror("cannot locate plug inputMesh from WmBarbSubdNode");
        return false;
    }
    // grab connected worldMesh plug from mesh
    foundPlugs = currentPlug.connectedTo(connectedPlugs, true, false, &status);
    if (!status || !foundPlugs)
    {
        status.perror("cannot locate mesh plug worldMesh->WmBarbSubdNode");
        return false;
    }
    // grab mesh node
    MFnDagNode meshNode = MFnDagNode(connectedPlugs[0].node(&status));
    if (!status)
    {
        status.perror("cannot locate mesh parent node from mesh worldMesh plug");
        return false;
    }
    // get path for mesh dagnode
    MDagPath meshPath;
    status = meshNode.getPath(meshPath);
    if (!status)
    {
        status.perror("cannot recover MDagPath for mesh MFnDagNode");
        return false;
    }
    MFnMesh scalpMesh = MFnMesh(meshPath);
    Vec3d strandTan;
    i_scalpTangents.clear();
    for ( int i = 0; i < m_strandRootFrames.length()/3; i++ )
    {
        strandTan = Vec3d(  m_strandRootFrames[ 3*i + 1 ].x, m_strandRootFrames[ 3*i + 1 ].y,
                m_strandRootFrames[ 3*i + 1 ].z  );
        strandTan.normalize();
        Vec3d scalpTan;
        MPoint root = m_strandVertices[ m_numberOfVerticesPerStrand*i ];
        getSurfaceTangent( scalpMesh, scalpTan, root, strandTan );
        i_scalpTangents.push_back( scalpTan );
    }
    return true;
}

