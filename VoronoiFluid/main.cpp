
#include "parser.h"

#include <bfstream.h>
#include "dualfluidsim3d.h"
#include "geometryinit.h"
#include "geometryutils.h"
#include "gluvi.h"
#include "iomesh.h"
#include "marching_tiles_hires.h"
#include "meancurvature.h"
#include "sampleseeder.h"
#include "tetmesh.h"
#include "tetmeshio.h"
#include "wallclocktime.h"

#include <subdivisionscheme.h>

//#include "mesh3dwrapper.h"

#ifndef WIN32
#include <sys/stat.h>
#endif

#include <pthread.h>

using namespace ElTopo;

// ---------------------------------------------------------
//
// Globals
//
// ---------------------------------------------------------

// Global fluid sim object
DualFluidSim3D* g_dual_sim = NULL;

// Buffers for rendering (sim data is copied into these buffers after each frame is computed)
std::vector<Vec3d> g_renderable_vertices;
std::vector<Vec3d> g_renderable_vertex_normals;
std::vector<float> g_renderable_vertex_curvatures;

std::vector<Vec3ui> g_renderable_triangles;
std::vector<Vec2ui> g_renderable_edges;
TetMesh* g_renderable_tet_mesh;
std::vector<float> g_renderable_liquid_phi;

Gluvi::Target3D* cam; 
Gluvi::DynamicText* status_text_widget;

float g_default_cam_target[3];
float g_default_cam_dist;
float g_default_cam_heading;
float g_default_cam_pitch;

float clipping_plane_distance = 2.5f;

bool g_draw_tet_vertices = false;
bool g_draw_tet_edges = false;
bool g_draw_tet_faces = false;
bool g_draw_tet_circumcentres = false;
bool g_draw_clipping_plane = false;
bool g_draw_marker_particles = false;
bool g_draw_voronoi_areas = false;
bool g_draw_solid_circumcentres = false;
bool g_draw_solid_box = true;
bool g_draw_domain_box = true;
bool g_draw_liquid_phi = false;
bool g_draw_explicit_surface = true;
bool g_draw_surface_edges = false;
bool g_draw_surface_vertices = false;
bool g_draw_valid_tet_vertices = false;
bool g_draw_velocities = false;
bool g_display_status_text = true;

bool g_running = false;
bool g_advance_single_frame = false;
bool g_rendering_sequence = false;
unsigned int g_frame = 0;

bool seed_samples_at_solid = true;
extern double g_air_sample_rejection_threshold;

int g_num_surface_substeps = 1;
double g_frame_rate = 30.0;


// TEMP
bool g_null_space_smoothing = true;


Vec3f DOMAIN_LOW(0,0,0);
Vec3f DOMAIN_HIGH(5,5,5);
Vec3f BOX_DOMAIN_LOW( 0.9353f );
Vec3f BOX_DOMAIN_HIGH( 4.1233f, 2.0f, 4.1233f );

double g_svd_rcond = 0.2;

#ifdef __APPLE__   
char g_path[256] = "/Users/tyson/scratch/voronoi3d";
#elif defined WIN32
char g_path[256] = "C:/output";
#else
char g_path[256] = "/var/tmp";
#endif

// TEMP:
std::vector<unsigned int> g_vertex_rank;
std::vector<Vec3d> g_vertex_primary_eigenvectors;


pthread_mutex_t thread_is_running_mutex = PTHREAD_MUTEX_INITIALIZER;
bool thread_is_running = false;
pthread_t advance_frame_thread;
bool waiting_for_thread_to_finish = false;

pthread_mutex_t sim_mutex = PTHREAD_MUTEX_INITIALIZER;

// ---------------------------------------------------------
//
// Function declarations
//
// ---------------------------------------------------------

void update_renderable_objects();

void draw_box( const Vec3f& lo, const Vec3f& hi );
void display();
void mouse(int button, int state, int x, int y);
void drag(int x, int y);

void snap_to_box( const Vec3d& box_low, const Vec3d& box_high, double tol, Vec3d& v );
void snap_to_solid_splash( Vec3d& v );
void snap_to_solid( Vec3d& v );
void load_and_process_splash( char* filename );
void load_and_process_mesh( char* filename );
void load_mesh( );

void keyboard(unsigned char key, int, int );

void restore_frame( unsigned int frame );
void count_triangles( );

void file_output( unsigned int frame );

void* advance_frame_async(void*);
void start_advance_frame();
void advance_frame_done();
void advance_frame( int junk );

void dent_surface( const Vec3d& dent_centre, double dent_radius, double dent_magnitude, std::vector<Vec3d>& surface_vertices );

void parse_script( const char* filename );

int main( int argc, char** argv );

// ---------------------------------------------------------
//
// Function defs
//
// ---------------------------------------------------------

// ---------------------------------------------------------

void update_renderable_objects()
{
   
   // acquire mutex on fluid sim object
   pthread_mutex_lock( &sim_mutex );   
   
   g_renderable_vertices = g_dual_sim->surface_tracker->get_positions();
   g_dual_sim->surface_tracker->get_all_vertex_normals(g_renderable_vertex_normals);
   g_renderable_triangles = g_dual_sim->surface_tracker->m_mesh.m_tris;
   g_renderable_edges = g_dual_sim->surface_tracker->m_mesh.m_edges;
   g_renderable_tet_mesh = new TetMesh(*g_dual_sim->mesh); //make a copy for safe visualization
      
   g_renderable_liquid_phi = g_dual_sim->liquid_phi;
   
   /*

   std::vector<float> vertexCurvatures(g_renderable_vertices.size());
   for(unsigned int i = 0; i < g_renderable_vertices.size(); ++i) {
      //Vec3d surf_normal = g_renderable_vertex_normals[i];

      //Vec3d curvatureNormal;
      //MeanCurvatureDriver::vertex_mean_curvature_normal(i, *(g_dual_sim->surface_tracker), curvatureNormal);
      //vertexCurvatures[i] = -(float)mag(curvatureNormal) * (dot(surf_normal,curvatureNormal) > 0?1:-1);
      
      Vec3d meanCurvatureNormal;
      MeanCurvatureDriver::vertex_mean_curvature_normal(i, *(g_dual_sim->surface_tracker), meanCurvatureNormal);
      Vec3d surfaceNormal = meanCurvatureNormal;
      normalize(surfaceNormal);
      float curvatureValue = dot(meanCurvatureNormal,surfaceNormal);
      vertexCurvatures[i] = curvatureValue;
   }

   g_renderable_vertex_curvatures = vertexCurvatures;

   g_vertex_rank.clear();
   g_vertex_rank.resize( g_dual_sim->surface_tracker->m_positions.size(), 0 );
   g_vertex_primary_eigenvectors.resize( g_dual_sim->surface_tracker->m_positions.size(), Vec3d(0,0,0) );
   for ( unsigned int i = 0; i < g_dual_sim->surface_tracker->m_positions.size(); ++i )
   {
      Vec3d eigenvectors[3];
      g_vertex_rank[i] = g_dual_sim->surface_tracker->classify_vertex( i, eigenvectors );
      g_vertex_primary_eigenvectors[i] = eigenvectors[2];
   }
   */
   
   // release mutex
   pthread_mutex_unlock( &sim_mutex );   
   
}


// ---------------------------------------------------------

void draw_box( const Vec3f& lo, const Vec3f& hi )
{
   glBegin(GL_LINE_STRIP);
   glVertex3f( lo[0], lo[1], lo[2] );
   glVertex3f( hi[0], lo[1], lo[2] );
   glVertex3f( hi[0], lo[1], hi[2] );
   glVertex3f( lo[0], lo[1], hi[2] );
   glVertex3f( lo[0], lo[1], lo[2] );
   glEnd();

   glBegin(GL_LINE_STRIP);
   glVertex3f( lo[0], hi[1], lo[2] );
   glVertex3f( hi[0], hi[1], lo[2] );
   glVertex3f( hi[0], hi[1], hi[2] );
   glVertex3f( lo[0], hi[1], hi[2] );
   glVertex3f( lo[0], hi[1], lo[2] );
   glEnd();
   
   glBegin(GL_LINES);
   glVertex3f( lo[0], lo[1], lo[2] );
   glVertex3f( lo[0], hi[1], lo[2] );
   glVertex3f( hi[0], lo[1], lo[2] );
   glVertex3f( hi[0], hi[1], lo[2] );
   glVertex3f( hi[0], lo[1], hi[2] );
   glVertex3f( hi[0], hi[1], hi[2] );
   glVertex3f( lo[0], lo[1], hi[2] );
   glVertex3f( lo[0], hi[1], hi[2] );
   glEnd();
}
       



// ---------------------------------------------------------

void display()
{

   if ( g_display_status_text )
   {
      pthread_mutex_lock( &thread_is_running_mutex );
      if ( thread_is_running )
      {
         if ( g_running )
         {
            // running async
            status_text_widget->set_color( 0.0f, 0.0f, 0.0f );
            status_text_widget->text = "Running";
         }
         else
         {
            // stopping after the current frame
            status_text_widget->set_color( 0.8f, 0.2f, 0.2f );         
            status_text_widget->text = "Stopping...";
         }
         
      }
      else
      {
         if ( waiting_for_thread_to_finish )
         {
            
            // thread has signalled that it's done
            // wait for the thread to finish and be destroyed
            pthread_join( advance_frame_thread, NULL );
            
            waiting_for_thread_to_finish = false;

            advance_frame_done();
            
            status_text_widget->set_color( 0.0f, 0.0f, 0.0f );
            status_text_widget->text = "Ready";
         }
      }
      pthread_mutex_unlock( &thread_is_running_mutex );
   }
   else
   {
      status_text_widget->set_color( 1.0f, 1.0f, 1.0f );
      status_text_widget->text = "";
   }
   
   /*
   // identify and draw some slivery tets
   std::vector<int> bad_tets;
   
   //collect sliver tetrahedra
   for(int i = 0; i < g_renderable_tet_mesh->tets.size(); ++i) {
      //get it's 4 vertices
      Vec3f verts[4];
      for(int j = 0; j < 4; ++j) {
         verts[j] = g_renderable_tet_mesh->vertices[g_renderable_tet_mesh->tets[i][j]];
      }

      Vec3d x0(verts[0]); Vec3d x1(verts[1]); Vec3d x2(verts[2]); Vec3d x3(verts[3]);
      //compute point-plane distance
      // do it the QR way for added robustness
      Vec3d x13=x1-x3;
      double r00=mag(x13)+1e-30;
      x13/=r00;
      Vec3d x23=x2-x3;
      double r01=dot(x23,x13);
      x23-=r01*x13;
      double r11=mag(x23)+1e-30;
      x23/=r11;
      Vec3d x03=x0-x3;
      double s2=dot(x23,x03)/r11;
      double s1=(dot(x13,x03)-r01*s2)/r00;
      double s3=1-s1-s2;
      // check if we are in range

      double distance=dist(x0, s1*x1+s2*x2+s3*x3);
      if(distance < 1e-3) {
         bad_tets.push_back(i);
      }
   }

   //render sliver tetrahedra
   glBegin(GL_LINES);
   for(int t = 0; t < bad_tets.size(); ++t) {
      int i = bad_tets[t];
      if(g_renderable_tet_mesh->tets.size() <= i) continue;

      Vec6ui edgelist = g_renderable_tet_mesh->tet_to_edge_map[i];
      for(int edge = 0; edge < 6; ++edge) {
         Vec2st edge_data = g_renderable_tet_mesh->edges[edgelist[edge]];
            
         const Vec3f& a = g_renderable_tet_mesh->vertices[ edge_data[0] ];
         const Vec3f& b = g_renderable_tet_mesh->vertices[ edge_data[1] ];
            
         glVertex3fv( a.v );
         glVertex3fv( b.v );
            
      }
      
   }
   glEnd();
   */
   

   // vertices
   
   if ( g_draw_tet_vertices )
   {
      glPointSize( 5.0f );
      glColor3f(1,0,0);
      glBegin(GL_POINTS);
      
      for ( unsigned int i = 0; i < g_renderable_tet_mesh->vertices.size(); ++i )
      {
         if ( g_renderable_tet_mesh->vertices[i][2] < clipping_plane_distance )
         {
            if ( g_draw_liquid_phi )
            {
               /*
               if ( g_renderable_liquid_phi[i] < 0.0f )
               {
                  glColor3f(0,0,1);
                  glVertex3fv( g_renderable_tet_mesh->vertices[i].v );
               }
               else
               {
                  glColor3f(1,0,0);
                  glVertex3fv( g_renderable_tet_mesh->vertices[i].v );
               }
               */
               Vec3f color(0,0,0);
               //std::cout << "Region id = " << g_dual_sim->region_IDs[i] << std::endl;
               if(g_dual_sim->region_IDs[i] < 0 || g_dual_sim->region_IDs[i] > 2)
                  color = Vec3f(1,1,0);
               else
                  color[g_dual_sim->region_IDs[i]] = 1;
               glColor3fv(color.v);
               glVertex3fv( g_renderable_tet_mesh->vertices[i].v );

            }
            else
            {
               glColor3f(0,0,1);
               glVertex3fv( g_renderable_tet_mesh->vertices[i].v );
            }
            
         }         
      }
      glEnd();
            
   }

   // edges
   
   if ( g_draw_tet_edges )
   {
      glColor3f(0,0,0);
      glBegin(GL_LINES);
      for ( unsigned int i = 0; i < g_renderable_tet_mesh->edges.size(); ++i )
      {
         const Vec3f& a = g_renderable_tet_mesh->vertices[ g_renderable_tet_mesh->edges[i][0] ];
         const Vec3f& b = g_renderable_tet_mesh->vertices[ g_renderable_tet_mesh->edges[i][1] ];
         if ( a[2] < clipping_plane_distance && b[2] < clipping_plane_distance )
         {
            if ( g_draw_voronoi_areas )
            {
               glColor3f( g_renderable_tet_mesh->voronoi_face_areas[i], 0, 0 );
            }
            
            if ( g_draw_solid_circumcentres )
            {
            }
            
            if ( g_draw_valid_tet_vertices )
            {
            }
            
            glVertex3fv( a.v );
            glVertex3fv( b.v );
         }
      }
      glEnd();
   }
   
   // triangles
   
   if ( g_draw_tet_faces )
   {
      glDisable(GL_CULL_FACE);
      
      glColor3f(1,1,1);
      glEnable(GL_POLYGON_OFFSET_FILL);
      glPolygonOffset(1.0f, 1.0f);      //  allow the wireframe to show through
      
      for ( unsigned int i = 0; i < g_renderable_tet_mesh->tris.size(); ++i )
      {
         const Vec3f& a = g_renderable_tet_mesh->vertices[ g_renderable_tet_mesh->tris[i][0] ];
         const Vec3f& b = g_renderable_tet_mesh->vertices[ g_renderable_tet_mesh->tris[i][1] ];
         const Vec3f& c = g_renderable_tet_mesh->vertices[ g_renderable_tet_mesh->tris[i][2] ];
         
         if (   a[2] < clipping_plane_distance 
             && b[2] < clipping_plane_distance
             && c[2] < clipping_plane_distance )
         {
            glBegin(GL_POLYGON);
            glVertex3fv( a.v );
            glVertex3fv( b.v );
            glVertex3fv( c.v );
            glVertex3fv( a.v );
            glEnd();
         }
      }
   }
   
   // circumcentres
   
   if ( g_draw_tet_circumcentres )
   {
      glColor3f(0,1,0);
      glBegin(GL_POINTS);
      for ( unsigned int i = 0; i < g_renderable_tet_mesh->tet_circumcentres.size(); ++i )
      {
         if ( g_draw_solid_circumcentres )
         {
         }
         
         if ( g_renderable_tet_mesh->tet_circumcentres[i][2] < clipping_plane_distance )
         {
            glVertex3fv( g_renderable_tet_mesh->tet_circumcentres[i].v );
         }
      }
      glEnd();
   }
   

   if( g_draw_velocities ) 
   {
      
      glColor3f(0,0,1);
      glBegin(GL_LINES);
      for(unsigned int i = 0; i < g_renderable_tet_mesh->vertices.size(); ++i) 
      {
         //if (  )       // check if tet_vertex_velocity_is_valid
         {
            if ( g_renderable_tet_mesh->vertices[i][2] < clipping_plane_distance ) 
            {
               //glVertex3fv(g_renderable_tet_mesh->vertices[i].v);
               //glVertex3fv((g_renderable_tet_mesh->vertices[i] + 0.1f*g_dual_sim->tet_vertex_velocities[i]).v);
            }
         }
      }
      glEnd();
      
      glColor3f(0,1,0);
      glBegin(GL_LINES);
      for(unsigned int i = 0; i < g_renderable_tet_mesh->voronoi_face_centroids.size(); ++i) 
      {
         // if( )       // check if voronoi full face velocity is valid
         {
            if ( g_renderable_tet_mesh->voronoi_face_centroids[i][2] < clipping_plane_distance ) 
            {
               //glVertex3fv(g_renderable_tet_mesh->voronoi_face_centroids[i].v);
               //glVertex3fv((g_renderable_tet_mesh->voronoi_face_centroids[i] + 0.1*g_dual_sim->full_voronoi_face_velocities).v);
            }
         }
      }
      glEnd();
      
      glColor3f(1,0,0);
      glBegin(GL_LINES);
      for(unsigned int i = 0; i < g_renderable_tet_mesh->tet_circumcentres.size(); ++i) 
      {
         //if( )      // check if voronoi_vertex_velocity_is_valid
         {
            if ( g_renderable_tet_mesh->tet_circumcentres[i][2] < clipping_plane_distance ) 
            {
               glVertex3fv(g_renderable_tet_mesh->tet_circumcentres[i].v);
               glVertex3fv((g_renderable_tet_mesh->tet_circumcentres[i] + 0.1f*g_dual_sim->voronoi_vertex_velocities[i]).v);
            }
         }
      }
      glEnd();
      

                  
   }

   // solid box
   
   if ( g_draw_solid_box )
   {
      glColor3f(0,0,0);
      draw_box( BOX_DOMAIN_LOW, BOX_DOMAIN_HIGH );
   }

   if(g_draw_domain_box ) 
   {
      glColor3f(0.5,0.5,0.5);
      draw_box( g_dual_sim->domain_low, g_dual_sim->domain_high );
   }

   // clipping plane
   
   if ( g_draw_clipping_plane )
   {
      glEnable( GL_BLEND );
      glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
      
      glColor4f( 0.5f, 0.5f, 0.5f, 0.4f );
      glBegin(GL_POLYGON);
      glVertex3f( -50.0f, -50.0f, clipping_plane_distance );
      glVertex3f(  50.0f, -50.0f, clipping_plane_distance );
      glVertex3f(  50.0f,  50.0f, clipping_plane_distance );
      glVertex3f( -50.0f,  50.0f, clipping_plane_distance );
      glVertex3f( -50.0f, -50.0f, clipping_plane_distance );
      glEnd();
      
      glDisable( GL_BLEND ); 
   }
   
   // Surface mesh

   if ( g_draw_explicit_surface )
   {
      
#if 0
      glEnableClientState( GL_VERTEX_ARRAY );
      glEnableClientState( GL_NORMAL_ARRAY );
      
      glVertexPointer( 3, GL_DOUBLE, 0, &(g_renderable_vertices[0]) );
      glNormalPointer( GL_DOUBLE, 0, &(g_renderable_vertex_normals[0]) );
    
      // triangles
      
      glEnable(GL_LIGHTING);
      glShadeModel(GL_SMOOTH);
      Gluvi::set_generic_lights();
      Gluvi::set_generic_material(1.0f, 1.0f, 1.0f, GL_FRONT);   // exterior surface colour
      Gluvi::set_generic_material(1.0f, 1.0f, 1.0f, GL_BACK);     
      if ( g_draw_surface_edges )
      {
         glEnable(GL_POLYGON_OFFSET_FILL);
         glPolygonOffset(1.0f, 1.0f);      //  allow the wireframe to show through
      }      
      glDrawElements(GL_TRIANGLES, 3 * g_renderable_triangles.size(), GL_UNSIGNED_INT, &(g_renderable_triangles[0]) );
      glDisable(GL_LIGHTING);

      // edges
      
      if ( g_draw_surface_edges )
      {
         glColor3f( 0.0f, 0.0f, 0.0f );
         glDrawElements( GL_LINES, 2 * g_renderable_edges.size(), GL_UNSIGNED_INT, &(g_renderable_edges[0]) );
      }

      glDisableClientState( GL_NORMAL_ARRAY );
      glDisableClientState( GL_VERTEX_ARRAY );
      
#else
      
      // triangles
      
      glEnable(GL_LIGHTING);
      glShadeModel(GL_SMOOTH);
      Gluvi::set_generic_lights();
      Gluvi::set_generic_material(1.0f, 1.0f, 1.0f, GL_FRONT);   // exterior surface colour
      Gluvi::set_generic_material(1.0f, 1.0f, 1.0f, GL_BACK);     
      if ( g_draw_surface_edges )
      {
         glEnable(GL_POLYGON_OFFSET_FILL);
         glPolygonOffset(1.0f, 1.0f);      //  allow the wireframe to show through
      }      
      
     
      //float divisor = 11;
      glBegin( GL_TRIANGLES );
      for ( unsigned int i = 0; i < g_renderable_triangles.size(); ++i )
      {
         const Vec3ui& tri = g_renderable_triangles[i];
         
         if(g_dual_sim->surface_tracker->m_mesh.get_triangle_label(i)[1] == 1)
            Gluvi::set_generic_material(1, 0, 0, GL_FRONT);   
         else if(g_dual_sim->surface_tracker->m_mesh.get_triangle_label(i)[1] == 2)
            Gluvi::set_generic_material(0, 0, 1, GL_FRONT);   
         else
            Gluvi::set_generic_material(0, 1, 0, GL_FRONT);   


        /* float curvValue = fabs(g_renderable_vertex_curvatures[tri[0]]);
         if(g_renderable_vertex_curvatures[tri[0]] < 0) {
            Gluvi::set_generic_material(curvValue / divisor, 0, 0, GL_FRONT);   
         }
         else {
            Gluvi::set_generic_material(0, 0, curvValue / divisor, GL_FRONT);   
         }*/
         glNormal3dv( g_renderable_vertex_normals[tri[0]].v );
         glVertex3dv( g_renderable_vertices[tri[0]].v );
         
       /*  curvValue = fabs(g_renderable_vertex_curvatures[tri[1]]);
         if(g_renderable_vertex_curvatures[tri[0]] < 0) {
            Gluvi::set_generic_material(curvValue / divisor, 0, 0, GL_FRONT);   
         }
         else {
            Gluvi::set_generic_material(0, 0, curvValue / divisor, GL_FRONT);   
         }*/

         
         glNormal3dv( g_renderable_vertex_normals[tri[1]].v );
         glVertex3dv( g_renderable_vertices[tri[1]].v );
         
         
        /* curvValue = fabs(g_renderable_vertex_curvatures[tri[2]]);
         if(g_renderable_vertex_curvatures[tri[2]] < 0) {
            Gluvi::set_generic_material(curvValue / divisor, 0, 0, GL_FRONT);   
         }
         else {
            Gluvi::set_generic_material(0, 0, curvValue / divisor, GL_FRONT);   
         }*/
         glNormal3dv( g_renderable_vertex_normals[tri[2]].v );
         glVertex3dv( g_renderable_vertices[tri[2]].v );

      }
      glEnd();
      
      glDisable(GL_LIGHTING);
      
      // edges
      
      if ( g_draw_surface_edges )
      {
         glEnable(GL_LINE_SMOOTH);
         glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);		// Antialias the lines
         glEnable(GL_BLEND);
         glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
         
         glColor3f( 0.0f, 0.0f, 0.0f );
         
         glBegin( GL_LINES );
         for ( unsigned int i = 0; i < g_renderable_edges.size(); ++i )
         {
            const Vec2ui& edge = g_renderable_edges[i];
            glVertex3dv( g_renderable_vertices[edge[0]].v );
            glVertex3dv( g_renderable_vertices[edge[1]].v );
         }
         glEnd();
      }
      
#endif
      
      
   }
      
   //if ( g_running )
   {
      glutTimerFunc( 0, advance_frame, 0);
   }
   
//   if ( g_rendering_sequence )
//   {
//      char sgifilename[256];
//      sprintf( sgifilename, "/Users/tyson/scratch/voronoi3d/screenshot%04d.sgi", g_frame );
//      Gluvi::sgi_screenshot( sgifilename );
//      ++g_frame;
//      
//      char filename[256];
//      //sprintf( filename, "/Users/tyson/scratch/sg2010/dent-regular-2/surface%04d.bin", g_frame );
//      //sprintf( filename, "/Users/tyson/scratch/sg2010/dent-adapt/surface%04d.bin", g_frame );
//      //sprintf( filename, "/Users/tyson/scratch/sg2010/merge3d/splash_triangles/surface%04d.bin", g_frame );
//      sprintf( filename, "/Users/tyson/scratch/bigsplash/big_splash_tris/surface%04d.bin", g_frame );
//      //load_and_process_mesh( filename );
//      load_and_process_splash( filename );
//
//   }
   
}
 
// ---------------------------------------------------------

void mouse(int button, int state, int x, int y)
{
   cam->oldmousex=x;
   cam->oldmousey=y;

}

// ---------------------------------------------------------

void drag(int x, int y)
{
   
   {
      int disty = (cam->oldmousey - y);
      //cam->near_clip_factor = max( 0.01f, cam->near_clip_factor + 0.003f * (float)(disty) );
      
      clipping_plane_distance += 0.03f * disty;      
   }
   
   cam->oldmousex=x;
   cam->oldmousey=y;

   glutPostRedisplay();
   
}

// ---------------------------------------------------------

void snap_to_box( const Vec3d& box_low, const Vec3d& box_high, double tol, Vec3d& v )
{
   Vec3d new_position(v);
   if ( fabs( v[0] - box_low[0] ) < tol ) { new_position[0] = box_low[0]; }
   if ( fabs( v[1] - box_low[1] ) < tol ) { new_position[1] = box_low[1]; }
   if ( fabs( v[2] - box_low[2] ) < tol ) { new_position[2] = box_low[2]; }
   if ( fabs( v[0] - box_high[0] ) < tol ) { new_position[0] = box_high[0]; }
   if ( fabs( v[1] - box_high[1] ) < tol ) { new_position[1] = box_high[1]; }
   if ( fabs( v[2] - box_high[2] ) < tol ) { new_position[2] = box_high[2]; }      
   v = new_position;
}

// ---------------------------------------------------------


void snap_to_solid_splash( Vec3d& v )
{
   if ( v[1] > 1.2 ) { return; }
   
   v[0] = max( v[0], (double) BOX_DOMAIN_LOW[0] );
   v[1] = max( v[1], (double) BOX_DOMAIN_LOW[1] );
   v[2] = max( v[2], (double) BOX_DOMAIN_LOW[2] );
   v[0] = min( v[0], (double) BOX_DOMAIN_HIGH[0] );
   v[1] = min( v[1], (double) BOX_DOMAIN_HIGH[1] );
   v[2] = min( v[2], (double) BOX_DOMAIN_HIGH[2] );
}


// ---------------------------------------------------------

void snap_to_solid( Vec3d& v )
{   
   float min_dist = 1e30;
   unsigned int min_wall_index = ~0;
   
   for ( unsigned int dim = 0; dim < 3; ++dim )
   {
      if ( fabs( v[dim] - BOX_DOMAIN_LOW[dim] ) < min_dist ) 
      {
         min_dist = fabs( v[dim] - BOX_DOMAIN_LOW[dim] );
         min_wall_index = dim;
      }   
   }

   for ( unsigned int dim = 0; dim < 3; ++dim )
   {
      if ( fabs( v[dim] - BOX_DOMAIN_HIGH[dim] ) < min_dist ) 
      {
         min_dist = fabs( v[dim] - BOX_DOMAIN_HIGH[dim] );
         min_wall_index = 3 + dim;
      }   
   }

   if ( min_wall_index < 3 )
   {
      v[min_wall_index] = BOX_DOMAIN_LOW[min_wall_index];
   }
   else
   {
      v[min_wall_index - 3] = BOX_DOMAIN_HIGH[min_wall_index - 3];
   }   
}

// ---------------------------------------------------------


void load_and_process_splash( char* filename )
{
   std::vector<Vec3d> x;
   std::vector<Vec3d> new_x;   
   NonDestructiveTriMesh mesh;   
   std::vector<double> masses;
   double t;
   bool ok = read_binary_file( mesh, x, masses, t, filename );
   assert(ok);
   
   std::vector<Vec3ui> tris;
   for ( unsigned int i = 0; i < mesh.m_tris.size(); ++i )
   {
      const Vec3ui& tri = mesh.m_tris[i];
//      if ( masses[tri[0]] < 1.5 && masses[tri[1]] < 1.5 && masses[tri[2]] < 1.5 )
      {
         tris.push_back( tri );
      }
   }   
   
   SurfTrackInitializationParameters surf_track_params;
   
   surf_track_params.m_use_fraction = true;
   surf_track_params.m_min_edge_length = 0.5;      // times initial average edge length
   surf_track_params.m_max_edge_length = 1.5;      // times initial average edge length
   surf_track_params.m_max_volume_change = 0.0001;  // times initial average edge length cubed
   surf_track_params.m_collision_safety = true;
   surf_track_params.m_allow_topology_changes = false;
   
   SurfTrack test_surface( x, tris, masses, surf_track_params );
   
   // find boundary of the open surface
   for ( unsigned int i = 0; i < test_surface.get_num_vertices(); ++i )
   {
      Vec3d point = test_surface.get_position(i);
      snap_to_solid_splash( point );
      test_surface.set_position(i, point);
   }   
   
   
   pthread_mutex_lock( &sim_mutex );   
   
   SurfTrack* explicit_surface = g_dual_sim->surface_tracker;
   explicit_surface->m_mesh = test_surface.m_mesh;
   explicit_surface->set_all_positions(test_surface.get_positions());
   explicit_surface->m_masses.resize( explicit_surface->get_num_vertices(), 1.0 );   
   //explicit_surface->get_all_vertex_normals();
   
   
   pthread_mutex_unlock( &sim_mutex );   
   
//   char out_filename[256];
//   sprintf( out_filename, "/Users/tyson/scratch/bigsplash/clipped-low/surface%04d.bin", g_frame );            
//   write_binary_file( test_surface.m_mesh, test_surface.m_positions, test_surface.m_masses, t, out_filename );

   update_renderable_objects();

   glutPostRedisplay();
   
   char sgifileformat[256];
   sprintf( sgifileformat, "%s/screenshot%%04d.sgi", g_path );
   Gluvi::sgi_screenshot(sgifileformat, g_frame);      
   
}

// ---------------------------------------------------------


void load_and_process_mesh( char* filename )
{
   std::vector<Vec3d> x;
   std::vector<Vec3d> new_x;   
   NonDestructiveTriMesh mesh;   
   std::vector<double> masses;
   double t;
   bool ok = read_binary_file( mesh, x, masses, t, filename );
   assert(ok);
   

   Vec3f box_centre = 0.5f * ( BOX_DOMAIN_LOW + BOX_DOMAIN_HIGH );
   Vec3f box_extents = BOX_DOMAIN_HIGH - BOX_DOMAIN_LOW;

//   for ( unsigned int i = 0; i < x.size(); ++i )
//   {
//      float vertex_solid_phi = solid_box_phi( box_centre, box_extents, Vec3f(x[i]) );
//      // mark vertices against the solid wall
//      if ( vertex_solid_phi > -1e-4 )
//      {
//         masses[i] = 2.0;
//      }
//   }
   
   std::vector<Vec3ui> tris;
   for ( unsigned int i = 0; i < mesh.m_tris.size(); ++i )
   {
      const Vec3ui& tri = mesh.m_tris[i];
      if ( masses[tri[0]] < 1.5 && masses[tri[1]] < 1.5 && masses[tri[2]] < 1.5 )
      {
         tris.push_back( tri );
      }
   }   
   
   SurfTrackInitializationParameters surf_track_params;
   
   surf_track_params.m_use_fraction = true;
   surf_track_params.m_min_edge_length = 0.5;      // times initial average edge length
   surf_track_params.m_max_edge_length = 1.5;      // times initial average edge length
   surf_track_params.m_max_volume_change = 0.0001;  // times initial average edge length cubed
   surf_track_params.m_collision_safety = true;
   surf_track_params.m_allow_topology_changes = false;
   
   SurfTrack test_surface( x, tris, masses, surf_track_params );
   
   // find boundary of the open surface
   for ( unsigned int i = 0; i < test_surface.m_mesh.m_edges.size(); ++i )
   {
      if ( test_surface.m_mesh.m_edge_to_triangle_map[i].size() == 1 )
      {
         Vec3d pt_a = test_surface.get_position(test_surface.m_mesh.m_edges[i][0]), 
            pt_b = test_surface.get_position(test_surface.m_mesh.m_edges[i][1]);

         snap_to_solid( pt_a );
         snap_to_solid( pt_b );

         test_surface.set_position(test_surface.m_mesh.m_edges[i][0], pt_a);
         test_surface.set_position(test_surface.m_mesh.m_edges[i][1], pt_b);
      }
   }   
   
   pthread_mutex_lock( &sim_mutex );   
   
   SurfTrack* explicit_surface = g_dual_sim->surface_tracker;
   explicit_surface->m_mesh = test_surface.m_mesh;
   explicit_surface->set_all_positions( test_surface.get_positions() );
   explicit_surface->m_masses.resize( explicit_surface->get_num_vertices(), 1.0 );   
   explicit_surface->rebuild_static_broad_phase();
   //explicit_surface->recompute_cached_vertex_normals();
   
   pthread_mutex_unlock( &sim_mutex );   
      
   update_renderable_objects();
   
   glutPostRedisplay();
      
   char sgifileformat[256];
   sprintf( sgifileformat, "%s/screenshot%%04d.sgi", g_path );
   Gluvi::sgi_screenshot(sgifileformat, g_frame);      
   
}

// ---------------------------------------------------------

void load_mesh( )
{
   std::vector<Vec3d> x;
   std::vector<Vec3d> new_x;
   
   NonDestructiveTriMesh mesh;
   
   std::vector<double> masses;
   double dt;
   
   bool ok = read_binary_file_with_newpositions( mesh, x, masses, new_x, dt, "/Users/tyson/scratch/pre-integration.bin" );
   
   std::vector<Vec3ui> tris = mesh.m_tris;
   
   assert(ok);
   
   SurfTrackInitializationParameters surf_track_params;
   
   surf_track_params.m_use_fraction = true;
   surf_track_params.m_min_edge_length = 0.5;      // times initial average edge length
   surf_track_params.m_max_edge_length = 1.5;      // times initial average edge length
   surf_track_params.m_max_volume_change = 0.0001;  // times initial average edge length cubed
   surf_track_params.m_collision_safety = true;
   surf_track_params.m_allow_topology_changes = false;
   
   SurfTrack test_surface( x, tris, masses, surf_track_params );
   
   pthread_mutex_lock( &sim_mutex );   
   
   SurfTrack* explicit_surface = g_dual_sim->surface_tracker;
   explicit_surface->m_mesh = test_surface.m_mesh;
   explicit_surface->set_all_positions(test_surface.get_positions());
   explicit_surface->set_all_newpositions(new_x);
   explicit_surface->m_masses.resize( explicit_surface->get_num_vertices(), 1.0 );
   explicit_surface->rebuild_static_broad_phase();
   
   pthread_mutex_unlock( &sim_mutex );   
   
   update_renderable_objects();
   
   glutPostRedisplay();
   
   char sgifileformat[256];
   sprintf( sgifileformat, "%s/screenshot%%04d.sgi", g_path );
   Gluvi::sgi_screenshot(sgifileformat, g_frame);      
      
}

// ---------------------------------------------------------

void keyboard(unsigned char key, int, int )
{
   switch( key )
   {
      case 'a':
         g_draw_voronoi_areas = !g_draw_voronoi_areas;
         break;                  
      case 'b':
         g_draw_solid_box = !g_draw_solid_box;
         g_draw_domain_box = !g_draw_domain_box;
         break;
      case 'c':
         g_draw_tet_circumcentres = !g_draw_tet_circumcentres;
         break;
      case 'C':
         break;         
         
      case 'd':
         g_default_cam_target[0] = cam->target[0];
         g_default_cam_target[1] = cam->target[1];
         g_default_cam_target[2] = cam->target[2];
         g_default_cam_dist = cam->dist;
         g_default_cam_heading = cam->heading;
         g_default_cam_pitch = cam->pitch;
         std::cout << "target " << g_default_cam_target[0] << " " << g_default_cam_target[1] << " " << g_default_cam_target[2] << std::endl;
         std::cout << "dist " << g_default_cam_dist << std::endl;
         std::cout << "heading " << g_default_cam_heading << std::endl;
         std::cout << "pitch " << g_default_cam_pitch << std::endl;
         break;         
      case 'e':
         g_draw_tet_edges = !g_draw_tet_edges;
         break;
      case 'E':
         g_draw_surface_edges = !g_draw_surface_edges;
         break;
      case 'l':
         g_draw_liquid_phi = !g_draw_liquid_phi;
         break;         
      case 'm':
         g_draw_marker_particles = !g_draw_marker_particles;
         break;
      case 'n':
         g_advance_single_frame = true;
         break;
      case 'p':
         g_draw_clipping_plane = !g_draw_clipping_plane;
         break;
      case 'r':
         cam->target[0] = g_default_cam_target[0];
         cam->target[1] = g_default_cam_target[1];
         cam->target[2] = g_default_cam_target[2];         
         cam->dist = g_default_cam_dist;
         cam->heading = g_default_cam_heading;
         cam->pitch = g_default_cam_pitch;         
         break;         
      case 's':
         g_draw_solid_circumcentres = !g_draw_solid_circumcentres;
         break;         
      case 'S':
         g_draw_explicit_surface = !g_draw_explicit_surface;
         break;
      case 't':
         g_draw_tet_faces = !g_draw_tet_faces;
         break;
      case 'T':
         //g_frame = 0;
         //while(1)
         {
            char filename[256];
            //sprintf( filename, "/Users/tyson/scratch/triangle_data/dent_adaptive/surface%04d.bin", g_frame );
            //sprintf( filename, "/Users/tyson/scratch/triangle_data/dent_regular/surface%04d.bin", g_frame );            
            //sprintf( filename, "/Users/tyson/scratch/triangle_data/surface_tension_cube_regular/surface%04d.bin", g_frame );            
            //sprintf( filename, "/Users/tyson/scratch/triangle_data/surface_tension_cube_adaptive/surface%04d.bin", g_frame );            
            //sprintf( filename, "/Users/tyson/scratch/triangle_data/surface_tension_damped/surface%04d.bin", g_frame );            
            //sprintf( filename, "/Users/tyson/scratch/triangle_data/surface_tension_generalized/surface%04d.bin", g_frame );            
            sprintf( filename, "/Users/tyson/scratch/triangle_data/surface_tension_improved/surface%04d.bin", g_frame );            
            
            load_and_process_mesh( filename );
            //load_and_process_splash( filename );         
            ++g_frame;
         }
         
         break;         
      case 'v':
         g_draw_tet_vertices = !g_draw_tet_vertices;
         break;
      case 'V':
         g_draw_valid_tet_vertices = !g_draw_valid_tet_vertices;
         break;
         
      case '9':
         g_draw_velocities = !g_draw_velocities;
         break;

      case ' ':
         g_running = !g_running;
         std::cout << (g_running ? "start" : "stop") << std::endl;
         break;
   }
   
   glutPostRedisplay();
   
}


// ---------------------------------------------------------

void restore_frame( unsigned int frame )
{
   
   char binfile[256];
   
#ifdef __APPLE__   
   sprintf( binfile, "%s/surface%04d.bin", "/Users/tyson/scratch/voronoi3d", frame );
#elif defined WIN32
   sprintf( binfile, "%s/surface%04d.bin", "C:/output", frame );
#else
   sprintf( binfile, "%s/surface%04d.bin", "/var/tmp", frame );
#endif
   
   pthread_mutex_lock( &sim_mutex );   
   
   SurfTrack* explicit_surface = g_dual_sim->surface_tracker;
   double junk;
   std::vector<Vec3d> points;
   read_binary_file( explicit_surface->m_mesh,
                     points,
                     explicit_surface->m_masses,
                     junk,
                     binfile );
   explicit_surface->set_all_positions(points);

   explicit_surface->m_mesh.update_connectivity();
   explicit_surface->rebuild_static_broad_phase();
   
   char elefile[256];
#ifdef __APPLE__   
   sprintf( elefile, "%s/tetmesh%04d.ele", "/Users/tyson/scratch/voronoi3d", frame );
#elif defined WIN32
   sprintf( elefile, "%s/tetmesh%04d.ele", "C:/output", frame );
#else
   sprintf( elefile, "%s/tetmesh%04d.ele", "/var/tmp", frame );
#endif   
   std::vector<Vec4ui> tets;
   load_ele_file( elefile, tets );
   
   char nodefile[256];
#ifdef __APPLE__   
   sprintf( nodefile, "%s/tetmesh%04d.node", "/Users/tyson/scratch/voronoi3d", frame );
#elif defined WIN32
   sprintf( nodefile, "%s/tetmesh%04d.node", "C:/output", frame );
#else
   sprintf( nodefile, "%s/tetmesh%04d.node", "/var/tmp", frame );
#endif   
   std::vector<Vec3f> xs;
   load_node_file( nodefile, xs );
   
   //TODO repair this.
   //g_dual_sim->mesh.initialize( tets, xs );
   assert(false);

   char velocityfile[256];
#ifdef __APPLE__   
   sprintf( velocityfile, "%s/velocities%04d.bin", "/Users/tyson/scratch/voronoi3d", frame );
#elif defined WIN32
   sprintf( velocityfile, "%s/velocities%04d.bin", "C:/output", frame );
#else
   sprintf( velocityfile, "%s/velocities%04d.bin", "/var/tmp", frame );
#endif   
   
   bifstream velocityfs( velocityfile );
   assert( velocityfs.good() );
   velocityfs.read_endianity();
   for ( unsigned int i = 0; i < g_dual_sim->tet_edge_velocities.size(); ++i )
   {
      velocityfs >> g_dual_sim->tet_edge_velocities[i];
   }
   
   pthread_mutex_unlock( &sim_mutex );   
   
}


// ---------------------------------------------------------

void count_triangles( )
{
   g_frame = 0;
   unsigned int total_num_triangles = 0;
   while (1)
   {
      NonDestructiveTriMesh junk_mesh;
      std::vector<Vec3d> xs;
      std::vector<double> ms;
      double junk;
      
      char binfile[256];
      sprintf( binfile, "%s/surface%04d.bin", "/Users/tyson/scratch/sg2010/new-dam-3", g_frame );
      
      read_binary_file( junk_mesh,
                        xs,
                        ms,
                        junk,
                        binfile );
      
      unsigned int num_real_triangles = 0;
      for ( unsigned int i = 0; i < junk_mesh.m_tris.size(); ++i )
      {
         if ( junk_mesh.m_tris[i][0] != junk_mesh.m_tris[i][1] && junk_mesh.m_tris[i][1] != junk_mesh.m_tris[i][2] && junk_mesh.m_tris[i][0] != junk_mesh.m_tris[i][2] )
         {
            ++num_real_triangles;
         }
      }
      
      total_num_triangles += num_real_triangles;
      ++g_frame;
      std::cout << "average number of triangles: " << total_num_triangles / g_frame << std::endl;
   }
   
   
}

// ---------------------------------------------------------

void file_output( unsigned int frame )
{
   pthread_mutex_lock( &sim_mutex );   
   
   char sgifileformat[256];
   sprintf( sgifileformat, "%s/screenshot%%04d.sgi", g_path );
   Gluvi::sgi_screenshot(sgifileformat, frame);      
   
   char binfile[256];
   sprintf( binfile, "%s/surface%04d.bin", g_path, frame );
   write_binary_file( g_dual_sim->surface_tracker->m_mesh,
                      g_dual_sim->surface_tracker->get_positions(),
                      g_dual_sim->surface_tracker->m_masses,
                      0.0,
                      binfile );
   
//   char elefile[256];
//   sprintf( elefile, "%s/tetmesh%04d.ele", g_path, frame );
   //write_ele_file( elefile, g_dual_sim->mesh.tets );

   //char nodefile[256];
//   sprintf( nodefile, "%s/tetmesh%04d.node", g_path, frame );
//   write_node_file( nodefile, g_dual_sim->mesh.vertices );

//   char velocityfile[256];
//   sprintf( velocityfile, "%s/velocities%04d.bin", g_path, frame );   
//   bofstream velocityfs( velocityfile );
//   assert( velocityfs.good() );
//   velocityfs.write_endianity();
//   for ( unsigned int i = 0; i < g_dual_sim->tet_edge_velocities.size(); ++i )
//   {
//      velocityfs << g_dual_sim->tet_edge_velocities[i];
//   }
   
   
   pthread_mutex_unlock( &sim_mutex );   
   
}

// ---------------------------------------------------------

// runs on a separate thread:
void* advance_frame_async( void* nothing )
{   

   // do work
   std::cout << "worker thread running" << std::endl;
   
   pthread_mutex_lock( &sim_mutex );   

   float sim_dt = 1.0f / static_cast<float>( g_frame_rate );
   
   g_dual_sim->advance( sim_dt, g_num_surface_substeps );

   pthread_mutex_unlock( &sim_mutex );   
   
   // signal we're done
   pthread_mutex_lock( &thread_is_running_mutex );
   thread_is_running = false;
   pthread_mutex_unlock( &thread_is_running_mutex );
   
   return NULL;
}


// ---------------------------------------------------------

void start_advance_frame()
{
      
   // make sure no frame is currently running
   
   pthread_mutex_lock( &thread_is_running_mutex );
   
   if ( !thread_is_running && !waiting_for_thread_to_finish )
   {
      thread_is_running = true;
      
      std::cout << std::endl << std::endl << std::endl << std::endl << std::endl << std::endl;
      std::cout << "Frame: " << g_frame << "-------------------------------------------------------------------------------------------------------" << std::endl;
      std::cout << std::endl ;
      file_output( g_frame );
      
      // kick off advance_frame_async  
      pthread_create( &advance_frame_thread, NULL, advance_frame_async, NULL );
     
      waiting_for_thread_to_finish = true;
   }
   
   pthread_mutex_unlock( &thread_is_running_mutex );

}

// ---------------------------------------------------------

void advance_frame_done()
{
   
   // copy sim data into render buffers
   update_renderable_objects();

   ++g_frame;
   if ( g_frame > 1000 ) { exit(0); }

}




// ---------------------------------------------------------

void advance_frame( int junk )
{
   
   if ( g_running || g_advance_single_frame )
   {
      start_advance_frame();
      g_advance_single_frame = false;
      
//      char filename[256];
//      //sprintf( filename, "/Users/tyson/scratch/generalized/surface%04d.bin", g_frame );            
//      //sprintf( filename, "/Users/tyson/scratch/triangle_data/surface_tension_cube_regular/surface%04d.bin", g_frame );            
//      //sprintf( filename, "/Users/tyson/scratch/triangle_data/surface_tension_cube_adaptive/surface%04d.bin", g_frame );            
//      //sprintf( filename, "/Users/tyson/scratch/triangle_data/surface_tension_improved/surface%04d.bin", g_frame );            
//      //sprintf( filename, "/Users/tyson/scratch/bigsplash/clipped-low/surface%04d.bin", g_frame );            
//      //sprintf( filename, "/Users/tyson/scratch/triangle_data/dent_regular/surface%04d.bin", g_frame );            
//      sprintf( filename, "/Users/tyson/scratch/triangle_data/dent_adaptive/surface%04d.bin", g_frame );            
//      load_and_process_mesh( filename );
      

      //++g_frame;
      
   }
      
   glutPostRedisplay();
   
}


// ---------------------------------------------------------

void dent_surface( const Vec3d& dent_centre, double dent_radius, double dent_magnitude, std::vector<Vec3d>& surface_vertices )
{
   
   double dent_width = 2.0 * dent_magnitude;
   
   for ( unsigned int i = 0; i < surface_vertices.size(); ++i )
   {
      double dist_to_circle = fabs( dist( surface_vertices[i], dent_centre ) - dent_radius );
      
      if ( dist_to_circle < dent_width )
      {
         double mag = 1.0 - (dist_to_circle/dent_width);
         surface_vertices[i][1] -= mag * dent_magnitude;
      }
   }
}

// ---------------------------------------------------------

void parse_script( const char* filename )
{
   std::ifstream filestream( filename );
   if ( !filestream.good() )
   {
      std::cout << "Could not open script file" << std::endl;
      exit(1);
   }
   
   std::cout << "opening file: " << filename << std::endl;
   
   ParseTree tree;
   parse_stream( filestream, tree );
   
   
   //
   // Global settings
   //
   
   std::string output_path;
   if ( tree.get_string( "output_path", output_path ) )
   {
      strncpy( g_path, output_path.c_str(), output_path.length() + 1);
   }
   
   //
   // Surface geometry
   //
   
   double surface_dx;
   assert( tree.get_number( "surface_dx", surface_dx ) );
   
   std::vector<Vec3d> surface_vertices(0);
   std::vector<Vec3ui> surface_triangles(0);
      
   const ParseTree* box_branch = tree.get_branch( "box" );
   if ( box_branch != NULL )
   {
      Vec3d box_min, box_max;
      box_branch->get_vec3d( "box_min", box_min );
      box_branch->get_vec3d( "box_max", box_max );
      double box_dx;
      Vec3ui resolution;
      if ( box_branch->get_number( "box_dx", box_dx ) )
      {
         resolution = Vec3ui((box_max - box_min) / box_dx);
      }
      else
      {
         resolution = Vec3ui((box_max - box_min) / surface_dx);
      }
      resolution[0] = max( (unsigned int)1, resolution[0] );
      resolution[1] = max( (unsigned int)1, resolution[1] );
      resolution[2] = max( (unsigned int)1, resolution[2] );
      std::cout << "box_min: " << box_min << std::endl;
      std::cout << "box_max: " << box_max << std::endl;
      std::cout << "resolution: " << resolution << std::endl;
      create_cube( box_min, box_max, resolution, surface_vertices, surface_triangles );
   }

   const ParseTree* sphere_branch = tree.get_branch( "sphere" );
   if ( sphere_branch != NULL )
   {
      Vec3d sphere_center;
      sphere_branch->get_vec3d( "sphere_center", sphere_center );
      double sphere_radius;
      sphere_branch->get_number( "sphere_radius", sphere_radius );

      double dx;
      if ( !sphere_branch->get_number( "sphere_dx", dx ) )
      {
         dx = surface_dx;
      }

      create_sphere( sphere_center, sphere_radius, dx, surface_vertices, surface_triangles );
   }

   const ParseTree* trimesh_branch = tree.get_branch( "trimesh" );
   if ( trimesh_branch != NULL )
   {
      printf("Found trimesh branch\n");
      Vec3d offset;
      trimesh_branch->get_vec3d("vec_offset", offset);
      printf("Got offset\n");

      double scale;
      trimesh_branch->get_number("scale", scale);
      printf("Got scale\n");
      std::string meshpath;
      trimesh_branch->get_string("filepath", meshpath);
      printf("Got path %s\n", meshpath.c_str());
      
      NonDestructiveTriMesh trimesh;
      
      printf("Reading file\n");
      read_objfile(trimesh, surface_vertices, meshpath.c_str());
      for(unsigned int i = 0; i < surface_vertices.size(); ++i) {
         surface_vertices[i] = surface_vertices[i]*scale + offset;
      }
      surface_triangles = trimesh.m_tris;
      printf("loaded file %s", meshpath.c_str());
   }
   
   const ParseTree* dented_box_branch = tree.get_branch( "dent" );
   if ( dented_box_branch != NULL )
   {
      Vec3d dent_min, dent_max;
      dented_box_branch->get_vec3d( "dent_min", dent_min );
      dented_box_branch->get_vec3d( "dent_max", dent_max );
      double box_dx;
      Vec3ui resolution;
      if ( dented_box_branch->get_number( "dent_dx", box_dx ) )
      {
         resolution = Vec3ui((dent_max - dent_min) / box_dx);
      }
      else
      {
         resolution = Vec3ui((dent_max - dent_min) / surface_dx);
      }
      resolution[0] = max( (unsigned int)1, resolution[0] );
      resolution[1] = max( (unsigned int)1, resolution[1] );
      resolution[2] = max( (unsigned int)1, resolution[2] );
         
      create_cube( dent_min, dent_max, resolution, surface_vertices, surface_triangles );
      
      Vec3d dent_center;
      double dent_radius, dent_magnitude;      
      dented_box_branch->get_vec3d( "dent_center", dent_center );
      dented_box_branch->get_number( "dent_radius", dent_radius );
      dented_box_branch->get_number( "dent_magnitude", dent_magnitude );
      
      std::cout << "dent_min: " << dent_min << std::endl;
      std::cout << "dent_max: " << dent_max << std::endl;
      std::cout << "dent_center: " << dent_center << std::endl;
      std::cout << "dent_radius: " << dent_radius << std::endl;
      std::cout << "dent_magnitude: " << dent_magnitude << std::endl;      
      
      dent_surface( dent_center, dent_radius, dent_magnitude, surface_vertices );
   }
   
   const ParseTree* union_branch = tree.get_branch( "union_implicit_boxes" );
   if ( union_branch != NULL )
   {

      std::vector<Array3d> phis;
      int num_boxes;
      union_branch->get_int( "num_boxes", num_boxes );
      phis.resize( num_boxes );
      
      double phi_dx;
      union_branch->get_number( "phi_dx", phi_dx );
      
      Array3d union_phi;
      
      union_phi.resize( (int) ceil( (DOMAIN_HIGH[0]-DOMAIN_LOW[0]) / phi_dx), 
                        (int) ceil( (DOMAIN_HIGH[1]-DOMAIN_LOW[1]) / phi_dx), 
                        (int) ceil( (DOMAIN_HIGH[2]-DOMAIN_LOW[2]) / phi_dx),
                        1e+30 );
      
      for ( int i = 0; i < num_boxes; ++i )
      {
         Array3d current_phi;
         char min_str[16];
         char max_str[16];
         
         sprintf( min_str, "box%d_min", i );
         sprintf( max_str, "box%d_max", i );
         
         std::cout << "min_str: " << min_str << std::endl;
         std::cout << "max_str: " << max_str << std::endl;
         
         Vec3d current_box_min, current_box_max;
         assert( union_branch->get_vec3d( min_str, current_box_min ) );
         assert( union_branch->get_vec3d( max_str, current_box_max ) );
      
         std::cout << "current_box_min: " << current_box_min << std::endl;
         std::cout << "current_box_max: " << current_box_max << std::endl;
         
         create_cube_signed_distance( current_box_min, current_box_max, phi_dx, Vec3d(DOMAIN_LOW), Vec3d(DOMAIN_HIGH), current_phi );         
         
         assert( current_phi.ni == union_phi.ni );
         assert( current_phi.nj == union_phi.nj );
         assert( current_phi.nk == union_phi.nk );
         
         for ( int i = 0; i < union_phi.ni; ++i )
            for ( int j = 0; j < union_phi.nj; ++j )
               for ( int k = 0; k < union_phi.nk; ++k )
               {
                  union_phi(i,j,k) = min( union_phi(i,j,k), current_phi(i,j,k) );
               }
      }
      
      MarchingTilesHiRes marching_tiles( Vec3d(DOMAIN_LOW), phi_dx, union_phi );
      marching_tiles.contour();
      marching_tiles.improve_mesh();
      
      for ( unsigned int i = 0; i < marching_tiles.x.size(); ++i ) 
      {
         surface_vertices.push_back( marching_tiles.x[i] );
      }
      
      for ( unsigned int i = 0; i < marching_tiles.tri.size(); ++i )
      {
         surface_triangles.push_back( marching_tiles.tri[i] );
      }   
      
      
   }
   
   //
   // SurfTrack parameters
   //
   
   double min_edge_fraction, max_edge_fraction, max_volume_change_fraction;
   double merge_proximity_fraction, repulsion_proximity_fraction;
   int perform_improvement, topology_changes, collision_safety;
   std::string subdivision_scheme;
   tree.get_number( "min_edge_length_fraction", min_edge_fraction );
   tree.get_number( "max_edge_length_fraction", max_edge_fraction );
   tree.get_number( "max_volume_change_fraction", max_volume_change_fraction );
   tree.get_number( "merge_proximity_fraction", merge_proximity_fraction );
   tree.get_number( "repulsion_proximity_fraction", repulsion_proximity_fraction );
   tree.get_int( "perform_improvement", perform_improvement );
   tree.get_int( "allow_topology_changes", topology_changes );   
   tree.get_int( "collision_safety", collision_safety );   
   tree.get_string( "subdivision_scheme", subdivision_scheme );
   
   SurfTrackInitializationParameters surf_track_params;
   surf_track_params.m_min_edge_length = min_edge_fraction * surface_dx;
   surf_track_params.m_max_edge_length = max_edge_fraction * surface_dx;
   surf_track_params.m_max_volume_change = max_volume_change_fraction * surface_dx * surface_dx * surface_dx;   
   surf_track_params.m_merge_proximity_epsilon = merge_proximity_fraction * surface_dx;
   surf_track_params.m_proximity_epsilon = repulsion_proximity_fraction * surface_dx;
   surf_track_params.m_perform_improvement = (perform_improvement != 0);
   surf_track_params.m_allow_topology_changes = (topology_changes != 0);
   surf_track_params.m_collision_safety = (collision_safety != 0);   

   if ( strcmp( subdivision_scheme.c_str(), "butterfly" ) == 0 )
   {
      std::cout << "using butterfly subdivision"  << std::endl;
      
      surf_track_params.m_subdivision_scheme = new ButterflyScheme();
   }
   
   std::vector<double> surface_masses( surface_vertices.size(), 1.0 );


   // TEMP
   int nss = 1;
   tree.get_int( "null_space_smoothing", nss );   
   g_null_space_smoothing = ( nss != 0 );

   double eigenvalue_rank_ratio;
   if ( tree.get_number( "eigenvalue_rank_ratio", eigenvalue_rank_ratio ) )
   {
      G_EIGENVALUE_RANK_RATIO = eigenvalue_rank_ratio;
   }
   
   
   //
   // Simulation parameters
   //
   
   double mesh_dx;
   tree.get_number( "sim_dx", mesh_dx );

   int free_surface, remesh, seed_at_solid, allow_solid_overlap, volume_correction;
   double surface_tension_coefficient;
   Vec3d gravity_force;
   tree.get_int( "free_surface", free_surface );
   tree.get_int( "remesh", remesh );
   tree.get_int( "seed_at_solid", seed_at_solid );
   tree.get_int( "allow_solid_overlap", allow_solid_overlap );
   tree.get_int( "volume_correction", volume_correction );
   tree.get_number( "surface_tension_coefficient", surface_tension_coefficient );
   tree.get_vec3d( "gravity_force", gravity_force );
   tree.get_number( "air_sample_rejection_threshold", g_air_sample_rejection_threshold );
   tree.get_number( "lapack_svd_rcond", g_svd_rcond );\
   
   tree.get_number( "frame_rate", g_frame_rate );
   tree.get_int( "num_surface_substeps", g_num_surface_substeps );
   
   
   const ParseTree* simulation_domain_branch = tree.get_branch( "simulation_domain" );
   if ( simulation_domain_branch != NULL )
   {
      Vec3d sim_min, sim_max;
      simulation_domain_branch->get_vec3d( "min", sim_min );
      simulation_domain_branch->get_vec3d( "max", sim_max );      
      DOMAIN_LOW = Vec3f(sim_min);
      DOMAIN_HIGH = Vec3f(sim_max);
   }

   const ParseTree* solid_domain_branch = tree.get_branch( "solid_domain" );
   if ( solid_domain_branch != NULL )
   {
      Vec3d sim_min, sim_max;
      solid_domain_branch->get_vec3d( "min", sim_min );
      solid_domain_branch->get_vec3d( "max", sim_max );      
      BOX_DOMAIN_LOW = Vec3f(sim_min);
      BOX_DOMAIN_HIGH = Vec3f(sim_max);
   }
   
   pthread_mutex_lock( &sim_mutex ); 
   
   g_dual_sim = new DualFluidSim3D( surface_vertices, surface_triangles, surface_masses, surf_track_params );
   
   std::vector<Vec4ui> tets;
   std::vector<Vec3f> xs;
   
   
    std::vector<Vec3f> input_xs;
    SampleSeeder::generate_bcc_points( DOMAIN_LOW, DOMAIN_HIGH, mesh_dx, input_xs );   
      
    Triangulation cgal_T;
    //Use CGAL Delaunay mesher
    compute_delaunay_CGAL(input_xs, tets, cgal_T);
    xs = input_xs;

    //Use TetGen Delaunay mesher
    //TetGen::delaunay_mesh( input_xs, xs, tets );
   
   int sphere_velocity_field = 0;

   tree.get_int( "sphere_velocity_field", sphere_velocity_field);
   if(sphere_velocity_field) {
      //start the velocities for the spheres going towards each other
      std::cout << "Setting up labels on two sphere geometry.\n";

      //also go through and set the triangle surface labels to be various
      //left of centre, outside is 0, inside is 1
      //right of centre, outside is 0, inside is 2
      for(int i = 0; i < g_dual_sim->surface_tracker->m_mesh.m_tris.size(); ++i) {
         Vec3d centre = g_dual_sim->surface_tracker->get_triangle_barycenter(i);

         Vec2i label;
         if(centre[0] > 2.5) {
            label = Vec2i(0,1);
         }
         else {
            label = Vec2i(0,2);
         }
         g_dual_sim->surface_tracker->m_mesh.set_triangle_label(i, label);
      }

   }
   
   g_dual_sim->free_surface = free_surface?true:false;
   g_dual_sim->mesh->initialize( tets, xs, cgal_T );
   
   // sim parameters
   g_dual_sim->domain_low = DOMAIN_LOW;
   g_dual_sim->domain_high = DOMAIN_HIGH;
   g_dual_sim->solid_low = BOX_DOMAIN_LOW;
   g_dual_sim->solid_high = BOX_DOMAIN_HIGH;   
   g_dual_sim->characteristic_distance = mesh_dx;
   
   // sim data members
   g_dual_sim->initialize();
   
   g_dual_sim->should_remesh = ( remesh != 0 );
   seed_samples_at_solid = ( seed_at_solid != 0 );
   g_dual_sim->allow_solid_overlap = ( allow_solid_overlap != 0 );
   g_dual_sim->initial_volume = g_dual_sim->surface_tracker->get_volume();
   g_dual_sim->volume_correction = ( volume_correction != 0 );
   g_dual_sim->surface_tension_coefficient = surface_tension_coefficient;
   g_dual_sim->gravity = Vec3f( gravity_force );

   int interpolation_scheme_by_int;
   
   if ( tree.get_int( "interpolation_scheme", interpolation_scheme_by_int ) )
   {
      if ( interpolation_scheme_by_int < 0 || interpolation_scheme_by_int > DualFluidSim3D::NUM_INTERPOLATION_SCHEMES )
      {
         std::cout << "SCRIPT ERROR: Invalid interpolation scheme specified" << std::endl;
      }
      else
      {
         g_dual_sim->interpolation_scheme = interpolation_scheme_by_int;
      }
   }
   
   std::string interpolation_scheme_by_name;
   
   if ( tree.get_string( "interpolation_scheme", interpolation_scheme_by_name ) )
   {
      const char* str = interpolation_scheme_by_name.c_str();
      if ( !strcmp( str, "barycentric" ) )
      {
         g_dual_sim->interpolation_scheme = DualFluidSim3D::BARYCENTRIC;
      }
      else if ( !strcmp( str, "improved" ) )
      {
         g_dual_sim->interpolation_scheme = DualFluidSim3D::IMPROVED_BARYCENTRIC;
      }
      else if ( !strcmp( str, "generalized" ) )
      {
         g_dual_sim->interpolation_scheme = DualFluidSim3D::GENERALIZED_BARYCENTRIC;
      }
      else if ( !strcmp( str, "whitney" ) )
      {
         g_dual_sim->interpolation_scheme = DualFluidSim3D::WHITNEY;
      }
      else
      {
         std::cout << "SCRIPT ERROR: Invalid interpolation scheme specified" << std::endl;
         exit(1);
      }
   }

   sphere_velocity_field = 0;
   tree.get_int( "sphere_velocity_field", sphere_velocity_field);
   if(sphere_velocity_field) {
      //start the velocities for the spheres going towards each other
      std::cout << "Setting up colliding sphere velocity field.\n";
      /*
      for(unsigned int i = 0; i < g_dual_sim->tet_edge_velocities.size(); ++i) {
         Vec3f target_velocity(0.5,0,0);
         Vec3f pos = g_dual_sim->mesh->voronoi_face_centroids[i];
         Vec3f dir = g_dual_sim->mesh->tet_edge_vectors[i];
         if(pos[0] < 2.5)      
            g_dual_sim->tet_edge_velocities[i] = dot(dir, target_velocity);
         else
            g_dual_sim->tet_edge_velocities[i] = dot(dir, -target_velocity);
      }
      for(unsigned int i = 0; i < g_dual_sim->tet_vertex_velocities.size(); ++i) {
         Vec3f target_velocity(0.2,0,0);
         Vec3f pos = g_dual_sim->mesh->vertices[i];
         if(pos[0] < 2.5)      
            g_dual_sim->tet_vertex_velocities[i] = target_velocity;
         else
            g_dual_sim->tet_vertex_velocities[i] = -target_velocity;
      }
      for(unsigned int i = 0; i < g_dual_sim->voronoi_vertex_velocities.size(); ++i) {
         Vec3f target_velocity(0.2,0,0);
         Vec3f pos = g_dual_sim->mesh->tet_circumcentres[i];
         if(pos[0] < 2.5)      
            g_dual_sim->voronoi_vertex_velocities[i] = target_velocity;
         else
            g_dual_sim->voronoi_vertex_velocities[i] = -target_velocity;
      }
      */

     

   }
   
   pthread_mutex_unlock( &sim_mutex ); 
   
   //
   // init gui
   //
   
   
   Vec3d camera_target;
   double camera_distance, camera_heading, camera_pitch;
   if ( tree.get_vec3d( "camera_target", camera_target ) )
   {
      cam->target[0] = (float) camera_target[0];
      cam->target[1] = (float) camera_target[1];
      cam->target[2] = (float) camera_target[2];
   }
   
   if ( tree.get_number( "camera_distance", camera_distance ) )
   {
      cam->dist = camera_distance;
   }
   
   if ( tree.get_number( "camera_heading", camera_heading ) )
   {
      cam->heading = camera_heading;
   }
   
   if ( tree.get_number( "camera_pitch", camera_pitch ) )
   {
      cam->pitch = camera_pitch;
   }
   
}


// ---------------------------------------------------------

int main( int argc, char** argv )
{

   set_time_base();
     
//   Gluvi::winwidth = 800;
//   Gluvi::winheight = 600;

   Gluvi::winwidth = 1920;
   Gluvi::winheight = 1080;

   Gluvi::init( "Voronoi Sim 3D", &argc, argv );    
   Gluvi::camera = new Gluvi::Target3D( );
   cam = (Gluvi::Target3D*) (Gluvi::camera);
      
   //
   // init El Topo
   //
      
   char script_filename[256];
   if ( argc > 1 )
   {
      strncpy( script_filename, argv[1], 256 );
   }
   else
   {
      //strncpy( script_filename, "C:\\cbatty\\Research\\ImagerCVS\\tbrochu\\voronoifluid3d\\scripts\\surface_tension_spheres.txt", 256 );
      strncpy( script_filename, "/Users/tyson/projects/tbrochu/voronoifluid3d/scripts/droplet_on_solid.txt", 256 );
   }
   parse_script(script_filename);
   
   g_default_cam_target[0] = cam->target[0];
   g_default_cam_target[1] = cam->target[1];
   g_default_cam_target[2] = cam->target[2];
   g_default_cam_dist = cam->dist;
   g_default_cam_heading = cam->heading;
   g_default_cam_pitch = cam->pitch;
   
   cam->near_clip_factor = 0.1f;
   
   Gluvi::userDisplayFunc = &display;   
   Gluvi::userMouseFunc = &mouse;
   Gluvi::userDragFunc = &drag;
   
   glutKeyboardFunc(keyboard);
   
   glClearColor( 1.0f, 1.0f, 1.0f, 0.0f );
   
   
   status_text_widget = new Gluvi::DynamicText( "Ready" );
   Gluvi::root.list.push_back( status_text_widget );
   
   std::cout << "output path: " << g_path << std::endl;

#ifndef WIN32
   
   // see if the output path exists, and if not, create it
   struct stat st;
   if ( stat( g_path, &st ) != 0)
   {
      std::cout << "Output path does not exist.  Attempting to create it." << std::endl;
      int result = mkdir( g_path, 0777 );
      if ( result != 0 )
      {
         std::cout << "failed to create output directory.  Error: " << result << std::endl;
         exit(1);
      }
      std::cout << "directory created" << std::endl;
   }
    
   char script_copy[256];
   snprintf( script_copy, 256, "%s/script.txt", g_path );
   char command[256];
   snprintf( command, 256, "cp %s %s", script_filename, script_copy );

   int ok = system( command );

   assert( ok == 0 );
   
#endif
   
   update_renderable_objects();
      
   Gluvi::run();
   
}
