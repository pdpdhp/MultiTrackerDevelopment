#ifndef TOPOBJUTIl_H
#define TOPOBJUTIl_H

//Utility operations on TopologicalObjects, eg. messing with connectivity.
//These use the iterators and add/delete[Simplex] functions, rather than messing with internals.
//This is perhaps less optimal, but likely far simpler than writing the special case code.
#include "BASim/src/Core/TopologicalObject/TopObjHandles.hh"
#include "BASim/src/Core/TopologicalObject/TopologicalObject.hh"

namespace BASim {

//Given an edge, return the two wings (adjacent faces) of that edge.
bool getEdgeFacePair(const TopologicalObject& obj, const EdgeHandle& eh, FaceHandle& f0, FaceHandle& f1);

//Given two edges, find a shared vertex if one exists.
VertexHandle getSharedVertex(const TopologicalObject& obj, const EdgeHandle& e0, const EdgeHandle& e1);

//Given an edge and a vertex which is on the edge, return the edge's other vertex
VertexHandle getEdgesOtherVertex(const TopologicalObject& obj, const EdgeHandle &eh, const VertexHandle& vh);

//Split an edge and insert a new vertex in between, subdividing all the faces sharing the edge.
//Should be non-manifold friendly. Orientation is maintained.
VertexHandle splitEdge(TopologicalObject& obj, const EdgeHandle& h, std::vector<FaceHandle>& newFaces);

//Takes an edge with two adjacent faces comprising a quad, and replaces the edge with the other diagonal of the quad.
//Not really well-defined in the non-manifold case. Orientation maintained if original face orientations matched.
EdgeHandle flipEdge(TopologicalObject& obj, const EdgeHandle& h);

bool getEdgeOppositeVertices(const TopologicalObject& obj, const EdgeHandle& eh, VertexHandle& v0, VertexHandle& v1);

bool getFaceThirdVertex(const TopologicalObject& obj, const FaceHandle& fh, const EdgeHandle&eh, VertexHandle& vertex);

bool isVertexOnBoundary(TopologicalObject& obj, VertexHandle& v);

void sanityCheckTopology(TopologicalObject& obj);

FaceHandle getEdgeOtherFace(const TopologicalObject& obj, const EdgeHandle& eh, const FaceHandle& fh);

EdgeHandle findEdge( const TopologicalObject& obj, const VertexHandle& v0, const VertexHandle& v1 );
//bool faceExists(const EdgeHandle& e0, const EdgeHandle& e1, const EdgeHandle& e2)
//bool faceExists(const VertexHandle& v0, const VertexHandle& v1, const VertexHandle& v2)

void tearEdge(TopologicalObject& obj,const EdgeHandle& e, const VertexHandle &va, const VertexHandle & vb,
        VertexHandle & newVerta, VertexHandle & newVertb, std::vector<FaceHandle> &newFaces,
        std::vector<FaceHandle> &facesToDelete, std::vector<EdgeHandle> &edgesToDelete);
void tearVertexAlong(TopologicalObject& obj,const EdgeHandle& e, const VertexHandle &va,
        VertexHandle & newVert, std::vector<FaceHandle> &newFaces,
        std::vector<FaceHandle> &facesToDelete, std::vector<EdgeHandle> &edgesToDelete);
void addPrevSide(TopologicalObject & obj, const FaceHandle &f, const EdgeHandle &e,
        const VertexHandle& pivot, VertexHandle &newVert, std::vector<EdgeHandle> & oldEdges,
        std::vector<FaceHandle> & oldFaces, std::vector<FaceHandle> &newFaces);
void addNextSide(TopologicalObject & obj, const FaceHandle &f, const EdgeHandle &e,
        const VertexHandle& pivot, VertexHandle &newVert, std::vector<EdgeHandle> & oldEdges,
        std::vector<FaceHandle> & oldFaces, std::vector<FaceHandle> &newFaces);

} //namespace BASim 

#endif //TOPOBJUTIl_H
