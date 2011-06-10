/**
 * \file BARodStepper.cc
 *
 * \author smith@cs.columbia.edu
 * \date 02/16/2010
 */

//#define KEEP_ONLY_SOME_RODS

#include <typeinfo>
#include "BAGroomingStepper.hh"
#include "../../Threads/MultithreadedStepper.hh"
#include "../../Core/Timer.hh"
#include "../../Collisions/Collision.hh"

#include <iostream>
#include <fstream>

#ifdef HAVE_OPENMP
#include <omp.h>
#endif

namespace BASim
{

BAGroomingStepper::BAGroomingStepper(std::vector<ElasticRod*>& rods, std::vector<TriangleMesh*>& trimeshes,
        std::vector<ScriptingController*>& scripting_controllers, std::vector<GroomingTimeStepper*>& steppers, const double& dt,
        const double time, const int num_threads, const PerformanceTuningParameters perf_param,
        std::vector<LevelSet*>* levelSets) :
            m_num_dof(0),
            m_rods(rods),
            m_number_of_rods(m_rods.size()),
            m_triangle_meshes(trimeshes),
            m_scripting_controllers(scripting_controllers),
            m_steppers(steppers),
            m_dt(dt),
            m_base_dof_indices(),
            m_base_vtx_indices(),
            m_base_triangle_indices(),
            m_edges(),
            m_faces(),
            m_vertex_radii(),
            m_edge_radii(),
            m_face_radii(),
            m_masses(),
            m_collision_immune(),
            m_respns_enbld(true),
            m_nan_enc(false),
            m_inf_enc(false),
            m_lt0_enc(false),
            m_gt0_enc(false),
            m_collision_detector(NULL),
            m_obj_start(-1),
            m_t(time),
            m_rod_labels(),
            m_simulationFailed(false),
	    m_useKineticDamping(false),
            m_stopOnRodError(false),
            m_perf_param(perf_param),
            m_level(0),
            m_geodata(m_xn, m_vnphalf, m_vertex_radii, m_masses, m_collision_immune, m_obj_start,
                    m_perf_param.m_implicit_thickness, m_perf_param.m_implicit_stiffness)
//m_timers(NULL)
{
    if (levelSets != NULL)
    {
        m_level_sets = *levelSets;
    }
    else
    {
        // Weirdly we're going to build a vector of null pointers. This is because
        // the vector of level sets can contain a pointer to a level set or a null pointer
        // depending on whether the corresponding m_triangle_mesh has a level set created for it.
        // To simplify the usage code we'll make this vector and pass it around.
        // If level sets become useful we should rethink the entire way data is passed into
        // BAGroomingStepper.
        m_level_sets.resize(m_triangle_meshes.size(), NULL);
    }

    g_log = new TextLog(std::cerr, MsgInfo::kDebug, true);
    InfoStream(g_log, "") << "Started logging BAGroomingStepper\n";

    for (std::vector<GroomingTimeStepper*>::iterator stepper = m_steppers.begin(); stepper != m_steppers.end(); ++stepper)
    {
        (*stepper)->setMaxIterations(m_perf_param.m_solver.m_max_iterations);
    }

#ifndef NDEBUG
    for (int i = 0; i < (int) m_number_of_rods; ++i)
        assert(m_rods[i] != NULL);
    for (int i = 0; i < (int) m_triangle_meshes.size(); ++i)
        assert(m_triangle_meshes[i] != NULL);

    // Do not check if any level sets are null as that may be valid as not all triangle meshes
    // may have a level set.
    //for( int i = 0; i < (int) m_level_sets.size(); ++i ) assert( m_level_sets[i] != NULL );
    if (m_level_sets.size() > 0)
    {
        // If there are any level sets then there has to be as one level set for every triangle mesh
        assert(m_level_sets.size() == m_triangle_meshes.size());
    }
    for (int i = 0; i < (int) m_steppers.size(); ++i)
        assert(m_steppers[i] != NULL);
#endif
    assert(m_dt > 0.0);

    if (num_threads > 0)
    {
        m_num_threads = num_threads;
        CopiousStream(g_log, "") << "User-set number of threads = " << m_num_threads << "\n";
    }
    else
    {
        m_num_threads = sysconf(_SC_NPROCESSORS_ONLN);
        CopiousStream(g_log, "") << "Default-set number of threads = " << m_num_threads << "\n";
    }
#ifdef HAVE_OPENMP
    omp_set_num_threads(m_num_threads);
#endif
    // Update internal state, prepare for execution
    prepareForExecution();

#ifndef NDEBUG
    // Number of degrees of freedom is non-negative multiple of 3 (3 coords per vertex)
    assert(m_num_dof >= 0);
    assert(m_num_dof % 3 == 0);

    // Base indices refers to rods
    assert(m_base_dof_indices.size() == m_number_of_rods);
    // Each base index is a non-negative multiple of 3
    for (int i = 0; i < (int) m_base_dof_indices.size(); ++i)
        assert(m_base_dof_indices[i] >= 0);
    for (int i = 0; i < (int) m_base_dof_indices.size(); ++i)
        assert(m_base_dof_indices[i] % 3 == 0);
    // Each base index must be greater than last
    for (int i = 0; i < (int) m_base_dof_indices.size() - 1; ++i)
        assert(m_base_dof_indices[i] < m_base_dof_indices[i + 1]);

    // Base tirangle indices refers to triangles
    assert(m_base_triangle_indices.size() == m_triangle_meshes.size());
    // Each base index is a non-negative multiple of 3
    for (int i = 0; i < (int) m_base_triangle_indices.size(); ++i)
        assert(m_base_triangle_indices[i] >= 0);
    for (int i = 0; i < (int) m_base_triangle_indices.size(); ++i)
        assert(m_base_triangle_indices[i] % 3 == 0);
    // Each base index must be greater than last
    for (int i = 0; i < (int) m_base_triangle_indices.size() - 1; ++i)
        assert(m_base_triangle_indices[i] < m_base_triangle_indices[i + 1]);

    // Check that we computed the proper start location of tirangle objects in the globale DOF array
    if (m_base_triangle_indices.size() > 0)
        assert(m_obj_start == (int) m_base_triangle_indices.front() / 3);

    // All edges and faces should contain valid vertices
    for (int i = 0; i < (int) m_edges.size(); ++i)
        assert(m_edges[i].first >= 0);
    for (int i = 0; i < (int) m_edges.size(); ++i)
        assert(m_edges[i].second >= 0);
    for (int i = 0; i < (int) m_edges.size(); ++i)
        assert(m_edges[i].first < m_num_dof / 3);
    for (int i = 0; i < (int) m_edges.size(); ++i)
        assert(m_edges[i].second < m_num_dof / 3);

    for (int i = 0; i < (int) m_faces.size(); ++i)
        assert(m_faces[i].idx[0] >= 0);
    for (int i = 0; i < (int) m_faces.size(); ++i)
        assert(m_faces[i].idx[1] >= 0);
    for (int i = 0; i < (int) m_faces.size(); ++i)
        assert(m_faces[i].idx[2] >= 0);
    for (int i = 0; i < (int) m_faces.size(); ++i)
        assert(m_faces[i].idx[0] < m_num_dof / 3);
    for (int i = 0; i < (int) m_faces.size(); ++i)
        assert(m_faces[i].idx[1] < m_num_dof / 3);
    for (int i = 0; i < (int) m_faces.size(); ++i)
        assert(m_faces[i].idx[2] < m_num_dof / 3);

    // In our case, all face vertices should belong to triangles
    for (int i = 0; i < (int) m_faces.size(); ++i)
        assert(m_faces[i].idx[0] >= m_obj_start);
    for (int i = 0; i < (int) m_faces.size(); ++i)
        assert(m_faces[i].idx[1] >= m_obj_start);
    for (int i = 0; i < (int) m_faces.size(); ++i)
        assert(m_faces[i].idx[2] >= m_obj_start);

    // Vertex radii must equal the number of verts!
    assert((int) m_vertex_radii.size() == m_num_dof / 3);
    // Edge radii must equal the number of edges!
    assert(m_edge_radii.size() == m_edges.size());
    // Face radii must equal the number of faces!
    assert(m_face_radii.size() == m_faces.size());
    // All radii must be greater or equal to 0
    for (int i = 0; i < (int) m_vertex_radii.size(); ++i)
        assert(m_vertex_radii[i] >= 0);
    for (int i = 0; i < (int) m_edge_radii.size(); ++i)
        assert(m_edge_radii[i] >= 0);
    for (int i = 0; i < (int) m_face_radii.size(); ++i)
        assert(m_face_radii[i] >= 0);

    // In our case, face radii must be 0. All other radii must be positive.
    for (int i = 0; i < (int) m_face_radii.size(); ++i)
        assert(m_face_radii[i] == 0);
    for (int i = 0; i < (int) m_edge_radii.size(); ++i)
        assert(m_edge_radii[i] < std::numeric_limits<double>::infinity());

    // Number of masses must equal number of verts
    assert((int) m_masses.size() == m_num_dof / 3);
    // Masses must be positive
    for (int i = 0; i < (int) m_masses.size(); ++i)
        assert(m_masses[i] >= 0.0);

    // Check that rod masses are positive doubles, that face masses are infs
    // TODO: Scripted verts get infinite mass, clean up this check later
    //for( int i = 0; i < m_obj_start; ++i ) assert( m_masses[i] < std::numeric_limits<double>::infinity() );
    for (int i = m_obj_start; i < m_num_dof / 3; ++i)
        assert(m_masses[i] == std::numeric_limits<double>::infinity());

    // For each edge, ensure that both vertices are either rod edges or face edges
    for (int i = 0; i < (int) m_edges.size(); ++i)
        assert(
                (m_edges[i].first < m_obj_start && m_edges[i].second < m_obj_start) || (m_edges[i].first >= m_obj_start
                        && m_edges[i].second >= m_obj_start));

    // For each triangle, ensure that all vertices do indeed belong to a triangulated object
    for (int i = 0; i < (int) m_faces.size(); ++i)
        assert(m_faces[i].idx[0] >= m_obj_start && m_faces[i].idx[1] >= m_obj_start && m_faces[i].idx[2] >= m_obj_start);

    // TODO: Furhter, check that they all belong to same rod or triangle obj
#endif

#ifdef TIMING_ON
    IntStatTracker::getIntTracker("CONVERGENCE_FAILURES_PROPAGATED_TO_BAGroomingStepper");
#endif

    /**
     *  Prepare backup structures.
     */
    m_startForces.resize(m_number_of_rods);
    for (int i = 0; i < m_number_of_rods; i++)
        m_startForces[i] = new VecXd(m_rods[i]->ndof());

    m_preDynamicForces.resize(m_number_of_rods);
    for (int i = 0; i < m_number_of_rods; i++)
        m_preDynamicForces[i] = new VecXd(m_rods[i]->ndof());

    m_preCollisionForces.resize(m_number_of_rods);
    for (int i = 0; i < m_number_of_rods; i++)
        m_preCollisionForces[i] = new VecXd(m_rods[i]->ndof());

    m_endForces.resize(m_number_of_rods);
    for (int i = 0; i < m_number_of_rods; i++)
        m_endForces[i] = new VecXd(m_rods[i]->ndof());

    m_rodbackups.resize(m_number_of_rods);
    int i = 0;
    for (std::vector<ElasticRod*>::const_iterator rod = m_rods.begin(); rod != m_rods.end(); rod++)
        m_rodbackups[i++].resize(**rod);

    m_objbackups.resize(m_triangle_meshes.size());
    i = 0;
    for (std::vector<TriangleMesh*>::const_iterator mesh = m_triangle_meshes.begin(); mesh != m_triangle_meshes.end(); mesh++)
        m_objbackups[i++].resize(**mesh);

    // For debugging purposes
#ifdef KEEP_ONLY_SOME_RODS
    WarningStream(g_log, "", MsgInfo::kOncePerMessage)
    << "WARNING: KEEP_ONLY_SOME_RODS: Simulating only a specified subset of rods!\n***********************************************************\n";
    std::set<int> keep_only;

    keep_only.insert(0);

    // Only the rods in the keep_only set are kept, the others are killed.
    for (int i = 0; i < m_number_of_rods; i++)
    if (keep_only.find(i) == keep_only.end())
    for (int j = 0; j < m_rods[i]->nv(); j++)
    m_rods[i]->setVertex(j, 0 * m_rods[i]->getVertex(j));
    else
    m_simulated_rods.push_back(i);
#else
    // Initially all rods passed from Maya will be simulated
    for (int i = 0; i < m_number_of_rods; i++)
        m_simulated_rods.push_back(i);
#endif
    m_killed_rods.clear();
}

BAGroomingStepper::~BAGroomingStepper()
{
    delete m_collision_detector;

    for (int i = 0; i < m_number_of_rods; i++)
    {
        delete m_startForces[i];
        delete m_preDynamicForces[i];
        delete m_preCollisionForces[i];
        delete m_endForces[i];
    }

    delete g_log;
}

// TODO: Check the indices here
void BAGroomingStepper::prepareForExecution()
{
    delete m_collision_detector;
    m_collision_detector = NULL;

    for (int i = 0; i < m_number_of_rods; ++i)
    {
        assert(m_rods[i] != NULL);
        m_rods[i]->globalRodIndex = i;
        // std::cerr << "Address of rod Nr " << i << ": " << m_rods[i] << '\n';
    }

    CopiousStream(g_log, "") << "About to extract rod information\n";

    for (int i = 0; i < m_number_of_rods; ++i)
    {
        assert(m_rods[i] != NULL);
#ifndef NDEBUG
        // Sanity checks for collision detection purposes
        ensureCircularCrossSection(*m_rods[i]);
        ensureNoCollisionsByDefault(*m_rods[i]);
#endif

        // Extract edges from the new rod
        for (int j = 0; j < m_rods[i]->nv() - 1; ++j)
        {
            m_edges.push_back(std::pair<int, int>(getNumVerts() + j, getNumVerts() + j + 1));
            assert(m_rods[i]->getRadiusScale() * m_rods[i]->radiusA(j) > 0.0);
            m_edge_radii.push_back(m_rods[i]->getRadiusScale() * m_rods[i]->radiusA(j));
        }
        assert(m_edges.size() == m_edge_radii.size());

        for (int j = 0; j < m_rods[i]->nv() - 1; ++j)
        {
            assert(m_rods[i]->getRadiusScale() * m_rods[i]->radiusA(j) > 0.0);
            // Radii associated with edges ... what to do if at a vertex with two radii? Average?
            m_vertex_radii.push_back(m_rods[i]->getRadiusScale() * m_rods[i]->radiusA(j));
        }
        m_vertex_radii.push_back(m_vertex_radii.back()); // < TODO: What the $^#! is this call?

        // Update vector that tracks the rod DOF in the system
        m_base_dof_indices.push_back(getNumDof());
        m_base_vtx_indices.push_back(getNumDof() / 3);

        // Extract masses from the new rod
        for (ElasticRod::vertex_iter itr = m_rods[i]->vertices_begin(); itr != m_rods[i]->vertices_end(); ++itr)
        {
            assert(m_rods[i]->getVertexMass(*itr, -1) > 0.0);
            m_masses.push_back(m_rods[i]->getVertexMass(*itr, -1));
        }

        // Update total number of DOF in the system
        m_num_dof += 3 * m_rods[i]->nv();

        assert((int) m_masses.size() == getNumVerts());
        assert((int) m_vertex_radii.size() == getNumVerts());
    }
    assert(m_number_of_rods == m_base_dof_indices.size());
    CopiousStream(g_log, "") << "Extracted rod information: " << m_num_dof / 3 << " vertices\n";

    m_obj_start = m_base_vtx_indices.back() + m_rods.back()->nv();

    CopiousStream(g_log, "") << "About to extract tri mesh information\n";
    for (int i = 0; i < (int) m_triangle_meshes.size(); ++i)
    {
        assert(m_triangle_meshes[i] != NULL);

        // Extract faces from the tri_mesh
        for (TriangleMesh::face_iter fit = m_triangle_meshes[i]->faces_begin(); fit != m_triangle_meshes[i]->faces_end(); ++fit)
        {
            TriangularFace triface;
            int j = 0;
            for (TriangleMesh::FaceVertexIter fvit = m_triangle_meshes[i]->fv_iter(*fit); fvit; ++fvit, ++j)
            {
                assert(j >= 0);
                assert(j < 3);
                triface.idx[j] = getNumVerts() + (*fvit).idx();
            }
            m_faces.push_back(triface);
            m_face_radii.push_back(0.0);
        }
        assert(m_faces.size() == m_face_radii.size());
        CopiousStream(g_log, "") << "Finished extracting face stuff: " << i << "\n";

        // Extract the vertex radii from the tri_mesh
        for (int j = 0; j < m_triangle_meshes[i]->nv(); ++j)
            m_vertex_radii.push_back(0.0);

        // Extract masses from the tri_mesh (just infinity for this object)
        for (int j = 0; j < m_triangle_meshes[i]->nv(); ++j)
            m_masses.push_back(std::numeric_limits<double>::infinity());

        // Add the mesh to the system
        //m_triangle_meshes.push_back(m_triangle_meshes[i]);

        // Update vector that tracks the ScriptedTriangleMesh DOF in the system
        m_base_triangle_indices.push_back(getNumDof());

        // Update total number of DOF in the system
        m_num_dof += 3 * m_triangle_meshes[i]->nv();

        assert((int) m_masses.size() == getNumVerts());
        assert((int) m_vertex_radii.size() == getNumVerts());
    }
    assert(m_base_triangle_indices.size() == m_triangle_meshes.size());
    CopiousStream(g_log, "") << "Extracted tri mesh information\n";

    // Resize the internal storage
    m_xn.resize(getNumDof());
    m_xn.setConstant(std::numeric_limits<double>::signaling_NaN());
    m_xnp1.resize(getNumDof());
    m_xnp1.setConstant(std::numeric_limits<double>::signaling_NaN());
    m_xdebug.resize(getNumDof());
    m_xdebug.setConstant(std::numeric_limits<double>::signaling_NaN());
    m_vnphalf.resize(getNumDof());
    m_vnphalf.setConstant(std::numeric_limits<double>::signaling_NaN());

    CopiousStream(g_log, "") << "About to extract positions\n";
    // Load positions for initial construction of the BVH
    RodSelectionType selected_rods;
    for (int i = 0; i < m_number_of_rods; i++)
        selected_rods.push_back(i);
    extractPositions(m_xn, selected_rods, 0.0);
    extractVelocities(m_vnphalf, selected_rods);
    CopiousStream(g_log, "") << "Extracted positions\n";

    CopiousStream(g_log, "") << "About to create collision detector\n";
    m_collision_detector = new CollisionDetectorType(m_geodata, m_edges, m_faces, m_dt, m_perf_param.m_skipRodRodCollisions,
            m_num_threads);
    CopiousStream(g_log, "") << "Created collision detector\n";

    m_collision_immune.resize(getNumVerts());

    if (m_perf_param.m_enable_penalty_response)
        enableImplicitPenaltyImpulses();

    // DEBUG
    m_total_solver_killed = m_total_collision_killed = m_total_explosion_killed = m_total_stretching_killed = 0;

    m_initialLengths.resize(m_number_of_rods);
    for (RodSelectionType::const_iterator rod = selected_rods.begin(); rod != selected_rods.end(); rod++)
        for (int j = 1; j < m_rods[*rod]->nv(); j++)
            m_initialLengths[*rod] += (m_rods[*rod]->getVertex(j) - m_rods[*rod]->getVertex(j - 1)).norm();

    CopiousStream(g_log, "") << "Finished BAGroomingStepper constructor\n";
}

bool BAGroomingStepper::execute()
{
    START_TIMER("BAGroomingStepper::execute")

    m_num_solver_killed = m_num_explosion_killed = m_num_collision_killed = m_num_stretching_killed = 0;
    DebugStream(g_log, "") << "Executing time step " << m_t << '\n';

    m_collision_detector->buildBVH();
    TraceStream(g_log, "") << "BVH has been rebuilt\n";

    if (!m_collision_disabled_rods.empty())
    {
        TraceStream(g_log, "") << "The following rods were collision-disabled in the previous time step: ";
        for (std::set<int>::const_iterator rod = m_collision_disabled_rods.begin(); rod != m_collision_disabled_rods.end(); rod++)
            TraceStream(g_log, "") << *rod << " ";
        TraceStream(g_log, "") << '\n';
    }
    m_collision_disabled_rods.clear();

    bool do_adaptive = true;
    bool result;

    int k = 0;
    for (int i = 0; i < m_number_of_rods; ++i)
    {
        // Extract masses from the new rod
        for (ElasticRod::vertex_iter itr = m_rods[i]->vertices_begin(); itr != m_rods[i]->vertices_end(); ++itr)
        {
            if (m_rods[i]->getBoundaryCondition()->isVertexScripted((*itr).idx()))
                m_masses[k++] = std::numeric_limits<double>::infinity();
            else
            {
                assert(m_rods[i]->getVertexMass(*itr, -1) > 0.0);
                m_masses[k++] = m_rods[i]->getVertexMass(*itr, -1);
            }
        }
    }
    assert(k = m_masses.size());

    // Prepare the list initially containing all rods.
    RodSelectionType selected_rods = m_simulated_rods;

    if (do_adaptive)
    {
        assert(m_level == 0);
        result = adaptiveExecute(m_dt, selected_rods);
    }
    else
        result = nonAdaptiveExecute(m_dt, selected_rods);

    m_total_solver_killed += m_num_solver_killed;
    m_total_collision_killed += m_num_collision_killed;
    m_total_explosion_killed += m_num_explosion_killed;
    m_total_stretching_killed += m_num_stretching_killed;

    DebugStream(g_log, "") << "Time step finished, " << selected_rods.size() << " rods remaining out of " << m_rods.size()
            << '\n';
    DebugStream(g_log, "") << "Rods killed because of solver failure: " << m_num_solver_killed << " (this step), "
            << m_total_solver_killed << " (total)\n";
    DebugStream(g_log, "") << "Rods killed because of collision failure: " << m_num_collision_killed << " (this step), "
            << m_total_collision_killed << " (total)\n";
    DebugStream(g_log, "") << "Rods killed because of explosion failure: " << m_num_explosion_killed << " (this step), "
            << m_total_explosion_killed << " (total)\n";
    DebugStream(g_log, "") << "Rods killed because of stretching failure: " << m_num_stretching_killed << " (this step), "
            << m_total_stretching_killed << " (total)\n";

    STOP_TIMER("BAGroomingStepper::execute")

#ifdef TIMING_ON
    // This is not using TextLog because std::setw is not supported. TODO: you know.
    std::cout << "Cumulative timing results (entire run up to this point)\n";
    std::cout << "========================================\n";
    Timer::report();
    std::cout << "========================================\n";
#endif

    return result;
}

bool BAGroomingStepper::nonAdaptiveExecute(double dt, RodSelectionType& selected_rods)
{
    setDt(dt);
    setTime(m_t + dt);
    //for (int i = 0; i < m_scripting_controllers.size(); ++i)
    //  m_scripting_controllers[i]->setTime(m_t);
    step(selected_rods);
    return (selected_rods.size() == 0);
}

bool BAGroomingStepper::adaptiveExecute(double dt, RodSelectionType& selected_rods)
{
    START_TIMER("BAGroomingStepper::adaptiveExecute")

    DebugStream(g_log, "") << "adaptiveExecute at level " << m_level << " with " << selected_rods.size() << " rod(s), m_t = "
            << m_t << ", dt = " << dt << '\n';

    // Backup all selected rods
    for (RodSelectionType::const_iterator rod = selected_rods.begin(); rod != selected_rods.end(); rod++)
        m_rodbackups[*rod].backupRod(*m_rods[*rod]);

    // Backup all objects
    for (int i = 0; i < (int) m_triangle_meshes.size(); ++i)
        m_objbackups[i].backupMesh(*m_triangle_meshes[i]);

    // Backup the current simulation time
    double time = m_t;

    // Set the desired timestep
    setDt(dt);
    // Advance the current time
    setTime(m_t + dt);

    // Attempt a full time step
    step(selected_rods);

    if (m_simulationFailed)
    {
        WarningStream(g_log, "", MsgInfo::kOncePerMessage) << "t = " << m_t
                << ": **** SIMULATION FAILED AND IS NOW STOPPED! ****\n***********************************************************\n";
        STOP_TIMER("BAGroomingStepper::adaptiveExecute")
        return true;
    }
    if (selected_rods.empty()) // Success!

    {
        TraceStream(g_log, "") << "t = " << m_t << ": adaptiveExecute has simulated (or killed) all rods\n";
        STOP_TIMER("BAGroomingStepper::adaptiveExecute")
        return true;
    }
    // Otherwise do two half time steps
    DebugStream(g_log, "") << "t = " << m_t << ": adaptiveExecute left " << selected_rods.size() << " rods for substepping\n";
    m_level++;

    // Restore all rods that remained selected after the step
    for (RodSelectionType::const_iterator rod = selected_rods.begin(); rod != selected_rods.end(); rod++)
        m_rodbackups[*rod].restoreRod(*m_rods[*rod]);

    // Restore all objects
    for (int i = 0; i < (int) m_triangle_meshes.size(); ++i)
        m_objbackups[i].restoreMesh(*m_triangle_meshes[i]);

    // Restore the time
    setTime(time);

    // Back up rod selection for time step 2
    RodSelectionType selected_rods_2 = selected_rods;

    DebugStream(g_log, "") << "t = " << m_t << " selected_rods: adaptiveExecute substepping (part 1) " << selected_rods.size()
            << " rods\n";

    bool first_success = adaptiveExecute(0.5 * dt, selected_rods);
    if (!first_success)
    {
        setDt(dt);
        STOP_TIMER("BAGroomingStepper::adaptiveExecute")
        return false;
    }
    DebugStream(g_log, "") << "t = " << m_t << " selected_rods: adaptiveExecute substepping (part 2) "
            << selected_rods_2.size() << " rods\n";

    // Remove from the rod selection any one that might have been killed during the first time step
    for (RodSelectionType::iterator rod = selected_rods_2.begin(); rod != selected_rods_2.end(); rod++)
        if (find(m_simulated_rods.begin(), m_simulated_rods.end(), *rod) == m_simulated_rods.end())
        {
            DebugStream(g_log, "") << "Erasing from second time step rod number " << *rod << '\n';
            selected_rods_2.erase(rod--);
        }
    bool second_success = adaptiveExecute(0.5 * dt, selected_rods_2);
    if (!second_success)
    {
        setDt(dt);
        STOP_TIMER("BAGroomingStepper::adaptiveExecute")
        return false;
    }
    TraceStream(g_log, "") << "Finished two adaptive steps\n";
    setDt(dt);
    m_level--;

    STOP_TIMER("BAGroomingStepper::adaptiveExecute")
    return first_success && second_success;
}

void BAGroomingStepper::step(RodSelectionType& selected_rods)
{
    START_TIMER("BAGroomingStepper::step")

    if (m_simulationFailed) // We stopped simulating already

    {
        STOP_TIMER("BAGroomingStepper::step")
        return;
    }

    TraceStream(g_log, "") << "t = " << m_t << ": BAGroomingStepper::step() begins with " << selected_rods.size() << " rods\n";

    assert(m_edges.size() == m_edge_radii.size());
    assert((int) m_masses.size() == m_xn.size() / 3);
    assert(m_xn.size() == m_xnp1.size());
    assert(m_xn.size() == m_vnphalf.size());
    assert(m_rod_labels.size() == 0 || m_rod_labels.size() == m_number_of_rods);

    // Sanity check to ensure rods are not "internally colliding" because radius is bigger than edge length
#ifndef NDEBUG
    for (int i = 0; i < (int) m_number_of_rods; ++i)
        ensureNoCollisionsByDefault(*m_rods[i]);
#endif
    // Sanity check to ensure different parts of sim have same time/timetep
#ifndef NDEBUG
    for (int i = 0; i < (int) m_scripting_controllers.size(); ++i)
        assert(m_scripting_controllers[i]->getTime() == m_t);
    for (int i = 0; i < (int) m_scripting_controllers.size(); ++i)
        assert(m_scripting_controllers[i]->getDt() == m_dt);
    for (int i = 0; i < (int) m_steppers.size(); ++i)
        assert(m_steppers[i]->getTimeStep() == m_dt);
    for (int i = 0; i < (int) m_number_of_rods; ++i)
        assert(m_rods[i]->getTimeStep() == m_dt);
#endif

    START_TIMER("BAGroomingStepper::step/setup");

    // Prepare list of steppers to be executed.
    std::vector<GroomingTimeStepper*> selected_steppers;
    for (RodSelectionType::const_iterator rod = selected_rods.begin(); rod != selected_rods.end(); rod++)
        selected_steppers.push_back(m_steppers[*rod]);

    STOP_TIMER("BAGroomingStepper::step/setup");

    START_TIMER("BAGroomingStepper::step/explo");

    if (m_perf_param.m_enable_explosion_detection)
        computeForces(m_startForces, selected_rods);

    STOP_TIMER("BAGroomingStepper::step/explo");

    START_TIMER("BAGroomingStepper::step/setup");

    // Save the pre-timestep positions
    extractPositions(m_xn, selected_rods, m_t - m_dt);

    STOP_TIMER("BAGroomingStepper::step/setup");

    START_TIMER("BAGroomingStepper::step/immune");
    // Determine which vertex are to be considered collision-immune for this step
    computeImmunity(selected_rods);
    STOP_TIMER("BAGroomingStepper::step/immune");

    // Step rods forward ignoring collisions
    START_TIMER("BAGroomingStepper::step/scripting");

    // Step scripted objects forward, set boundary conditions
    for (std::vector<ScriptingController*>::const_iterator scripting_controller = m_scripting_controllers.begin(); scripting_controller
            != m_scripting_controllers.end(); scripting_controller++)
        (*scripting_controller)->execute();

    STOP_TIMER("BAGroomingStepper::step/scripting");

    START_TIMER("BAGroomingStepper::step/penalty");

    // Jungseock's implicit penalty
    std::list<Collision*> penalty_collisions;
    // The penalty collisions list is used to create penalty forces. All that is deleted and cleared at the end of this step.
    if (m_perf_param.m_enable_penalty_response)
        setupPenaltyForces(penalty_collisions, selected_rods);

    STOP_TIMER("BAGroomingStepper::step/penalty");

    TraceStream(g_log, "") << "BAGroomingStepper::step: computing pre-dynamic forces" << '\n';

    if (m_perf_param.m_enable_explosion_detection)
        computeForces(m_preDynamicForces, selected_rods);

    START_TIMER("BAGroomingStepper::step/steppers");

    bool dependable_solve = true;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < selected_steppers.size(); i++)
    {
        GroomingTimeStepper* const stepper = selected_steppers[i];

        bool result = stepper->execute();
        if (!result)
            TraceStream(g_log, "") << stepper->getDiffEqSolver().getName() << " solver for rod "
                    << stepper->getRod()->globalRodIndex << " failed to converge after " << stepper->getMaxIterations()
                    << " iterations\n";
        dependable_solve = dependable_solve && result;
    }

    STOP_TIMER("BAGroomingStepper::step/steppers");

    TraceStream(g_log, "") << "Dynamic step is " << (dependable_solve ? "" : "not ") << "entirely dependable"
            << (dependable_solve ? " :-)\n" : "!\n");

    // If we do rod-rod collisions (meaning no selective adaptivity) and global dependability failed, we might as well stop here.
    if (!m_perf_param.m_skipRodRodCollisions && !dependable_solve)
    {
        WarningStream(g_log, "", MsgInfo::kOncePerMessage) << "t = " << m_t
                << " selected_rods: step() failed (due to rod-rod) for " << selected_rods.size() << " rods\n";
        STOP_TIMER("BAGroomingStepper::step")
        return;
    }

    START_TIMER("BAGroomingStepper::step/setup");

    // Post time step position
    extractPositions(m_xnp1, selected_rods, m_t);

    // Average velocity over the timestep just completed
    m_vnphalf = (m_xnp1 - m_xn) / m_dt;

    STOP_TIMER("BAGroomingStepper::step/setup");

    START_TIMER("BAGroomingStepper::step/inexten");

    for (int i = 0; i < selected_steppers.size(); i++)
        applyInextensibilityVelocityFilter(selected_steppers[i]->getRod()->globalRodIndex);

    STOP_TIMER("BAGroomingStepper::step/inexten");

    START_TIMER("BAGroomingStepper::step/immune");

    // Mark invalid rods as entirely collision-immune, so we don't waste time on colliding them.
    for (RodSelectionType::const_iterator rod = selected_rods.begin(); rod != selected_rods.end(); rod++)
        if (!m_steppers[*rod]->HasSolved())
        {
            // std::cerr << "Rod number " << *rod << " failed to solve\n";
            for (int j = 0; j < m_rods[*rod]->nv(); ++j)
                m_collision_immune[m_base_vtx_indices[*rod] + j] = true;
        }

    STOP_TIMER("BAGroomingStepper::step/immune");

    START_TIMER("BAGroomingStepper::step/response");
    TraceStream(g_log, "") << "Starting collision response\n";

    if (m_perf_param.m_enable_explosion_detection)
        computeForces(m_preCollisionForces, selected_rods);

    std::vector<bool> failed_collisions_rods(m_number_of_rods);
    std::vector<bool> stretching_rods(m_number_of_rods);
    if (m_perf_param.m_collision.m_max_iterations > 0)
    {
        if (!executeIterativeInelasticImpulseResponse(failed_collisions_rods, stretching_rods))
        {
            TraceStream(g_log, "") << "Some collision responses failed!\n";
            //all_collisions_succeeded = false;
        }
    }
    TraceStream(g_log, "") << "Finished collision response\n";

    STOP_TIMER("BAGroomingStepper::step/response");

    START_TIMER("BAGroomingStepper::step/setup");

    // Store the response part for visualization
    m_vnresp = m_vnphalf - (m_xnp1 - m_xn) / m_dt;

    // Compute final positions from corrected velocities
    m_xnp1 = m_xn + m_dt * m_vnphalf;

#ifndef NDEBUG
    // Ensure boundary conditions respected by corrected positions
    // For each selected rod
    for (RodSelectionType::const_iterator rod = selected_rods.begin(); rod != selected_rods.end(); rod++)
    {
        RodBoundaryCondition* boundary = m_rods[*rod]->getBoundaryCondition();
        int rodbase = m_base_dof_indices[*rod];

        // For each vertex of the current rod
        for (int j = 0; j < m_rods[*rod]->nv(); ++j)
        {
            // If that vertex has a prescribed position
            if (boundary->isVertexScripted(j))
            {
                Vec3d desiredposition = boundary->getDesiredVertexPosition(j, m_t);
                Vec3d actualvalue = m_xnp1.segment<3> (rodbase + 3 * j);
                assert(approxEq(desiredposition, actualvalue, 1.0e-6));
            }
        }
    }
#endif

    // Copy new positions and velocities back to rods
    restorePositions(m_xnp1, selected_rods);
    restoreVelocities(m_vnphalf, selected_rods);
    // Also copy response velocity to rods (for visualisation purposes only)
    restoreResponses(m_vnresp, selected_rods);

    TraceStream(g_log, "") << "About to update properties" << '\n';

    // Update frames and such in the rod (Is this correct? Will this do some extra stuff?)
    for (RodSelectionType::const_iterator selected_rod = selected_rods.begin(); selected_rod != selected_rods.end(); selected_rod++)
        m_rods[*selected_rod]->updateProperties();

    // Sanity check to ensure rod's internal state is consistent
#ifndef NDEBUG
    for (RodSelectionType::const_iterator selected_rod = selected_rods.begin(); selected_rod != selected_rods.end(); selected_rod++)
        m_rods[*selected_rod]->verifyProperties();
#endif

    STOP_TIMER("BAGroomingStepper::step/setup");

    START_TIMER("BAGroomingStepper::step/explo");

    // Explosion detection
    std::vector<bool> exploding_rods(m_number_of_rods);
    if (m_perf_param.m_enable_explosion_detection)
    {
        TraceStream(g_log, "") << "About to detect explosions" << '\n';
        computeForces(m_endForces, selected_rods);
        checkExplosions(exploding_rods, failed_collisions_rods, selected_rods);
    }
    // Decide whether to substep or kill some rods

    STOP_TIMER("BAGroomingStepper::step/explo");

    // Check lengths again... Why is this necessary?
    checkLengths(stretching_rods);

    START_TIMER("BAGroomingStepper::step/exception");

    int rod_kill = 0;
    for (RodSelectionType::iterator rodit = selected_rods.begin(); rodit != selected_rods.end(); rodit++)
    {
        int rodidx = *rodit;

        bool solveFailure = !m_steppers[rodidx]->HasSolved();
        bool explosion = exploding_rods[rodidx];
        bool collisionFailure = failed_collisions_rods[rodidx];
        bool stretching = stretching_rods[rodidx];

        bool substep = (solveFailure && m_level < m_perf_param.m_solver.m_max_substeps) //
                || (explosion && m_level < m_perf_param.m_explosion.m_max_substeps) //
                || (collisionFailure && m_level < m_perf_param.m_collision.m_max_substeps) //
                || (stretching && m_level < m_perf_param.m_stretching.m_max_substeps);

        bool killRod = (solveFailure && m_perf_param.m_solver.m_in_case_of == FailureMode::KillTheRod) //
                || (explosion && m_perf_param.m_explosion.m_in_case_of == FailureMode::KillTheRod)//
                || (collisionFailure && m_perf_param.m_collision.m_in_case_of == FailureMode::KillTheRod) //
                || (stretching && m_perf_param.m_stretching.m_in_case_of == FailureMode::KillTheRod);

        bool haltSim = (solveFailure && m_perf_param.m_solver.m_in_case_of == FailureMode::HaltSimulation)//
                || (explosion && m_perf_param.m_explosion.m_in_case_of == FailureMode::HaltSimulation)//
                || (collisionFailure && m_perf_param.m_collision.m_in_case_of == FailureMode::HaltSimulation) //
                || (stretching && m_perf_param.m_stretching.m_in_case_of == FailureMode::HaltSimulation);

        if (substep) // Only in that case keep the rod in the selected list
            continue;
        else if (killRod)
        {
            // DEBUG
            if (solveFailure && m_perf_param.m_solver.m_in_case_of == FailureMode::KillTheRod)
                m_num_solver_killed++;
            if (explosion && m_perf_param.m_explosion.m_in_case_of == FailureMode::KillTheRod)
                m_num_explosion_killed++;
            if (collisionFailure && m_perf_param.m_collision.m_in_case_of == FailureMode::KillTheRod)
                m_num_collision_killed++;
            if (stretching && m_perf_param.m_stretching.m_in_case_of == FailureMode::KillTheRod)
                m_num_stretching_killed++;

            rod_kill++;
            killTheRod(*rodit);
        }
        else if (haltSim)
            m_simulationFailed = true;
	else if (m_useKineticDamping)
        {
	  // TODO (sainsley) : add flag check here
            //     std::cout << "treatment: accept this step as-is" << '\n';
            // at this point, the step is either successful, or includes only ignorable errors

            // Accept this step

            ElasticRod* rod = m_rods[rodidx];

            // std::cout << "KE[" << rodidx << "] = " << rod->computeKineticEnergy() << '\n';

            // Apply kinetic damping
            rod->recordKineticEnergy();
            if (rod->isKineticEnergyPeaked())
            {
	      // std::cout << "Zeroing energy for rod " << rodidx << '\n';
                for (int i = 0; i < rod->nv(); ++i)
		{
		    rod->setVelocity(i, Vec3d(0,0,0));
		}
	    }
        }

        selected_rods.erase(rodit--);
        // the -- compensates for the erased rod; this is dangerous since it assumes array (rather than linked-list) semantics for selected_rods
        // Yes I know, I'm surprised this even works.
    }

    STOP_TIMER("BAGroomingStepper::step/exception");

    if (rod_kill)
        NoticeStream(g_log, "") << "This step killed " << rod_kill << " rods\n";

    if (selected_rods.size() > 0)
        TraceStream(g_log, "") << "Step finished, " << selected_rods.size() << " rods must be substepped\n";
    else
        TraceStream(g_log, "") << "Step finished, all rods treated (either successful step, removed, or errors ignored)\n";

    if (m_killed_rods.size() > 0)
    {
        std::ostringstream ost;
        for (RodSelectionType::const_iterator rod = m_killed_rods.begin(); rod != m_killed_rods.end(); rod++)
            ost << *rod << ' ';
        DebugStream(g_log, "") << "List of rods killed: " << ost.str() << '\n';
    }

    START_TIMER("BAGroomingStepper::step/penalty");

    // Clean up penalty collisions list
    for (std::list<Collision*>::iterator i = penalty_collisions.begin(); i != penalty_collisions.end(); i++)
        delete *i;
    penalty_collisions.clear();
    // Clear existing penalties
    for (std::vector<RodPenaltyForce*>::const_iterator penalty_force = m_implicit_pnlty_forces.begin(); penalty_force
            != m_implicit_pnlty_forces.end(); penalty_force++)
        (*penalty_force)->clearProximityCollisions();

    STOP_TIMER("BAGroomingStepper::step/penalty");

    STOP_TIMER("BAGroomingStepper::step");
}

/*
 * Extracting/Restoring
 */
void BAGroomingStepper::extractPositions(VecXd& positions, const RodSelectionType& selected_rods, const double time) const
{
    assert(m_number_of_rods == m_base_dof_indices.size());
    assert(getNumDof() == positions.size());

    if (getNumDof() == 0)
        return;

#ifndef NDEBUG
    positions.setConstant(std::numeric_limits<double>::signaling_NaN());
#endif

    for (RodSelectionType::const_iterator rod = selected_rods.begin(); rod != selected_rods.end(); rod++)
        for (int j = 0; j < m_rods[*rod]->nv(); ++j)
        {
            assert(m_base_dof_indices[*rod] + 3 * j + 2 < positions.size());
            positions.segment<3> (m_base_dof_indices[*rod] + 3 * j) = m_rods[*rod]->getVertex(j);
        }

    assert(m_triangle_meshes.size() == m_base_triangle_indices.size());

    //    std::cerr << "positions.size() = " << positions.size() << '\n';
    //    std::cerr << "m_base_triangle_indices.size() = " << m_base_triangle_indices.size() << '\n';

    for (int i = 0; i < (int) m_triangle_meshes.size(); ++i)
    {
        int j = 0;
        for (TriangleMesh::vertex_iter vit = m_triangle_meshes[i]->vertices_begin(); vit
                != m_triangle_meshes[i]->vertices_end(); ++vit, ++j)
        {
            assert(m_base_triangle_indices[i] + 3 * j + 2 < positions.size());
            positions.segment<3> (m_base_triangle_indices[i] + 3 * j) = m_triangle_meshes[i]->getVertex(*vit);
        }
    }

    //    assert((positions.cwise() == positions).all());

    // Ensure boundary conditions loaded properly
#ifndef NDEBUG
    // For each rod in the selected list
    for (RodSelectionType::const_iterator rod = selected_rods.begin(); rod != selected_rods.end(); rod++)
    {
        RodBoundaryCondition* boundary = m_rods[*rod]->getBoundaryCondition();
        int rodbase = m_base_dof_indices[*rod];

        // For each vertex of the current rod, if that vertex has a prescribed position
        for (int j = 0; j < m_rods[*rod]->nv(); ++j)
            if (boundary->isVertexScripted(j))
            {
                //  std::cout << "BridsonTimeStepper is calling RodBoundaryCondition at m_t = " << m_t << '\n';
                Vec3d desiredposition = boundary->getDesiredVertexPosition(j, time);
                Vec3d actualvalue = positions.segment<3> (rodbase + 3 * j);
                assert(approxEq(desiredposition, actualvalue, 1.0e-6));
            }
    }
#endif
}

void BAGroomingStepper::extractVelocities(VecXd& velocities, const RodSelectionType& selected_rods) const
{
    assert(m_number_of_rods == m_base_dof_indices.size());
    assert(getNumDof() == velocities.size());

    if (getNumDof() == 0)
        return;

#ifndef NDEBUG
    velocities.setConstant(std::numeric_limits<double>::signaling_NaN());
#endif

    for (RodSelectionType::const_iterator rod = selected_rods.begin(); rod != selected_rods.end(); rod++)
        for (int j = 0; j < m_rods[*rod]->nv(); ++j)
        {
            assert(m_base_dof_indices[*rod] + 3 * j + 2 < velocities.size());
            velocities.segment<3> (m_base_dof_indices[*rod] + 3 * j) = m_rods[*rod]->getVelocity(j);
        }

    assert(m_triangle_meshes.size() == m_base_triangle_indices.size());
    for (int i = 0; i < (int) m_triangle_meshes.size(); ++i)
    {
        int j = 0;
        for (TriangleMesh::vertex_iter vit = m_triangle_meshes[i]->vertices_begin(); vit
                != m_triangle_meshes[i]->vertices_end(); ++vit, ++j)
        {
            assert(m_base_triangle_indices[i] + 3 * j + 2 < velocities.size());
            velocities.segment<3> (m_base_triangle_indices[i] + 3 * j) = Vec3d::Zero();
        }
    }

    //    assert((velocities.cwise() == velocities).all());
}

void BAGroomingStepper::restorePositions(const VecXd& positions, const RodSelectionType& selected_rods)
{
    assert(m_number_of_rods == m_base_dof_indices.size());

    for (RodSelectionType::const_iterator rod = selected_rods.begin(); rod != selected_rods.end(); rod++)
        for (int j = 0; j < m_rods[*rod]->nv(); ++j)
            if (!m_rods[*rod]->getBoundaryCondition()->isVertexScripted(j))
                m_rods[*rod]->setVertex(j, positions.segment<3> (m_base_dof_indices[*rod] + 3 * j));
}

void BAGroomingStepper::restoreVelocities(const VecXd& velocities, const RodSelectionType& selected_rods)
{
    assert(m_number_of_rods == m_base_dof_indices.size());

    for (RodSelectionType::const_iterator rod = selected_rods.begin(); rod != selected_rods.end(); rod++)
        for (int j = 0; j < m_rods[*rod]->nv(); ++j)
            if (!m_rods[*rod]->getBoundaryCondition()->isVertexScripted(j))
                m_rods[*rod]->setVelocity(j, velocities.segment<3> (m_base_dof_indices[*rod] + 3 * j));
}

void BAGroomingStepper::restoreResponses(const VecXd& responses, const RodSelectionType& selected_rods)
{
    assert(m_number_of_rods == m_base_dof_indices.size());

    for (RodSelectionType::const_iterator rod = selected_rods.begin(); rod != selected_rods.end(); rod++)
        for (int j = 0; j < m_rods[*rod]->nv(); ++j)
            if (!m_rods[*rod]->getBoundaryCondition()->isVertexScripted(j))
                m_rods[*rod]->setResponse(j, responses.segment<3> (m_base_dof_indices[*rod] + 3 * j));
}

/**
 * Enabling/disabling procedures
 */
void BAGroomingStepper::enableImplicitPenaltyImpulses()
{
    m_perf_param.m_enable_penalty_response = true;
    for (int i = 0; i < (int) m_number_of_rods; i++)
    {
        RodPenaltyForce *pnlty = new RodPenaltyForce();
        m_implicit_pnlty_forces.push_back(pnlty);
        m_steppers[i]->addExternalForce(pnlty);
    }

    TraceStream(g_log, "") << "Implicit penalty response is now enabled\n";

}

void BAGroomingStepper::disableImplicitPenaltyImpulses()
{
    m_perf_param.m_enable_penalty_response = false;

    for (int i = 0; i < (int) m_number_of_rods; i++)
    {
        std::vector<RodExternalForce*>& forces = m_steppers[i]->getExternalForces();
        for (int j = 0; j < (int) forces.size(); j++)
        {
            RodPenaltyForce* rod_penalty_force = dynamic_cast<RodPenaltyForce*> (forces[j]);
            if (rod_penalty_force)
            {
                forces.erase(forces.begin() + j);
                break;
            }
        }
    }
    m_implicit_pnlty_forces.clear();
}

void BAGroomingStepper::enableResponse()
{
    m_respns_enbld = true;
}

void BAGroomingStepper::disableResponse()
{
    m_respns_enbld = false;
}

void BAGroomingStepper::setNumInelasticIterations(const int& num_itr)
{
    assert(num_itr >= 0);
    m_perf_param.m_collision.m_max_iterations = num_itr;
}

void BAGroomingStepper::computeImmunity(const RodSelectionType& selected_rods)
{
    // Initially, the rods not treated in this step are collision-immune
    for (std::vector<bool>::iterator i = m_collision_immune.begin(); i != m_collision_immune.begin() + m_obj_start; i++)
        *i = true;
    for (RodSelectionType::const_iterator rod = selected_rods.begin(); rod != selected_rods.end(); rod++)
        if (m_collision_disabled_rods.find(*rod) == m_collision_disabled_rods.end()) // If the rod is not in the disabled set
            for (int j = 0; j < m_rods[*rod]->nv(); ++j)
                m_collision_immune[m_base_vtx_indices[*rod] + j] = false;

    // Find the initial vertex-face intersections and mark them collision-immune
    std::list<Collision*> collisions;
    m_collision_detector->getCollisions(collisions, EdgeFace);
    while (!collisions.empty())
    {
        EdgeFaceIntersection* intersection = dynamic_cast<EdgeFaceIntersection*> (collisions.front());
        collisions.pop_front();

        if (intersection)
        {
            m_collision_immune[intersection->v0] = true;
            // std::cerr << "BARodStepper::step: Vertex " << intersection->v0 << " has been marked collision-immune\n";
        }
    }
}

/**
 * Utilities
 */
double BAGroomingStepper::computeTotalForceNorm() const
{
    double totalforcenormsqr = 0.0;
    for (int i = 0; i < (int) m_number_of_rods; ++i)
    {
        VecXd force(m_rods[i]->ndof());
        force.setZero();
        m_rods[i]->computeForces(force);
        totalforcenormsqr += force.squaredNorm();
    }
    return sqrt(totalforcenormsqr);
}

void BAGroomingStepper::setDt(double dt)
{
    assert(dt > 0.0);
    m_dt = dt;

    // Set the timestep for the rod controllers
    for (int i = 0; i < (int) m_steppers.size(); ++i)
        m_steppers[i]->setTimeStep(dt);

    // Set the timestep for the scripted object controllers
    for (int i = 0; i < (int) m_scripting_controllers.size(); ++i)
        m_scripting_controllers[i]->setDt(dt);
}

void BAGroomingStepper::setTime(double time)
{
    m_t = time;
    // std::cout << "settingTime in BARodStepper to be " << m_t << '\n';

    // Set the time for the rod controllers
    for (int i = 0; i < (int) m_steppers.size(); ++i)
        m_steppers[i]->setTime(m_t);

    // Set the time for the scripted object controllers
    for (int i = 0; i < (int) m_scripting_controllers.size(); ++i)
        m_scripting_controllers[i]->setTime(m_t);
}

double BAGroomingStepper::getDt() const
{
    return m_dt;
}

double BAGroomingStepper::getTime() const
{
    //std::cout << "BARodStepper::getTime() = " << m_t << '\n';
    return m_t;
}

void BAGroomingStepper::skipRodRodCollisions(bool skipRodRodCollisions)
{
    TraceStream(g_log, "") << "Switching rod-rod collisions " << (skipRodRodCollisions ? "OFF" : "ON") << '\n';
    m_perf_param.m_skipRodRodCollisions = skipRodRodCollisions;

    if (m_collision_detector)
        m_collision_detector->setSkipRodRodCollisions(skipRodRodCollisions);
}

void BAGroomingStepper::setRodLabels(const std::vector<std::string>& rod_labels)
{
    assert(rod_labels.size() == m_number_of_rods);
    m_rod_labels = rod_labels;
}

int BAGroomingStepper::getContainingRod(int vert_idx) const
{
    assert(vert_idx >= 0);
    assert(vert_idx < getNumVerts());

    return upper_bound(m_base_vtx_indices.begin(), m_base_vtx_indices.end(), vert_idx) - m_base_vtx_indices.begin() - 1;
}

bool BAGroomingStepper::isRodVertex(int vert) const
{
    assert(vert >= 0);
    assert(vert < getNumVerts());

    // Is a vertex if index is less than start of object vertices in global array
    return vert < m_obj_start;
}

bool BAGroomingStepper::vertexAndFaceShareVertex(const int& v, const int& f0, const int& f1, const int& f2) const
{
    return v == f0 || v == f1 || v == f2;
}

bool BAGroomingStepper::vertexAndFaceShareVertex(const int& vertex, const int& face) const
{
    assert(face < (int) m_faces.size());
    assert(vertex >= 0);
    assert(vertex < getNumVerts());
    assert(m_faces[face].idx[0] >= 0);
    assert(m_faces[face].idx[0] < getNumVerts());
    assert(m_faces[face].idx[1] >= 0);
    assert(m_faces[face].idx[1] < getNumVerts());
    assert(m_faces[face].idx[2] >= 0);
    assert(m_faces[face].idx[2] < getNumVerts());

    return vertexAndFaceShareVertex(vertex, m_faces[face].idx[0], m_faces[face].idx[1], m_faces[face].idx[2]);
}

bool BAGroomingStepper::isProperCollisionTime(double time)
{
    if (time != time)
    {
        if (!m_nan_enc)
            DebugStream(g_log, "")
                    << "\033[31;1mWARNING IN BRIDSON STEPPER:\033[m Encountered NaN collision time from root finder. Supressing further messages of this type."
                    << '\n';
        m_nan_enc = true;
        return false;
    }
    if (time == std::numeric_limits<double>::infinity())
    {
        if (!m_inf_enc)
            DebugStream(g_log, "")
                    << "\033[31;1mWARNING IN BRIDSON STEPPER:\033[m Encountered INF collision time from root finder. Supressing further messages of this type."
                    << '\n';
        m_inf_enc = true;
        return false;
    }
    if (time < 0.0)
    {
        if (!m_lt0_enc)
            DebugStream(g_log, "") << "\033[31;1mWARNING IN BRIDSON STEPPER:\033[m Encountered scaled collision time " << time
                    << " less than 0.0. Supressing further messages of this type.\n";
        m_lt0_enc = true;
        return false;
    }
    if (time > 1.0)
    {
        if (!m_gt0_enc)
            DebugStream(g_log, "") << "\033[31;1mWARNING IN BRIDSON STEPPER:\033[m Encountered scaled collision time " << time
                    << " greater than 1.0. Supressing further messages of this type.\n";
        m_gt0_enc = true;
        return false;
    }
    return true;
}

int BAGroomingStepper::getNumDof() const
{
    assert(m_num_dof >= 0);
    return m_num_dof;
}

int BAGroomingStepper::getNumVerts() const
{
    assert(m_num_dof % 3 == 0);
    return m_num_dof / 3;
}

// Ensures each rod edge has circular cross section.
void BAGroomingStepper::ensureCircularCrossSection(const ElasticRod& rod) const
{
    // Ensure circular cross section
    for (int i = 0; i < (int) rod.ne(); ++i)
    {
        if (rod.getRadiusScale() * rod.radiusA(i) != rod.getRadiusScale() * rod.radiusB(i))
        {
            DebugStream(g_log, "")
                    << "\033[31;1mWARNING IN BRIDSON STEPPER:\033[m Contact currently not supported for non-circular cross sections. Assuming circular cross section."
                    << '\n';
        }
    }
}

// Ensures each internal rod edge has length less than sum of neighbors' radii.
void BAGroomingStepper::ensureNoCollisionsByDefault(const ElasticRod& rod) const
{
    // Ensure "non-attached" edges are not colliding by default
    for (int i = 1; i < (int) rod.ne() - 1; ++i)
    {
        double edgelen = rod.getEdge(i).norm();
        double radsum = rod.getRadiusScale() * rod.radiusA(i - 1) + rod.getRadiusScale() * rod.radiusA(i + 1);
        if (edgelen <= radsum)
        {
            DebugStream(g_log, "")
                    << "\033[31;1mWARNING IN BRIDSON STEPPER:\033[m Detected edges that collide by default. Instabilities may result."
                    << '\n';
        }
    }
}

void BAGroomingStepper::killTheRod(int rod) // TODO: remove the rod properly in Maya
{
    m_killed_rods.push_back(rod);
    for (int j = 0; j < m_rods[rod]->nv(); j++)
        m_rods[rod]->setVertex(j, 0 * m_rods[rod]->getVertex(j));
    m_simulated_rods.erase(find(m_simulated_rods.begin(), m_simulated_rods.end(), rod));

#ifndef KEEP_ONLY_SOME_RODS
    assert(m_simulated_rods.size() + m_killed_rods.size() == m_number_of_rods);
#endif
}

void BAGroomingStepper::computeForces(std::vector<VecXd*> Forces, const RodSelectionType& selected_rods)
{
    for (RodSelectionType::const_iterator rod = selected_rods.begin(); rod != selected_rods.end(); rod++)
    {
        Forces[*rod]->setZero();
        //m_rods[*rod]->computeForces(*Forces[*rod]);
        m_steppers[*rod]->evaluatePDot(*Forces[*rod]);
    }
}

void BAGroomingStepper::addRod(ElasticRod* rod, GroomingTimeStepper* stepper)
{
    // Add the rod
    m_rods.push_back(rod);
    m_simulated_rods.push_back(m_number_of_rods);
    m_rods.back()->globalRodIndex = m_number_of_rods++;
    // Add the stepper
    m_steppers.push_back(stepper);
    m_steppers.back()->setMaxIterations(m_perf_param.m_solver.m_max_iterations);

    assert(m_rods.size() == m_steppers.size());
    assert(m_number_of_rods == m_rods.size());

    // Extend the force vectors used to detect explosions
    m_startForces.push_back(new VecXd(m_rods.back()->ndof()));
    m_preDynamicForces.push_back(new VecXd(m_rods.back()->ndof()));
    m_preCollisionForces.push_back(new VecXd(m_rods.back()->ndof()));
    m_endForces.push_back(new VecXd(m_rods.back()->ndof()));

    m_rodbackups.resize(m_number_of_rods); // growing by 1
    m_rodbackups.back().resize(*m_rods.back());

    // Extract edges from the new rod
    for (int j = 0; j < m_rods.back()->nv() - 1; ++j)
    {
        m_edges.push_back(std::pair<int, int>(getNumVerts() + j, getNumVerts() + j + 1));
        assert(m_rods.back()->getRadiusScale() * m_rods.back()->radiusA(j) > 0.0);
        m_edge_radii.push_back(m_rods.back()->getRadiusScale() * m_rods.back()->radiusA(j));
    }
    assert(m_edges.size() == m_edge_radii.size());

    for (int j = 0; j < m_rods.back()->nv() - 1; ++j)
    {
        assert(m_rods.back()->getRadiusScale() * m_rods.back()->radiusA(j) > 0.0);
        // Radii associated with edges ... what to do if at a vertex with two radii? Average?
        m_vertex_radii.push_back(m_rods.back()->getRadiusScale() * m_rods.back()->radiusA(j));
    }
    m_vertex_radii.push_back(m_vertex_radii.back()); // < TODO: What the $^#! is this call?

    // Update vector that tracks the rod DOF in the system
    m_base_dof_indices.push_back(getNumDof());
    m_base_vtx_indices.push_back(getNumDof() / 3);

    // Extract masses from the new rod
    for (ElasticRod::vertex_iter itr = m_rods.back()->vertices_begin(); itr != m_rods.back()->vertices_end(); ++itr)
    {
        assert(m_rods.back()->getVertexMass(*itr, -1) > 0.0);
        m_masses.push_back(m_rods.back()->getVertexMass(*itr, -1));
    }

    // Update total number of DOF in the system
    m_num_dof += 3 * m_rods.back()->nv();

    assert((int) m_masses.size() == getNumVerts());
    assert((int) m_vertex_radii.size() == getNumVerts());

    // Resize the internal storage
    m_xn.resize(getNumDof());
    m_xnp1.resize(getNumDof());
    m_xdebug.resize(getNumDof());
    m_vnphalf.resize(getNumDof());

    m_collision_immune.resize(getNumVerts());

    if (m_perf_param.m_enable_penalty_response)
        enableImplicitPenaltyImpulses(); // TODO: probably a memory leak to fix here.

    // Update the rods in the collision detector
    m_collision_detector->rebuildRodElements(m_edges);

    // Initial length of the recently added rod
    m_initialLengths.push_back(0);
    for (int j = 1; j < m_rods.back()->nv(); j++)
        m_initialLengths.back() += (m_rods.back()->getVertex(j) - m_rods.back()->getVertex(j - 1)).norm();

}

void BAGroomingStepper::removeRod(int rodIdx)
{
    killTheRod(rodIdx);
}

}
