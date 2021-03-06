#ifndef _WMBUNSENCOLLISIONMESHNODE_HH_
#define _WMBUNSENCOLLISIONMESHNODE_HH_

#ifdef WETA
//#include <weta/Wfigaro/Collisions/CollisionMeshData.hh>
#include <weta/Wfigaro/Core/TriangleMesh.hh>
#include <weta/Wfigaro/Render/TriangleMeshRenderer.hh>
#else
//#include <BASim/src/Collisions/CollisionMeshData.hh>
#include "BASim/src/Core/TriangleMesh.hh"
#include "BASim/src/Render/TriangleMeshRenderer.hh"
#endif

#include "Beaker.hh"
#include "WmFigMeshController.hh"

#include <maya/MPxLocatorNode.h>
#include <maya/MString.h> 
#include <maya/MTypeId.h> 
#include <maya/MPlug.h>
#include <maya/MMatrix.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MColor.h>
#include <maya/M3dView.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNurbsSurface.h>
#include <maya/MFnNurbsSurfaceData.h>
#include <maya/MFnPointArrayData.h>
#include <maya/MFnMatrixData.h>
#include <maya/MIOStream.h>
#include <maya/MTime.h>
#include <maya/MGlobal.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnVectorArrayData.h>
#include <maya/MVectorArray.h>
#include <maya/MFnNurbsCurveData.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MFnStringData.h>
#include <maya/MFnMesh.h>

using namespace BASim;

class WmBunsenCollisionMeshNode : public MPxLocatorNode 
{
public:
    WmBunsenCollisionMeshNode();
	virtual	~WmBunsenCollisionMeshNode();
    virtual MStatus compute( const MPlug& i_plug, MDataBlock& i_data );
	virtual void draw( M3dView & view, const MDagPath & path, 
                       M3dView::DisplayStyle style,
                       M3dView::DisplayStatus status );

	virtual bool isBounded() const;
	
	virtual MStatus connectionMade( const  MPlug & plug, const  MPlug & otherPlug, bool asSrc );
	virtual MStatus connectionBroken( const  MPlug & plug, const  MPlug & otherPlug, bool asSrc );

    virtual void postConstructor();
	
	static  void* creator();
	static  MStatus initialize();

	static MTypeId typeId;
    static MString typeName;
	static MObject ia_inMesh;
    static MObject ia_startTime;
    static MObject ia_time;
    static MObject oa_meshData;
    
    static MObject ia_friction;
    static MObject ia_thickness;
    static MObject ia_separationStrength;
    static MObject ia_damping;
    static MObject ia_coefficientOfRestitution;
    static MObject ia_fullCollisions;
    static MObject ia_drawCollisionData;
    
    // Level Set attributes
    static MObject ia_createLevelSet;
    static MObject ia_levelSetCellSize;
    static MObject ia_drawLevelSet;
    static MObject ia_isLevelSetStatic;
    static MObject ia_meshTransform;
    
    
    // Recieve a pointer to Beaker so that we can update beaker with the details for this mesh
    void initialise( Beaker* i_beaker, const unsigned int i_collisionMeshIndex,
                     TriangleMesh** o_currentMesh = NULL,  
                     WmFigMeshController** o_meshController = NULL );

    //CollisionMeshData* collisionMeshData() { return m_collisionMeshData; }
        			
private:
    MStatus updateCollisionMeshFromMayaMesh( MFnMesh &meshFn, bool forceReset = false,
                                             std::string filename = "" );
    void updateLevelSetFromInputs( MDataBlock& i_dataBlock );

    double m_currentTime;
    double m_previousTime;
    double m_startTime;
    
    double m_friction;
    double m_thickness;
    bool m_fullCollisions;
    bool m_drawCollisionData;
    bool m_enableCaching;
    MString m_cacheDirectory;
    //CollisionMeshData *m_collisionMeshData;
    TriangleMesh* m_nextMesh;        // Mesh at next full frame
    TriangleMesh* m_previousMesh;    // mesh at previous full frame
    TriangleMesh* m_currentMesh;     // Mesh at current in between frame in simulation substeps
    TriangleMeshRenderer* m_triangleMeshRenderer;
    bool m_meshConnected;

    unsigned int m_collisionMeshIndex;
    Beaker* m_beaker;
    WmFigMeshController* m_meshController;
    
    Eigen::Matrix4f m_meshTransformMatrix;
};


#endif
