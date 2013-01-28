// ---------------------------------------------------------
//
//  facesplitter.h
//  Christopher Batty 2013
//  
//  Functions supporting the "face split" operation: subdivide face into three triangles
//
// ---------------------------------------------------------

#ifndef EL_TOPO_FACESPLITTER_H
#define EL_TOPO_FACESPLITTER_H

// ---------------------------------------------------------
//  Nested includes
// ---------------------------------------------------------

#include <cstddef>
#include <vector>

// ---------------------------------------------------------
//  Forwards and typedefs
// ---------------------------------------------------------

namespace ElTopo {

class SurfTrack;
template<unsigned int N, class T> struct Vec;
typedef Vec<3,double> Vec3d;
typedef Vec<2,size_t> Vec2st;
typedef Vec<3,size_t> Vec3st;

// ---------------------------------------------------------
//  Class definitions
// ---------------------------------------------------------

// ---------------------------------------------------------
///
/// Edge splitter object.  Splits "long" edges by introducing a new vertex at the midpoint, optionally offsetting for curvature 
/// preservation.
///
// ---------------------------------------------------------

class FaceSplitter
{
    
public:
    
    /// Constructor
    ///
   FaceSplitter( SurfTrack& surf );
        
private:
    
    /// The mesh this object operates on
    /// 
    SurfTrack& m_surf;   
    
    
    /// Check collisions between the edge [neighbour, new] and the given edge 
    ///
    bool split_edge_edge_collision(size_t neighbour_index, 
                                   const Vec3d& new_vertex_position, 
                                   const Vec3d& new_vertex_smooth_position, 
                                   const Vec2st& edge );
    
    /// Determine if the new vertex introduced by the edge split has a collision along its pseudo-trajectory.
    ///
    bool split_triangle_vertex_collision( const Vec3st& triangle_indices, 
                                         const Vec3d& new_vertex_position, 
                                         const Vec3d& new_vertex_smooth_position, 
                                         size_t overlapping_vert_index, 
                                         const Vec3d& vert );
    
    /// Determine if the pseudo-trajectory of the new vertex has a collision with the existing mesh.
    ///
    bool split_face_pseudo_motion_introduces_intersection( const Vec3d& new_vertex_position, 
      const Vec3d& new_vertex_smooth_position, size_t face);
     

    /// Determine if face should be allowed to be split
    ///    
    bool face_is_splittable( size_t face_index );
    
    /// Split an face, using the barycenter point
    ///
    bool split_face( size_t face );
    
};

}

#endif //EL_TOPO_FACESPLITTER_H