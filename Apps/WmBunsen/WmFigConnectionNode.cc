#include "WmFigConnectionNode.hh"

using namespace BASim;

// Required by Maya to identify node
/* static */ MTypeId WmFigConnectionNode::typeID ( 0x001135, 0xDA ); 
/* static */ MString WmFigConnectionNode::typeName( "wmFigConnectionNode" );

// 
/* static */ MObject WmFigConnectionNode::ia_time;
/* static */ MObject WmFigConnectionNode::ia_startTime;

/* static */ MObject WmFigConnectionNode::ia_rodNumber;
/* static */ MObject WmFigConnectionNode::ia_edgeNumber;
/* static */ MObject WmFigConnectionNode::ia_transformMatrix;

// We output the material frame to the rod node rather than having it connect directly because
// if this node is deleted we want the frame connection to be deleted too. So this node 
// routes it through.
/* static */ MObject WmFigConnectionNode::oa_materialFrame;

WmFigConnectionNode::WmFigConnectionNode() : m_startTime( 1 ), m_currentTime( 1 ), m_previousTime( 1 )
{
}

WmFigConnectionNode::~WmFigConnectionNode()
{
}

MStatus WmFigConnectionNode::compute( const MPlug& i_plug, MDataBlock& i_dataBlock ) 
{
    MStatus stat;
    
    //cerr << "WmFigConnectionNode::compute plug = " << i_plug.name() << endl;
	
    if ( i_plug == oa_materialFrame )
    {
        m_previousTime = m_currentTime;
        m_currentTime = i_dataBlock.inputValue( ia_time, &stat ).asTime().value();
        CHECK_MSTATUS( stat );
        
        m_startTime = i_dataBlock.inputValue( ia_startTime, &stat ).asDouble();
        CHECK_MSTATUS( stat );
        
        int edgeNumber = i_dataBlock.inputValue( ia_edgeNumber, &stat ).asInt();
        CHECK_MSTATUS( stat );

        int rodNumber = i_dataBlock.inputValue( ia_rodNumber, &stat ).asInt();
        CHECK_MSTATUS( stat );

        MMatrix transformMatrix = i_dataBlock.inputValue( ia_transformMatrix, &stat ).asMatrix();
        CHECK_MSTATUS( stat );
        
        MDataHandle materialFrameH = i_dataBlock.outputValue( oa_materialFrame, &stat );
        CHECK_MSTATUS( stat );
        
        materialFrameH.set( transformMatrix );
        
        materialFrameH.setClean();
        i_dataBlock.setClean( i_plug );
    
    }
    else
    {
		return MS::kUnknownParameter;
	}

	return MS::kSuccess;
}

void WmFigConnectionNode::draw( M3dView& i_view, const MDagPath& i_path,
                            M3dView::DisplayStyle i_style,
                            M3dView::DisplayStatus i_status )
{ 
	MStatus stat;
	MObject thisNode = thisMObject();

	
	i_view.beginGL(); 
	glPushAttrib( GL_CURRENT_BIT | GL_POINT_BIT | GL_LINE_BIT );
       
    glPopAttrib();
	i_view.endGL();
}

MStatus WmFigConnectionNode::connectionMade( const  MPlug & plug, const  MPlug & otherPlug, bool asSrc ) 
{    
    MStatus stat;
    MStatus retVal(MS::kUnknownParameter );
    
    return retVal;
}

MStatus WmFigConnectionNode::connectionBroken( const  MPlug & plug, const  MPlug & otherPlug, bool asSrc )
{
    MStatus stat;
    MStatus retVal( MS::kUnknownParameter );
    
    return retVal;
}

bool WmFigConnectionNode::isBounded() const
{ 
	return false;
}

void* WmFigConnectionNode::creator()
{
	return new WmFigConnectionNode();
}

/*static */ MStatus WmFigConnectionNode::addNumericAttribute( MObject& i_attribute, MString i_longName, 
    MString i_shortName, MFnNumericData::Type i_type, double i_defaultValue, bool i_isInput, bool i_isOutput )
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
    if ( i_isOutput )
        nAttr.setReadable( true );
    
    stat = addAttribute( i_attribute );
    if ( !stat ) { stat.perror( "addAttribute " + i_longName ); return stat; }

    return stat;
}

/* static */ MStatus WmFigConnectionNode::initialize()
{ 
    MStatus stat;

    {
        MFnMatrixAttribute mAttr;
        oa_materialFrame = mAttr.create( "materialFrame", "mf", MFnMatrixAttribute::kDouble, &stat );
        if ( !stat ) 
        {
            stat.perror("create oa_materialFrame attribute");
            return stat;
        }
        CHECK_MSTATUS( mAttr.setWritable( false ) );
        CHECK_MSTATUS( mAttr.setReadable( true ) );
        stat = addAttribute( oa_materialFrame );
        if ( !stat ) { stat.perror( "addAttribute oa_materialFrame" ); return stat; }
    }
    
    {
        MFnMatrixAttribute mAttr;
        ia_transformMatrix = mAttr.create( "transformMatrix", "tm", MFnMatrixAttribute::kDouble, &stat );
        if ( !stat ) 
        {
            stat.perror("create ia_transformMatrix attribute");
            return stat;
        }
        CHECK_MSTATUS( mAttr.setWritable( true ) );
        CHECK_MSTATUS( mAttr.setReadable( false ) );
        stat = addAttribute( ia_transformMatrix );
        if ( !stat ) { stat.perror( "addAttribute ia_transformMatrix" ); return stat; }
    }
    
    {
        MFnUnitAttribute	uAttr;
        ia_time = uAttr.create( "time", "t", MTime( 0.0 ), &stat );
        if ( !stat ) 
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
	stat = attributeAffects( ia_time, oa_materialFrame );
	if ( !stat ) { stat.perror( "attributeAffects ia_time->oa_materialFrame" ); return stat; }
    
    addNumericAttribute( ia_startTime, "startTime", "stt", MFnNumericData::kDouble, 1.0, true, false );
	stat = attributeAffects( ia_startTime, oa_materialFrame );
	if ( !stat ) { stat.perror( "attributeAffects ia_startTime->oa_materialFrame" ); return stat; }
    
    addNumericAttribute( ia_rodNumber, "rodNumber", "rn", MFnNumericData::kInt, 0, true, true );
	stat = attributeAffects( ia_rodNumber, oa_materialFrame );
	if ( !stat ) { stat.perror( "attributeAffects ia_rodNumber->oa_materialFrame" ); return stat; }
    
    addNumericAttribute( ia_edgeNumber, "edgeNumber", "en", MFnNumericData::kInt, 0, true, true );
	stat = attributeAffects( ia_edgeNumber, oa_materialFrame );
	if ( !stat ) { stat.perror( "attributeAffects ia_edgeNumber->oa_materialFrame" ); return stat; }
    
	return MS::kSuccess;
}
