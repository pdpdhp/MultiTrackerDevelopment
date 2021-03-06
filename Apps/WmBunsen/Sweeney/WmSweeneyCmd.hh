#ifndef WMSWEENEYCMD_HH_
#define WMSWEENEYCMD_HH_

#include <map>
#include <set>
#include <string>
#include <vector>

#include <maya/MArgDatabase.h>
#include <maya/MDagModifier.h>
#include <maya/MObject.h>
#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MSelectionList.h>
#include <maya/MStringArray.h>
#include <maya/MPointArray.h>
#include <maya/MItCurveCV.h>
#include <maya/MFnTransform.h>

struct WmSweeneyHelp
{
    std::string m_longName;
    std::string m_help;
    std::vector<MSyntax::MArgType> m_argTypes;
};

class WmSweeneyCmd: public MPxCommand
{
public:
    /// Constructor
    WmSweeneyCmd();
    /// Destructor
    virtual ~WmSweeneyCmd();

    /// Static Maya calls to create an instance of the command
    static void *creator();
    /// Static Maya calls to register command syntax
    static MSyntax syntaxCreator();

    /// Entry point the first time the command is invoked
    MStatus doIt( const MArgList &i_mArgList );
    /// Entry point if the command is redone
    MStatus redoIt();
    /// Entry point if the command is undone
    MStatus undoIt();
    /// Maya queries the command to see if it's undoable or not
    bool isUndoable() const
    {
        return m_undoable;
    }
    /// Maya queries the command to see if it has a registered Maya syntax or not
    bool hasSyntax() const
    {
        return true;
    }

    void getNodes( MSelectionList opt_nodes );

    static MString typeName;

protected:
    static void p_AddFlag( MSyntax &i_mSyntax, const char * const i_shortName,
            const char * const i_longName, const char * const i_help,
            const MSyntax::MArgType i_argType1 = MSyntax::kNoArg,
            const MSyntax::MArgType i_argType2 = MSyntax::kNoArg,
            const MSyntax::MArgType i_argType3 = MSyntax::kNoArg,
            const MSyntax::MArgType i_argType4 = MSyntax::kNoArg,
            const MSyntax::MArgType i_argType5 = MSyntax::kNoArg,
            const MSyntax::MArgType i_argType6 = MSyntax::kNoArg );

    MStatus createDagNode( const char *transformName, const char *nodeType, MObject &parentObj,
            MObject *transformObjP, MObject *shapeObjP, MDagModifier *iDagModifier,
            MString& o_shapeName );

    static void printHelp();

    static void getSelectedCurves( const MSelectionList &i_selectionList,
            MSelectionList &o_meshList );

    void p_PerformConnect();

    void createSweeneyNode();
    void createSweeneySubsetNode();
    MStatus createClumpCenterLinesFromPelt();
    MStatus createGaussianVolumetricForce();
    void addCollisionMeshes();
    void setSimulateAll();

public:
    // Data
protected:
    // Data
    static void appendToResultString( MString& i_resultString );

    /// True if the command is undoable, false otherwise
    bool m_undoable;

    enum Operation
    {
        Error = 0, Help, CreateRods,
    };

    // The operation we're supposed to be performing
    Operation m_op;

    // Context variables that may (or may not) have been provided on the command line
    MString m_node_name;

    /// Arguments passed the first time the command is called, used on redo
    MArgDatabase *m_mArgDatabase;

    // DG modified for Undo/Redo
    MDGModifier m_dgmodifier;

    /// Dag Modifier For Undo/Redo
    MDagModifier *m_mDagModifier;

    /// Undo For SetAttr
    //  Wmaya::undo_t *m_undo;

    /// Undo Selection List
    MSelectionList m_undoSelectionList;

    MSelectionList m_meshList; // list of meshes
    MSelectionList m_barberShopNodeList;
    MSelectionList m_sweeneyNodeList;
    MSelectionList m_allOtherTransformNodesList;
    MObject m_selectedSweeneyNode;

    MObject m_selectedSphereParentNode;
    MObject m_selectedPeltNode;
    MObject m_selectedPeltMeshNode;

    static MStringArray m_results;

    static std::map<std::string, WmSweeneyHelp> m_help;
};

#endif
