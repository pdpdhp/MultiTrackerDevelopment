#include "WmFigRodBarbInput.hh"

#include <maya/MGlobal.h>

using namespace std;

WmFigRodBarbInput::WmFigRodBarbInput( MObject& i_verticesAttribute, MObject& i_strandRootFramesAttribute, 
                        double i_percentageOfBarbStrands, int i_verticesPerRod,
                        bool i_lockFirstEdgeToInput, double i_vertexSpacing,
                        double i_minimumRodLength, RodOptions& i_rodOptions,
                        double i_massDamping, BASim::Vec3d& i_gravity,  WmFigRodGroup& i_rodGroup,
                        RodTimeStepper::Method i_solverType, std::set< int >& i_simulationSet ) : 
    m_verticesAttribute( i_verticesAttribute ),
    m_strandRootFramesAttribute( i_strandRootFramesAttribute ),
    m_percentageOfBarbStrands( i_percentageOfBarbStrands ),
    m_verticesPerRod( i_verticesPerRod ), m_lockFirstEdgeToInput( i_lockFirstEdgeToInput ),
    m_vertexSpacing( i_vertexSpacing ), m_minimumRodLength( i_minimumRodLength ),
    m_rodGroup( i_rodGroup ), m_rodOptions( i_rodOptions ), m_massDamping( i_massDamping ),
    m_gravity( i_gravity ),
    m_solverType( i_solverType ), m_simulationSet( i_simulationSet )
{
    m_simulating = true;
}

WmFigRodBarbInput::~WmFigRodBarbInput()
{
}

void WmFigRodBarbInput::initialiseRodDataFromInput( MDataBlock& i_dataBlock )
{
    MStatus stat;

    MVectorArray vertices;
    int numStrands;
    getStrandVertices( i_dataBlock, vertices, numStrands );

    // store the material frames coming from barbershop
    vector<MaterialFrame> strandRootFrames;
    getStrandRootFrames( i_dataBlock, strandRootFrames );

    m_rodGroup.removeAllRods();

    int rodIndex = 0;

    int inputStrandVertexIndex = 0;
    for ( int i = 0; i < numStrands; i++ )
    {       

        // First check if the user has decided to specify a set of rods they want to simulate.
        bool isPlaceHolderRod = false;
        if ( m_simulationSet.size() != 0 )
        {   
            isPlaceHolderRod = ( m_simulationSet.count( i ) == 0 );
        
            if ( isPlaceHolderRod)
            {
                // Add a placeholder rod, most likely too small to actually be a real rod.
                m_rodGroup.addRod();

                continue;
            }
        }

        // If the user has specificed a vertex spacing > 0 then they want to override the vertices
        // that came from Barbershop. Very sensible of them as the rods stability is much reduced 
        // with vertices that are very close together.
        if ( m_vertexSpacing > 0.0 )
        {
            // First find how many vertices we need based on the length of the strand
            
            vector< MVector > curve;
            curve.resize( m_verticesPerRod );

            curve[ 0 ] = vertices[ inputStrandVertexIndex++ ]; 
            double length = 0.0;

            for ( int v = 1; v < m_verticesPerRod; v++ )
            {
                // The vertices array contains the vertices of *all* input strands
                // so we need to keep a seperate counter for it or it will always be giving
                // us the first strand.
                curve[ v ] = vertices[ inputStrandVertexIndex++ ];
                length += ( curve[ v ] - curve[ v - 1 ] ).length();
            }

            if ( length < m_minimumRodLength )
            {
                MGlobal::displayWarning( MString( "Input strand " ) + i + " is less than minimum length (" + m_minimumRodLength + "), not turning it into a rod." );

                // We still create a placeholder so that the indices match up from the inputs to the
                // rods.
                int rodIndex = m_rodGroup.addRod();
            }
            else
            {
                int numVerticesRequired = length / m_vertexSpacing;
    
                if ( numVerticesRequired < 3 )
                {
                    numVerticesRequired = 3;
                    MGlobal::displayWarning( MString( "Input strand " ) + i + " was going to have less than 3 vertices in the rod, setting it to 3." );
                }
    
                vector< MVector > resampledCurve;
                resampleCurve( numVerticesRequired, curve, resampledCurve );
                 
                vector<BASim::Vec3d> inputStrandVertices;
                inputStrandVertices.resize( numVerticesRequired );

                for ( int v = 0; v < numVerticesRequired; v++ )
                {
                    MVector vertex = resampledCurve[ (int)v ];
        
                    inputStrandVertices[ v ] = BASim::Vec3d( vertex[0], vertex[1], vertex[2] );                                                                  
                }
                
                // Create space to store the data for each CV.
                //(*i_pRodData)[i]->allocateStorage( numVerticesRequired );
                //(*i_pRodData)[ i ]->resetVertexPositions( inputStrandVertices );

                RodOptions rodOptions = m_rodOptions;
                rodOptions.numVertices = (int)inputStrandVertices.size();

                // Mass damping should be in rod options, it's dumb to pass it seperately.
                rodIndex = m_rodGroup.addRod( inputStrandVertices, rodOptions, m_massDamping, 
                                              m_gravity, m_solverType );
            }
        }
        else
        {
            vector<BASim::Vec3d> inputStrandVertices;
            inputStrandVertices.resize( m_verticesPerRod );
            
            // Just use the vertices that came from Barbershop
            // Make sure we have enough space to store the data for each CV.
            //(*i_pRodData)[i]->allocateStorage( m_verticesPerRod );
        
            for ( int v=0; v<m_verticesPerRod; v++ )
            {
                // The vertices array contains the vertices of *all* input strands
                // so we need to keep a seperate counter for it or it will always be giving
                // us the first strand.
                MVector vertex = vertices[ inputStrandVertexIndex++ ];
    
                inputStrandVertices[ v ] = BASim::Vec3d( vertex[0], vertex[1], vertex[2] );                                          
            }

            RodOptions rodOptions = m_rodOptions;
            rodOptions.numVertices = m_verticesPerRod;

            rodIndex = m_rodGroup.addRod( inputStrandVertices, rodOptions, m_massDamping, m_gravity, m_solverType );
        }

        // We need to add edge data so that each from the first edge will be locked to the input curve
		// 
		// We're not actually passing the orientation to BASim so no point in pretending we are
        /*if ( strandRootFrames.size() > i && m_lockFirstEdgeToInput )
        {
            // The strand root frames may not have been connected for some reason so don't
            // rely on this having data

            // rod is probably null at this point but rodData will deal with it nicely
            //(*i_pRodData)[ i ]->addKinematicEdge( 0, (*i_pRodData)[ i ]->rod, &strandRootFrames[ i ] );
            
            m_rodGroup.addKinematicEdge( rodIndex, 0, &strandRootFrames[ i ] );
        }
        else*/
        {   // If there's no root frame data then just lock the edge and it will not rotate.

            if ( !m_rodGroup.isPlaceHolderRod( rodIndex ) )
            {
                m_rodGroup.addKinematicEdge( rodIndex, 0 );
            }
        }
    }
}

void WmFigRodBarbInput::updateRodDataFromInput( MDataBlock& i_dataBlock )
{
 
    MVectorArray strandVertices;
    int numStrands;

    getStrandVertices( i_dataBlock, strandVertices, numStrands );

    // If there are no strands to use to update then don't update
    if ( numStrands == 0 )
    {
        cerr << "No input strands to update with\n";
        return;
    }

    // Every Barbershop strand has the same number of vertices
    //int numStrandVertices = strandVertices.length() / numStrands;

    // store the material frames coming from barbershop
    vector<MaterialFrame> strandRootFrames;
    getStrandRootFrames( i_dataBlock, strandRootFrames );

    if ( numStrands != m_rodGroup.numberOfRods() )
    {
        MGlobal::displayError( "Number of Barbershop strands does not equal number of rods!"
                                "Did you change the input? Rewind sim to reset." );
        return;
    }

    int inputStrandVertexIndex = 0;
    for ( int i = 0; i < numStrands; i++ )
    {
        // Check if this is a real rod before we actually do anything
        if ( m_rodGroup.isPlaceHolderRod( i ) )
        {
            // As all the input strands come from the same array we need to skip it forward
            // past this strand, ready for the next rod.

            inputStrandVertexIndex += m_verticesPerRod;
            continue;
        }

        int numVerticesInRod = m_rodGroup.numberOfVerticesInRod( i );

        vector<BASim::Vec3d> inputStrandVertices;         
        inputStrandVertices.resize( numVerticesInRod );

        if ( numVerticesInRod != m_verticesPerRod )
        {
            vector< MVector > curve;
            curve.resize( m_verticesPerRod );

            for ( int v = 0; v < m_verticesPerRod; v++ )
            {
                curve[ v ] = strandVertices[ inputStrandVertexIndex++ ];
            }

            vector< MVector > resampledCurve;
            resampleCurve( numVerticesInRod, curve, resampledCurve );

            for ( int v = 0; v < numVerticesInRod; v++ )
            {
                inputStrandVertices[ v ] = BASim::Vec3d( resampledCurve[ v ][ 0 ], resampledCurve[ v ][ 1 ], resampledCurve[ v ][ 2 ] );
            }
        }
        else
        {
            for ( int v = 0; v < m_verticesPerRod; v++ )
            {
                MVector vertex = strandVertices[ inputStrandVertexIndex++ ];
                inputStrandVertices[ v ] = BASim::Vec3d( vertex[ 0 ], vertex[ 1 ], vertex[ 2 ] );
            }
        }

        m_rodGroup.updateRodNextVertexPositions( i, inputStrandVertices );

        // The strand root frames may not have been connected for some reason so don't
        // rely on having that data
        if ( strandRootFrames.size() > i && m_lockFirstEdgeToInput )
        {
            //(*i_pRodData)[ i ]->updateKinematicEdge( 0, strandRootFrames[ i ] );
            m_rodGroup.updateKinematicEdge( i, 0, strandRootFrames[ i ] );
        }
        else // remove the entry in the map
        {
            // FIXME:
            // We haven't released a version of barbershop that gives the frames so
            // dont remove the edge just yet, leave it and we'll get the first edge
            // locked we just won't track rotation properly without the frames.

            //(*mx_rodData)[ i ]->removeKinematicEdge( 0 );
        }
    }
}

int WmFigRodBarbInput::numberOfInputs( MDataBlock& i_dataBlock )
{
    MStatus stat;

    MDataHandle verticesH = i_dataBlock.inputValue( m_verticesAttribute, &stat );
    CHECK_MSTATUS( stat );
    MFnVectorArrayData verticesData( verticesH.data(), &stat );
    CHECK_MSTATUS( stat );
    MVectorArray vertices = verticesData.array( &stat );

    return ( vertices.length() / m_verticesPerRod ) * ( m_percentageOfBarbStrands / 100.0 );
}

void WmFigRodBarbInput::getStrandVertices( MDataBlock& i_dataBlock, MVectorArray& o_vertices, 
    int& o_numStrands )
{
    MStatus stat;

    MDataHandle verticesH = i_dataBlock.inputValue( m_verticesAttribute, &stat );
    CHECK_MSTATUS( stat );
    MFnVectorArrayData verticesData( verticesH.data(), &stat );
    CHECK_MSTATUS( stat );
    o_vertices = verticesData.array( &stat );

    o_numStrands = o_vertices.length() / m_verticesPerRod;

    // The user may have requested that we use less than the total number
    // of Fozzie strands, so scale by that %
    o_numStrands *= m_percentageOfBarbStrands / 100.0;
}

void WmFigRodBarbInput::getStrandRootFrames( MDataBlock& i_dataBlock, 
    vector<MaterialFrame>& o_strandRootFrames )
{
    MStatus stat;

    MDataHandle strandRootFramesH = i_dataBlock.inputValue( m_strandRootFramesAttribute, &stat );
    CHECK_MSTATUS( stat );
    MFnVectorArrayData strandRootFrameVecData( strandRootFramesH.data(), &stat );
    
    if ( !stat )    // Strand root frames are not connected so just leave
        return;
        
    CHECK_MSTATUS( stat );

    MVectorArray strandRootFrameVec;
    strandRootFrameVec = strandRootFrameVecData.array( &stat );

    o_strandRootFrames.resize( strandRootFrameVec.length()/3 );
    MVector v;
    unsigned int idx = 0;
    for ( unsigned int rIdx=0; rIdx < o_strandRootFrames.size(); rIdx++ )
    {
        v = strandRootFrameVec[ idx++ ];
        o_strandRootFrames[ rIdx ].m1 = BASim::Vec3d( v[0], v[1], v[2] );
        v = strandRootFrameVec[ idx++ ];
        o_strandRootFrames[ rIdx ].m2 = BASim::Vec3d( v[0], v[1], v[2] );
        v = strandRootFrameVec[ idx++ ];
        o_strandRootFrames[ rIdx ].m3 = BASim::Vec3d( v[0], v[1], v[2] );
    }
}
