#include "TopObjUtil.hh"
#include <set>

namespace BASim {

VertexHandle getSharedVertex(const TopologicalObject& obj, const EdgeHandle& e0, const EdgeHandle& e1) {
  VertexHandle v0 = obj.fromVertex(e0), v1 = obj.toVertex(e0),
    v2 = obj.fromVertex(e1), v3 = obj.toVertex(e1);

  if(v0 == v2 || v0 == v3) return v0;
  else if(v1 == v2 || v1 == v3) return v1;
  else return VertexHandle(-1);
}

EdgeHandle getSharedEdge(const TopologicalObject& obj, const FaceHandle& f0, const FaceHandle& f1) {
  //search for shared edge betwixt the two
  for(FaceEdgeIterator fit = obj.fe_iter(f0); fit; ++fit) {
    EdgeHandle e_out = *fit;
    for(FaceEdgeIterator fit2 = obj.fe_iter(f1); fit2; ++fit2) {
      EdgeHandle e_in = *fit2;
      if(e_out == e_in)
        return e_out;
    }
  }

  return EdgeHandle(-1);
}

VertexHandle getEdgesOtherVertex(const TopologicalObject& obj, const EdgeHandle &eh, const VertexHandle& vh) {
  VertexHandle v0 = obj.fromVertex(eh), v1 = obj.toVertex(eh);
  assert(v0 == vh || v1 == vh);
  return v0 == vh? v1 : v0;
}

FaceHandle getEdgeOtherFace(const TopologicalObject& obj, const EdgeHandle& eh, const FaceHandle& fh) {
  EdgeFaceIterator efit = obj.ef_iter(eh);
  for(; efit; ++efit) {
    FaceHandle cur = *efit;
    if(cur != fh)
      return cur;
  }
  return FaceHandle(-1);
}

TetHandle  getFaceOtherTet (const TopologicalObject& obj, const FaceHandle& fh, const TetHandle& th) {
  FaceTetIterator ftit = obj.ft_iter(fh);
  for(; ftit; ++ftit) {
    TetHandle cur = *ftit;
    if(cur != th)
      return cur;
  }
  return TetHandle(-1);
}

bool getEdgeFacePair(const TopologicalObject& obj, const EdgeHandle& eh, FaceHandle& f0, FaceHandle& f1) {

  EdgeFaceIterator ef_it = obj.ef_iter(eh);
  if(!ef_it) return false; //if there is no face at all
  f0 = *ef_it;
  ++ef_it;
  if(!ef_it) return false; //there is no 2nd face, we also can't flip
  f1 = *ef_it;
  ++ef_it;

  assert(f0 != f1); //we should never get the same face
  assert(!ef_it); //this should probably not be used in non-manifold situations
  
  return true;
}

void addPrevSide(TopologicalObject & obj, const FaceHandle &f, const EdgeHandle &e,
        const VertexHandle& pivot, const VertexHandle &newVert, std::vector<EdgeHandle> & oldEdges,
        std::vector<FaceHandle> & oldFaces, std::vector<FaceHandle> &newFaces){
    EdgeHandle curEdge = obj.prevEdge(f, e);
    FaceHandle curFace = f;
    VertexHandle thirdV;
    while ( curEdge.idx() != -1 ){

//       obj.addEdge(newVert, getEdgesOtherVertex(obj, curEdge, pivot));
       oldEdges.push_back(curEdge);

       curFace = getEdgeOtherFace(obj, curEdge, curFace );
       if ( curFace.idx() == -1 ) break;
       getFaceThirdVertex(obj, curFace, curEdge, thirdV);
       newFaces.push_back(obj.addFace(newVert, getEdgesOtherVertex(obj, curEdge, pivot), thirdV ));
       oldFaces.push_back(curFace);

       curEdge = obj.prevEdge ( curFace, curEdge);
    }

}
void addNextSide(TopologicalObject & obj, const FaceHandle &f, const EdgeHandle &e,
        const VertexHandle& pivot, const VertexHandle &newVert, std::vector<EdgeHandle> & oldEdges,
        std::vector<FaceHandle> & oldFaces, std::vector<FaceHandle> &newFaces){
    assert (obj.fromVertex(e) == pivot || obj.toVertex(e) == pivot);
    EdgeHandle curEdge = obj.nextEdge(f, e);
    FaceHandle curFace = f;
    VertexHandle thirdV;
    
    while ( curEdge.idx() != -1 ){

        //TODO:instead of adding edges, just move them
//        std::cout << obj.ne();
//       obj.addEdge(newVert, getEdgesOtherVertex(obj, curEdge, pivot));
//       std::cout << " - " << obj.ne() << std::endl;
       oldEdges.push_back(curEdge);

       curFace = getEdgeOtherFace(obj, curEdge, curFace );
       if ( curFace.idx() == -1 ) break;
       getFaceThirdVertex(obj, curFace, curEdge, thirdV);
       VertexHandle otherV = getEdgesOtherVertex(obj, curEdge, pivot);
       
       FaceHandle newFace = obj.addFace(newVert, thirdV, otherV);
       newFaces.push_back(newFace);
       oldFaces.push_back(curFace);

       curEdge = obj.nextEdge ( curFace, curEdge);
    }

}
void tearVertexAlong(TopologicalObject& obj,const EdgeHandle& e, const VertexHandle &va,
        VertexHandle & newVert, std::vector<FaceHandle> &newFaces,
        std::vector<FaceHandle> &facesToDelete, std::vector<EdgeHandle> &edgesToDelete){

    assert ( va == obj.fromVertex(e) || va == obj.toVertex(e));
    assert ( obj.edgeIncidentFaces(e) == 2);

    //(va, vb) edge will remain on the top, the new verts will make the bottom, that is, f1

    //Figure out the orientation of the faces
    EdgeFaceIterator efit = obj.ef_iter(e);
    FaceHandle f1 = *efit;
    EdgeHandle next = obj.nextEdge(*efit, e);
    if ( getSharedVertex(obj, e, next) != va){
        ++efit;
    }
    f1 = *efit;

    VertexHandle other = getEdgesOtherVertex(obj, e, va);

    //Add one new vert
    newVert = obj.addVertex();

    //Accumulators to know what to delete at the end
    //Add the first edge and faces
//    EdgeHandle newEdge = obj.addEdge(newVert, other);
    VertexHandle thirdV;
    getFaceThirdVertex(obj, f1, e, thirdV);

    facesToDelete.push_back(f1);
    newFaces.push_back(obj.addFace(other, newVert, thirdV));

    //Add in all the walkable directions to the new verts
    addNextSide(obj, f1, e, va, newVert, edgesToDelete, facesToDelete, newFaces);

}
void tearInteriorEdge(TopologicalObject& obj,const EdgeHandle& e, const VertexHandle &va, const VertexHandle & vb,
        std::vector<VertexHandle> &newVerts,
        std::vector<FaceHandle> &newFaces,
        std::vector<FaceHandle> &facesToDelete,
        std::vector<EdgeHandle> &edgesToDelete){
    assert ( obj.edgeExists(e));
    assert ( obj.edgeIncidentFaces(e) == 2);
    assert ( va == obj.fromVertex(e) && vb == obj.toVertex(e));
    assert ( !obj.isBoundary(va));
    assert ( !obj.isBoundary(vb));
    EdgeFaceIterator efit = obj.ef_iter(e);
    FaceHandle f1 = *efit;
    ++efit;
    FaceHandle f2 = *efit;

    VertexHandle vc, vd;
    getFaceThirdVertex(obj, f1, e, vc);
    getFaceThirdVertex(obj, f2, e, vd);

    //Add the new vertices: 4 in total
    newVerts.push_back ( obj.addVertex() );
    newVerts.push_back ( obj.addVertex() );
    newVerts.push_back ( obj.addVertex() );
    newVerts.push_back ( obj.addVertex() );

    //Add the faces: 4 in total; this will also add the edges
    if(obj.getRelativeOrientation(f1, e) == 1) {
       newFaces.push_back ( obj.addFace(va, newVerts[0], vc) );//First the two faces corresponding to f1
       newFaces.push_back ( obj.addFace(vc, newVerts[1], vb) );
    }
    else {
       newFaces.push_back ( obj.addFace(va, vc, newVerts[0]) );//First the two faces corresponding to f1
       newFaces.push_back ( obj.addFace(vc, vb, newVerts[1]) );
    }
    if(obj.getRelativeOrientation(f2, e) == -1) {
       newFaces.push_back ( obj.addFace(vb, newVerts[2], vd) );//Then the ones corresponding to f2
       newFaces.push_back ( obj.addFace(vd, newVerts[3], va) );
    }
    else {
       newFaces.push_back ( obj.addFace(vb, vd, newVerts[2]) );//Then the ones corresponding to f2
       newFaces.push_back ( obj.addFace(vd, va, newVerts[3]) );
    }

    //Store in the accum lists what will be deleted and what was added
    edgesToDelete.push_back(e); //This edge must be deleted
    //Both original faces
    facesToDelete.push_back(f1);
    facesToDelete.push_back(f2);

}
void tearEdge(TopologicalObject& obj,const EdgeHandle& e, const VertexHandle &va, const VertexHandle & vb,
        VertexHandle & newVerta, VertexHandle & newVertb, std::vector<FaceHandle> &newFaces,
        std::vector<FaceHandle> &facesToDelete, std::vector<EdgeHandle> &edgesToDelete){
    assert ( obj.edgeExists(e));
    assert ( obj.edgeIncidentFaces(e) == 2);
    assert ( va == obj.fromVertex(e) && vb == obj.toVertex(e));

    //(va, vb) edge will remain on the top, the new verts will make the bottom, that is, f1

    //Figure out the orientation of the faces
    EdgeFaceIterator efit = obj.ef_iter(e);
    FaceHandle f1;
    FaceHandle f2;
    EdgeHandle next = obj.nextEdge(*efit, e);
    if ( getSharedVertex(obj, e, next) == va){
        f1 = *efit;
        ++efit;
        f2 = *efit;
    }
    else{
        f2 = *efit;
        ++efit;
        f1 = *efit;
    }

    //Add the new verts
    newVerta = obj.addVertex();
    newVertb = obj.addVertex();

    //Accumulators to know what to delete at the end
    //Add the first edge and faces
    //Note: no need to add the edge when adding a face
//    EdgeHandle newEdge = obj.addEdge(newVerta, newVertb);
    
    VertexHandle thirdV;
    getFaceThirdVertex(obj, f1, e, thirdV);

    facesToDelete.push_back(f1);
    newFaces.push_back(obj.addFace(newVertb, newVerta, thirdV));


    //Add in all the walkable directions to the new verts
    addNextSide(obj, f1, e, va, newVerta, edgesToDelete, facesToDelete, newFaces);
    addPrevSide(obj, f1, e, vb, newVertb, edgesToDelete, facesToDelete, newFaces);


    //Deletion defered to caller so that attributes can be copied
//    //Now delete all the extra things
//    // -all faces that were added to the newverts
//    // -all edges that were added to the newverts
//    for (int i = 0; i < (int)facesToDelete.size(); ++i){
//        obj.deleteFace(facesToDelete[i], false);
//    }
//    for (int i = 0; i < (int)edgesToDelete.size(); ++i){
//        obj.deleteEdge(edgesToDelete[i], false);
//    }


}


VertexHandle splitEdge(TopologicalObject& obj, const EdgeHandle& splitEdge, std::vector<FaceHandle>& newFaces) {
  EdgeFaceIterator ef_iter = obj.ef_iter(splitEdge);

  newFaces.clear();

  //get the edge's vertices
  VertexHandle from_vh, to_vh;
  from_vh = obj.fromVertex(splitEdge);
  to_vh = obj.toVertex(splitEdge);

  //add a new midpoint vertex
  VertexHandle newVert = obj.addVertex();

  //add the two new edges that are part of the original split edge
  EdgeHandle e_0 = obj.addEdge(from_vh, newVert);
  EdgeHandle e_1 = obj.addEdge(to_vh, newVert);

  //now iterate over the existing faces, splitting them in two appropriately
  std::vector<FaceHandle> facesToDelete;
  EdgeFaceIterator ef_it = obj.ef_iter(splitEdge);
  for(;ef_it; ++ef_it) {
    const FaceHandle& fh = *ef_it;

    //store this for deletion later.
    facesToDelete.push_back(fh);

    //find the other vertex in the first face
    FaceVertexIterator fv_it = obj.fv_iter(fh);
    while((*fv_it == from_vh) || (*fv_it == to_vh)) ++fv_it;
    VertexHandle other_vh = *fv_it;

    //create the new edge that splits this face
    EdgeHandle e_faceSplit = obj.addEdge(other_vh, newVert);

    //build the two new faces
    FaceEdgeIterator fe_it = obj.fe_iter(fh);
    for(;fe_it; ++fe_it) {
      EdgeHandle cur = *fe_it;

      //for each edge that isn't the splitEdge...      
      if(cur == splitEdge) continue;

      //accumulate a list of the new edges for the associated new face.
      //it is done in this manner to ensure new orientation is consistent with the old
      std::vector<EdgeHandle> edgeList;

      //determine which half of the splitEdge to use for this new face
      EdgeHandle halfEdge = obj.fromVertex(cur) == from_vh || obj.toVertex(cur) == from_vh? e_0 : e_1;

      FaceEdgeIterator fe_it2 = obj.fe_iter(fh);
      for(;fe_it2; ++fe_it2) {
        EdgeHandle cur2 = *fe_it2;
        if(cur2 == cur) edgeList.push_back(cur); //the current edge
        else if(cur2 == splitEdge) edgeList.push_back(halfEdge); //half of the original split edge
        else edgeList.push_back(e_faceSplit); //the new edge that splits the face
      }
      FaceHandle newFace = obj.addFace(edgeList[0], edgeList[1], edgeList[2]);
      newFaces.push_back(newFace);
    }

  }

  //Delete the previous faces and edge. Done as a post-process so as not to mess up the iterators.
  for(unsigned int i = 0; i < facesToDelete.size(); ++i)
    obj.deleteFace(facesToDelete[i], false);
  obj.deleteEdge(splitEdge, false);
  
  //Return a handle to the vertex we created
  return newVert;
}


EdgeHandle flipEdge(TopologicalObject& obj, const EdgeHandle& eh, const FaceHandle& fh, const FaceHandle& fh2,
  FaceHandle& fNew1, FaceHandle& fNew2) {
  assert(obj.edgeExists(eh));

  VertexHandle from_vh, to_vh;
  from_vh = obj.fromVertex(eh);
  to_vh = obj.toVertex(eh);

  assert(from_vh != to_vh); //the edge should not be degenerate
  
  assert(fh != fh2); //we shouldn't be trying to flip with the same face twice.
  
  //find the 3rd vertex in the first face
  FaceVertexIterator fv_it = obj.fv_iter(fh);
  while((*fv_it == from_vh) || (*fv_it == to_vh)) ++fv_it;
  VertexHandle f1_vh = *fv_it;

  //find the 3rd vertex in the second face
  FaceVertexIterator fv_it2 = obj.fv_iter(fh2);
  while((*fv_it2 == from_vh) || (*fv_it2 == to_vh)) ++fv_it2;
  VertexHandle f2_vh = *fv_it2;

  assert(f1_vh != f2_vh);

  assert(from_vh != f1_vh);
  assert(from_vh != f2_vh);
  assert(to_vh != f1_vh);
  assert(to_vh != f2_vh);

  //check for an edge already matching this description, and possibly use it.
  EdgeHandle existingEdge = findEdge(obj, f1_vh, f2_vh);

  EdgeHandle flippedEdge;
  if(existingEdge.isValid())
    flippedEdge = existingEdge;
  else
    flippedEdge = obj.addEdge(f1_vh, f2_vh);

  //grab all the current edges in proper order, starting from the shared face
  EdgeHandle e0 = obj.nextEdge(fh, eh);
  EdgeHandle e1 = obj.nextEdge(fh, e0);

  EdgeHandle e2 = obj.nextEdge(fh2, eh);
  EdgeHandle e3 = obj.nextEdge(fh2, e2);

  assert(e0 != e1);
  assert(e0 != e2);
  assert(e0 != e3);
  assert(e0 != eh);
  assert(e0 != flippedEdge);

  assert(e1 != e2);
  assert(e1 != e3);
  assert(e1 != eh);
  assert(e1 != flippedEdge);

  assert(e2 != e3);
  assert(e2 != eh);
  assert(e2 != flippedEdge);

  assert(e3 != eh);
  assert(e3 != flippedEdge);

  assert(eh != flippedEdge);

  //flip the edges of the second face so it matches the first face 
  //(if the original faces didn't sync, it doesn't matter, since there's no way to flip and maintain consistency)
  VertexHandle sharedV = getSharedVertex(obj, e1, e2);
  if(!sharedV.isValid())
    std::swap(e2,e3);

  //add in the new faces
  fNew1 = obj.addFace(e1, e2, flippedEdge);
  fNew2 = obj.addFace(e3, e0, flippedEdge);

  //Delete the old patch
  bool success = obj.deleteFace(fh, false);
  assert(success);
  success = obj.deleteFace(fh2, false);
  assert(success);
  success = obj.deleteEdge(eh, false);
  assert(success);

  return flippedEdge;
}


//check if a vertex is on the boundary of a simplex mesh.
//We define this by the fact that one of its' edges doesn't have
//two faces attached.
bool isVertexOnBoundary(TopologicalObject& obj, VertexHandle& v) {
  VertexEdgeIterator ve_iter = obj.ve_iter(v);
  for(;ve_iter; ++ve_iter) {
    FaceHandle f0, f1;
    bool success = getEdgeFacePair(obj, *ve_iter, f0, f1);
    if(!success)
      return true;
  }
  return false;
}


bool getEdgeOppositeVertices(const TopologicalObject& obj, const EdgeHandle& eh, VertexHandle& v0, VertexHandle& v1) {
  FaceHandle f0,f1;
  bool success = getEdgeFacePair(obj, eh, f0, f1);
  
  if(!success) //there is only one other vertex
    return false;

  VertexHandle edgeV0 = obj.fromVertex(eh);
  VertexHandle edgeV1 = obj.toVertex(eh);
  
  for(FaceVertexIterator fv_it = obj.fv_iter(f0); fv_it; ++fv_it) {
    VertexHandle cur = *fv_it;
    if(cur != edgeV0 && cur != edgeV1)
      v0 = cur;
  }
  for(FaceVertexIterator fv_it = obj.fv_iter(f1); fv_it; ++fv_it) {
    VertexHandle cur = *fv_it;
    if(cur != edgeV0 && cur != edgeV1)
      v1 = cur;
  }
  return true;
}

bool getFaceThirdVertex(const TopologicalObject& obj, const FaceHandle& fh, const EdgeHandle&eh, VertexHandle& vertex) {
   //assumes edge eh if one of the edges of face fh
   for(FaceVertexIterator fvit = obj.fv_iter(fh); fvit; ++fvit) {
      if(*fvit  != obj.fromVertex(eh) && *fvit != obj.toVertex(eh)) {
         vertex = *fvit;
         return true;
      }
   }
   return false;
}

bool getTetFourthVertex(const TopologicalObject& obj, const TetHandle& th, const FaceHandle&fh, VertexHandle& vertex) {
  
  FaceVertexIterator fvit = obj.fv_iter(fh); assert(fvit);
  VertexHandle v0 = *fvit; ++fvit; assert(fvit);
  VertexHandle v1 = *fvit; ++fvit; assert(fvit);
  VertexHandle v2 = *fvit; ++fvit; assert(!fvit);
  
  //assumes face fh if one of the faces of tet th
  for(TetVertexIterator tvit = obj.tv_iter(th); tvit; ++tvit) {
    if(*tvit != v0 && *tvit != v1 && *tvit != v2) {
      vertex = *tvit;
      return true;
    }
  }
  return false;
}

void sanityCheckTopology(TopologicalObject& obj) {
  
  for(EdgeIterator e_it = obj.edges_begin(); e_it != obj.edges_end(); ++e_it) {
    assert(obj.fromVertex(*e_it) != obj.toVertex(*e_it));
    assert(obj.edgeExists(*e_it));
    assert(obj.vertexExists(obj.fromVertex(*e_it)));
    assert(obj.vertexExists(obj.toVertex(*e_it)));

    FaceHandle f0, f1;
    bool success = getEdgeFacePair(obj, *e_it, f0, f1);
    if(success) {
      //count the number of unique edges
      std::set<EdgeHandle> edges;
      for(FaceEdgeIterator fe_it = obj.fe_iter(f0); fe_it; ++fe_it) edges.insert(*fe_it);
      for(FaceEdgeIterator fe_it = obj.fe_iter(f1); fe_it; ++fe_it) edges.insert(*fe_it);
      assert(edges.size() == 5);

      VertexHandle v0, v1;
      assert(f0 != f1);
      bool success2 = getEdgeOppositeVertices(obj, *e_it, v0, v1);
      if(success2) {
        assert(v0 != v1);
      }
    }

  }

  for(FaceIterator f_it = obj.faces_begin(); f_it != obj.faces_end(); ++f_it) {
    std::vector<VertexHandle> verts;
    for(FaceVertexIterator fv_it = obj.fv_iter(*f_it); fv_it; ++fv_it) {
      verts.push_back(*fv_it);

    }
    assert(verts.size() == 3);
    assert(verts[0] != verts[1] && verts[0] != verts[2]);
    
    std::vector<EdgeHandle> edges;
    for(FaceEdgeIterator fe_it = obj.fe_iter(*f_it); fe_it; ++fe_it) {
      edges.push_back(*fe_it);
    }
    assert(edges.size() == 3);
    assert(edges[0] != edges[1] && edges[0] != edges[2]);

    //check for other faces with the same edges
    for(FaceEdgeIterator fe_it = obj.fe_iter(*f_it); fe_it; ++fe_it) {
      EdgeHandle edge = *fe_it;
      FaceHandle f0, f1;
      bool success = getEdgeFacePair(obj, edge, f0, f1);
      if(!success) continue;
      assert(f0 == *f_it || f1 == *f_it);
      assert(f0 != f1);
      VertexHandle v0, v1;
      success = getEdgeOppositeVertices(obj, edge, v0, v1);
      if(!success) continue;
      assert(v0 != v1);
    }

  }

}

EdgeHandle findEdge( const TopologicalObject& obj, const VertexHandle& v0, const VertexHandle& v1 ) {
  for(VertexEdgeIterator ve_it = obj.ve_iter(v0); ve_it; ++ve_it) {
    EdgeHandle eh = *ve_it;
    if(obj.fromVertex(eh) == v1 || obj.toVertex(eh) == v1)
      return eh;
  }
  return EdgeHandle(-1);
}

FaceHandle findFace( const TopologicalObject& obj, const VertexHandle& v0, const VertexHandle& v1, const VertexHandle& v2 )
{
  std::set<VertexHandle> verts;
  verts.insert(v0);
  verts.insert(v1);
  verts.insert(v2);
  assert(verts.size() == 3);
  for (VertexFaceIterator vfit = obj.vf_iter(v0); vfit; ++vfit)
  {
    FaceVertexIterator fvit = obj.fv_iter(*vfit); assert(fvit);
    if (verts.find(*fvit) == verts.end()) continue;
    ++fvit; assert(fvit);
    if (verts.find(*fvit) == verts.end()) continue;
    ++fvit; assert(fvit);
    if (verts.find(*fvit) == verts.end()) continue;
    ++fvit; assert(!fvit);
    return *vfit;
  }
  return FaceHandle();
}
  
FaceHandle findFace( const TopologicalObject& obj, const EdgeHandle& e0,   const EdgeHandle& e1,   const EdgeHandle& e2 )
{
  std::set<EdgeHandle> edges;
  edges.insert(e0);
  edges.insert(e1);
  edges.insert(e2);
  assert(edges.size() == 3);
  for (EdgeFaceIterator efit = obj.ef_iter(e0); efit; ++efit)
  {
    FaceEdgeIterator feit = obj.fe_iter(*efit); assert(feit);
    if (edges.find(*feit) == edges.end()) continue;
    ++feit; assert(feit);
    if (edges.find(*feit) == edges.end()) continue;
    ++feit; assert(feit);
    if (edges.find(*feit) == edges.end()) continue;
    ++feit; assert(!feit);
    return *efit;
  }
  return FaceHandle();
}

TetHandle  findTet ( const TopologicalObject& obj, const VertexHandle& v0, const VertexHandle& v1, const VertexHandle& v2, const VertexHandle& v3 )
{
  std::set<VertexHandle> verts;
  verts.insert(v0);
  verts.insert(v1);
  verts.insert(v2);
  verts.insert(v3);
  assert(verts.size() == 4);
  for (VertexFaceIterator vfit = obj.vf_iter(v0); vfit; ++vfit)
  {
    for (FaceTetIterator ftit = obj.ft_iter(*vfit); ftit; ++ftit)
    {
      TetVertexIterator tvit = obj.tv_iter(*ftit); assert(tvit);
      if (verts.find(*tvit) == verts.end()) continue;
      ++tvit; assert(tvit);
      if (verts.find(*tvit) == verts.end()) continue;
      ++tvit; assert(tvit);
      if (verts.find(*tvit) == verts.end()) continue;
      ++tvit; assert(tvit);
      if (verts.find(*tvit) == verts.end()) continue;
      ++tvit; assert(!tvit);
      return *ftit;
    }
  }
  return TetHandle();
}

TetHandle  findTet ( const TopologicalObject& obj, const FaceHandle& f0,   const FaceHandle& f1,   const FaceHandle& f2,   const FaceHandle& f3 )
{
  std::set<FaceHandle> faces;
  faces.insert(f0);
  faces.insert(f1);
  faces.insert(f2);
  faces.insert(f3);
  assert(faces.size() == 4);
  for (FaceTetIterator ftit = obj.ft_iter(f0); ftit; ++ftit)
  {
    TetFaceIterator tfit = obj.tf_iter(*ftit); assert(tfit);
    if (faces.find(*tfit) == faces.end()) continue;
    ++tfit; assert(tfit);
    if (faces.find(*tfit) == faces.end()) continue;
    ++tfit; assert(tfit);
    if (faces.find(*tfit) == faces.end()) continue;
    ++tfit; assert(tfit);
    if (faces.find(*tfit) == faces.end()) continue;
    ++tfit; assert(!tfit);
    return *ftit;
  }
  return TetHandle();
}

bool isFaceMatch(const TopologicalObject& obj, const FaceHandle& fh, const VertexHandle& v0, const VertexHandle& v1, const VertexHandle& v2) {
  //This could probably be done more optimally internally to TopologicalObject
  std::vector<VertexHandle> src_vh;
  src_vh.push_back(v0);src_vh.push_back(v1);src_vh.push_back(v2);
  std::vector<VertexHandle> face_vh(3);
  int i = 0;
  for(FaceVertexIterator fvit = obj.fv_iter(fh); fvit; ++fvit) {
    face_vh[i] = *fvit;
    ++i;
  }
  std::sort(face_vh.begin(), face_vh.end());
  std::sort(src_vh.begin(), src_vh.end());
  
  return face_vh[0] == src_vh[0] && face_vh[1] == src_vh[1] && face_vh[2] == src_vh[2];
}
  
bool faceContainsVertex(const TopologicalObject & obj, const FaceHandle & fh, const VertexHandle & vh)
{
  for (FaceVertexIterator fvit = obj.fv_iter(fh); fvit; ++fvit)
    if (vh == *fvit)
      return true;
  return false;
}

bool faceContainsEdge(const TopologicalObject & obj, const FaceHandle & fh, const EdgeHandle & eh)
{
  for (FaceEdgeIterator feit = obj.fe_iter(fh); feit; ++feit)
    if (eh == *feit)
      return true;
  return false;
}

bool tetContainsVertex(const TopologicalObject & obj, const TetHandle & th, const VertexHandle & vh)
{
  for (TetVertexIterator tvit = obj.tv_iter(th); tvit; ++tvit)
    if (vh == *tvit)
      return true;
  return false;
}

FaceHandle getVertexOppositeFaceInTet(const TopologicalObject & obj, const TetHandle & th, const VertexHandle & vh)
{
  for (TetFaceIterator tfit = obj.tf_iter(th); tfit; ++tfit)
  {
    FaceVertexIterator fvit = obj.fv_iter(*tfit); assert(fvit);
    VertexHandle v0 = *fvit; ++fvit; assert(fvit);
    VertexHandle v1 = *fvit; ++fvit; assert(fvit);
    VertexHandle v2 = *fvit; ++fvit; assert(!fvit);
    
    if (v0 != vh && v1 != vh && v2 != vh)
      return *tfit;
  }
  
  return FaceHandle();
}

EdgeHandle getVertexOppositeEdgeInFace(const TopologicalObject & obj, const FaceHandle & fh, const VertexHandle & vh)
{
  for (FaceEdgeIterator feit = obj.fe_iter(fh); feit; ++feit)
    if (obj.fromVertex(*feit) != vh && obj.toVertex(*feit) != vh)
      return *feit;
  
  return EdgeHandle();
}

bool isVertexManifold(const TopologicalObject & obj, const VertexHandle & v)
{
  if (obj.nf() == 0)  // top dimension = 1, manifold = 1d manifold
  {
    return obj.vertexIncidentEdges(v) == 2 || obj.vertexIncidentEdges(v) == 1;
    
  } else  // top dimension = 2/3
  {
    if (obj.vertexIncidentEdges(v) == 0)
      return false;
    
    int ebcount = 0;
    for (VertexEdgeIterator veit = obj.ve_iter(v); veit; ++veit)
    {
      if (!isEdgeManifold(obj, *veit))
        return false;
      if (obj.edgeIncidentFaces(*veit) == 1)
        ebcount++;    // edge boundary
    }
    return ebcount == 2 || ebcount == 0;    
  }
}

bool isEdgeManifold(const TopologicalObject & obj, const EdgeHandle & e)
{
  if (obj.nt() == 0)  // top dimension = 2, manifold = 2d manifold
  {
    return obj.edgeIncidentFaces(e) == 2 || obj.edgeIncidentFaces(e) == 1;
    
  } else  // top dimension = 3, manifold = 3d manifold
  {
    if (obj.edgeIncidentFaces(e) == 0)
      return false;
    
    int fbcount = 0;
    for (EdgeFaceIterator efit = obj.ef_iter(e); efit; ++efit)
    {
      if (!isFaceManifold(obj, *efit))
        return false;
      if (obj.faceIncidentTets(*efit) == 1)
        fbcount++;    // face boundary
    }
    return fbcount == 2 || fbcount == 0;
  }
}

bool isFaceManifold(const TopologicalObject & obj, const FaceHandle & f)
{
  return obj.faceIncidentTets(f) == 2 || obj.faceIncidentTets(f) == 1;
}

}
