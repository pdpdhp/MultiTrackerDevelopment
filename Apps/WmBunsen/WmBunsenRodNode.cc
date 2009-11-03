#include "WmBunsenRodNode.hh"

///////////////////////////////////////////////////////////////////////////////////
// WARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNING
// WARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNING
// The below typeID is NOT A VALID WETA ID
// WARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNING
// WARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNINGWARNING
///////////////////////////////////////////////////////////////////////////////////

// Required by Maya to identify node
/* static */ MTypeId WmBunsenRodNode::ia_typeID( 0x80007 ); 
/* static */ MString WmBunsenRodNode::ia_typeName( "wmBunsenRodNode" );

// 
/* static */ MObject WmBunsenRodNode::ia_time;
/* static */ MObject WmBunsenRodNode::ia_startTime;
/* static */ MObject WmBunsenRodNode::ia_nurbsCurves;

// User adjustable rod Options
/* static */ MObject WmBunsenRodNode::ia_cvsPerRod;
/* static */ MObject WmBunsenRodNode::ia_youngsModulus;
/* static */ MObject WmBunsenRodNode::ia_shearModulus;
/* static */ MObject WmBunsenRodNode::ia_minorRadius;
/* static */ MObject WmBunsenRodNode::ia_majorRadius;

// Output and cache attributes
/* static */ MObject WmBunsenRodNode::ca_syncAttrs;
/* static */ MObject WmBunsenRodNode::oa_rodsChanged;

WmBunsenRodNode::WmBunsenRodNode() : m_initialised( false ), mx_rodData( NULL ), mx_world( NULL ),
                                     m_numberOfInputCurves( 0 )
{
    m_rodOptions.YoungsModulus = 1000.0;
    m_rodOptions.ShearModulus = 375.0;
    m_rodOptions.radiusA = 0.5;
    m_rodOptions.radiusB = 1.0;
}

WmBunsenRodNode::~WmBunsenRodNode()
{
}

void WmBunsenRodNode::initialiseRodData( vector<RodData*>* i_rodData )
{
    mx_rodData = i_rodData;
    
    // We need to resize the rodData vectors to hold the correct number of per vertex data
    // for all our inputs
    
    updateRodDataFromInputs();
    
    // Since we're initialising the rod data that means the rod is about to be created. In which
    // case we need to set the current vertex positions since they will not get set by the
    // simulation until the user moves forward a frame.
    
    size_t numRods = mx_rodData->size();
    for ( size_t r=0; r<numRods; r++ )
    {
        size_t numVertices = (*mx_rodData)[ r ]->undeformedVertexPositions.size();
        (*mx_rodData)[ r ]->initialVertexPositions.resize( numVertices );
        for ( size_t v=0; v<numVertices; v++ )
        {
            (*mx_rodData)[ r ]->initialVertexPositions[ v ] = (*mx_rodData)[ r ]->undeformedVertexPositions[ v ];
        }
    }
}

void WmBunsenRodNode::updateRodDataFromInputs( )
{    
    
    if ( mx_rodData->size() != m_numberOfInputCurves )
    {
        MGlobal::displayError( "Number of rods does not equal number of input curves, rewind simulation to reset" );
        return;
    }
    
    MDataHandle inputCurveH;
    MObject inputCurveObj;
    MPoint pt;
    MFnNurbsCurveData dataCreator;
    MMatrix mat;
    MPoint rootP;
    MStatus stat;
 
    // We may get called from not inside a compute by the WmBunsenNode initialising all its
    // data. So build our own datablock to use.
    MDataBlock dataBlock = MPxNode::forceCache();
    
    MArrayDataHandle inArrayH = dataBlock.inputArrayValue( ia_nurbsCurves, &stat );
    CHECK_MSTATUS(stat);
   
    size_t numCurvesConnected = inArrayH.elementCount();

    for ( unsigned int i = 0; i < numCurvesConnected; i++ )
    {
        inArrayH.jumpToElement( i );
        inputCurveH = inArrayH.inputValue( &stat );
        CHECK_MSTATUS( stat );

        // Use asNurbsCurveTransformed to get the curve data as we
        // want it in world space.
        inputCurveObj = inputCurveH.asNurbsCurveTransformed();
        MFnNurbsCurve inCurveFn( inputCurveObj );
        
        MPoint cv;
        int nCVs = inCurveFn.numCVs();
       
        (*mx_rodData)[i]->rodOptions = m_rodOptions;
        
        // Overrid the number of cvs at the moment to match the input curve
        (*mx_rodData)[i]->rodOptions.numVertices = nCVs;
        
        // Make sure we have enough space to store the date for each CV. This should only
        // ever cause a resize when we are called by initialiseRodData().
        (*mx_rodData)[ i ]->undeformedVertexPositions.resize( nCVs );
        (*mx_rodData)[ i ]->initialVertexPositions.resize( nCVs );
        
        std::string frame = "time";
        if (frame == "time") 
            (*mx_rodData)[i]->rodOptions.refFrame = ElasticRod::TimeParallel;
        else if (frame == "space") 
            (*mx_rodData)[i]->rodOptions.refFrame = ElasticRod::SpaceParallel;
    
        for ( int c = 0; c < (*mx_rodData)[ i ]->rodOptions.numVertices; ++c ) 
        {
            MPoint cv;
            stat = inCurveFn.getCV( c,cv,MSpace::kWorld );
            CHECK_MSTATUS( stat );
            (*mx_rodData)[ i ]->undeformedVertexPositions[ c ] = Vec3d( cv.x, cv.y, cv.z );
        }
    }
}

MStatus WmBunsenRodNode::compute( const MPlug& i_plug, MDataBlock& i_dataBlock ) 
{
    MStatus stat;
	
    if ( i_plug == ca_syncAttrs )
    {
        m_previousTime = m_currentTime;
        m_currentTime = i_dataBlock.inputValue( ia_time, &stat ).asTime().value();
		CHECK_MSTATUS( stat );
        
	    m_startTime = i_dataBlock.inputValue( ia_startTime, &stat ).asDouble();
		CHECK_MSTATUS( stat );
        
        m_rodOptions.YoungsModulus = i_dataBlock.inputValue( ia_youngsModulus, &stat ).asDouble();
        CHECK_MSTATUS( stat );
        m_rodOptions.ShearModulus = i_dataBlock.inputValue( ia_shearModulus, &stat ).asDouble();
        CHECK_MSTATUS( stat );
        m_rodOptions.radiusA = i_dataBlock.inputValue( ia_minorRadius, &stat ).asDouble();
        CHECK_MSTATUS( stat );
        m_rodOptions.radiusB = i_dataBlock.inputValue( ia_majorRadius, &stat ).asDouble();
        CHECK_MSTATUS( stat );
		
        updateRodDataFromInputs();

		MDataHandle outputData = i_dataBlock.outputValue ( ca_syncAttrs, &stat );
		if ( !stat )
        {
			stat.perror("WmBunsenRodNode::compute get ca_syncAttrs");
			return stat;
		}
		
        // We don't even need to put anything in the output handle as nothing uses it.
        // Just tell Maya it's clean so it doesn't repeatedly evaluate it.

		stat = i_dataBlock.setClean( i_plug );
		if ( !stat )
        {
			stat.perror("WmBunsenRodNode::compute setClean");
			return stat;
		}
	} 
    else if (  i_plug == oa_rodsChanged )
    {
        if ( mx_rodData != NULL )
        {
            // The Bunsen node is asking us to update the rod data in Beaker for our inputs
            // so do so here.....
        }
    }
    else
    {
		return MS::kUnknownParameter;
	}

	return MS::kSuccess;
}

void WmBunsenRodNode::draw( M3dView& i_view, const MDagPath& i_path,
                            M3dView::DisplayStyle i_style,
                            M3dView::DisplayStatus i_status )
{ 
	MStatus stat;
	MObject thisNode = thisMObject();

	MPlug syncPlug( thisNode, ca_syncAttrs );
	double d; 
	stat = syncPlug.getValue( d );
	if ( !stat ) 
    {
		stat.perror( "WmBunsenRodNode::draw getting ca_syncAttrs" );
		return;
	}

	i_view.beginGL(); 
	glPushAttrib( GL_CURRENT_BIT | GL_POINT_BIT | GL_LINE_BIT );
    
	// draw dynamic Hair

	// What did this line do? it was here from the devkit example. Is it to with point colouring
	//view.setDrawColor ( WmBunsenRodNode );

	glPopAttrib();
	i_view.endGL();
}

MStatus WmBunsenRodNode::connectionMade( const  MPlug & plug, const  MPlug & otherPlug, bool asSrc ) 
{    
    MStatus stat;
    MStatus retVal(MS::kUnknownParameter );

    if( plug == ia_nurbsCurves )
    {
        m_numberOfInputCurves++;
    }

    return retVal;
}

MStatus WmBunsenRodNode::connectionBroken( const  MPlug & plug, const  MPlug & otherPlug, bool asSrc )
{
    MStatus stat;
    MStatus retVal( MS::kUnknownParameter );

    if( plug == ia_nurbsCurves )
    {
        m_numberOfInputCurves--;
    }

    return retVal;
}


bool WmBunsenRodNode::isBounded() const
{ 
	return false;
}

void* WmBunsenRodNode::creator()
{
	return new WmBunsenRodNode();
}

/*static */ MStatus WmBunsenRodNode::addNumericAttribute( MObject& i_attribute, MString i_longName, 
    MString i_shortName, MFnNumericData::Type i_type, double i_defaultValue, bool i_isInput )
{
    // Creates a numeric attribute with default attributes
    MStatus stat = MS::kSuccess;
    
    MFnNumericAttribute nAttr;
    i_attribute = nAttr.create( i_longName, i_shortName, i_type, i_defaultValue, &stat );
    if ( !stat )
    {
        cerr << "Failed to create attribute " << i_longName << endl;
        return stat;
    }
    if ( i_isInput )
        nAttr.setWritable( true );
    else
        nAttr.setWritable( false );
    
    stat = addAttribute( i_attribute );
    if ( !stat ) { stat.perror( "addAttribute " + i_longName ); return stat; }

    return stat;
}

/* static */ MStatus WmBunsenRodNode::initialize()
{ 
    MStatus stat;

    addNumericAttribute( ca_syncAttrs, "syncAttrs", "sya", MFnNumericData::kDouble, 1.0, false );
    addNumericAttribute( oa_rodsChanged, "rodsChanged", "rch", MFnNumericData::kBoolean, true, false );
    
    {
        MFnUnitAttribute	uAttr;
        ia_time = uAttr.create( "time", "t", MTime( 0.0 ), &stat );
        if ( !stat) 
        {
            stat.perror("create ia_time attribute");
            return stat;
        }
        CHECK_MSTATUS( uAttr.setWritable(true) );
        CHECK_MSTATUS( uAttr.setConnectable(true) );
        CHECK_MSTATUS( uAttr.setStorable(false) );
        stat = addAttribute( ia_time );
        if ( !stat ) { stat.perror( "addAttribute ia_time" ); return stat; }
    }
    
    addNumericAttribute( ia_startTime, "startTime", "stt", MFnNumericData::kDouble, 1.0, true );
	stat = attributeAffects( ia_startTime, ca_syncAttrs );
	if ( !stat ) { stat.perror( "attributeAffects ia_startTime->ca_syncAttrs" ); return stat; }

    addNumericAttribute( ia_youngsModulus, "youngsModulus", "ymo", MFnNumericData::kDouble, 1000.0, true );
    stat = attributeAffects( ia_youngsModulus, ca_syncAttrs );
	if ( !stat ) { stat.perror( "attributeAffects ia_youngsModulus->ca_syncAttrs" ); return stat; }

    addNumericAttribute( ia_shearModulus, "shearModulus", "shm", MFnNumericData::kDouble, 375.0, true );
    stat = attributeAffects( ia_shearModulus, ca_syncAttrs );
	if ( !stat ) { stat.perror( "attributeAffects ia_shearModulus->ca_syncAttrs" ); return stat; }
    
    addNumericAttribute( ia_minorRadius, "minorRadius", "mir", MFnNumericData::kDouble, 0.5, true );
    stat = attributeAffects( ia_minorRadius, ca_syncAttrs );
	if ( !stat ) { stat.perror( "attributeAffects ia_minorRadius->ca_syncAttrs" ); return stat; }

    addNumericAttribute( ia_majorRadius, "majorRadius", "mar", MFnNumericData::kDouble, 1.0, true );
    stat = attributeAffects( ia_majorRadius, ca_syncAttrs );
	if ( !stat ) { stat.perror( "attributeAffects ia_majorRadius->ca_syncAttrs" ); return stat; }
    
    {
        MFnTypedAttribute   tAttr;  
        ia_nurbsCurves = tAttr.create( "nurbsCurves", "nc",
                                       MFnNurbsCurveData::kNurbsCurve, &stat );
        CHECK_MSTATUS( stat );
        CHECK_MSTATUS( tAttr.setArray(true) );
        CHECK_MSTATUS( tAttr.setReadable(false) );
        CHECK_MSTATUS( tAttr.setWritable(true) );
        CHECK_MSTATUS( tAttr.setUsesArrayDataBuilder(true) );
        stat = addAttribute( ia_nurbsCurves );
        CHECK_MSTATUS( stat );
    }
    
	stat = attributeAffects( ia_time, ca_syncAttrs );
	if ( !stat ) { stat.perror( "attributeAffects ia_time->ca_syncAttrs" ); return stat; }

    stat = attributeAffects( ia_nurbsCurves, oa_rodsChanged );
    if ( !stat ) { stat.perror( "attributeAffects ia_nurbsCurves->oa_rodsChanged" ); return stat; }
    
	return MS::kSuccess;
}
