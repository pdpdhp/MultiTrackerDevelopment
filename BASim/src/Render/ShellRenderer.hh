/**
 * \file DefoObjRenderer.hh
 *
 * \author batty@cs.columbia.edu
 * \date April 20, 2011
 */

#ifndef SHELLRENDERER_H
#define SHELLRENDERER_H

#include "BASim/src/Physics/DeformableObjects/Shells/ElasticShell.hh"
#include "BASim/src/Render/RenderBase.hh"

namespace BASim {

  /** Class that implements OpenGL rendering for elastic shells. Mimics Breannan's triangle mesh code. */
  class ShellRenderer : public RenderBase
  {
  public:
  
    enum DrawMode { NONE, DBG, DBG_BUBBLE, DBG_JUNCTION, DBG_MULTIPHASE, FLAT, SMOOTH, NUM_MODE };

    ShellRenderer( ElasticShell& shell );
    
    void render();
    void renderEdges();
    void renderVelocity();
    void cycleMode(int inc = 1);
    DrawMode getMode() const { return m_mode; }
    void setMode(DrawMode mode) { m_mode = mode; }
    
    virtual Vec3d calculateObjectCenter();
    virtual Scalar calculateObjectBoundingRadius(const Vec3d& center);
    
    void keyboard(unsigned char key, int x, int y);
    
    void turnOffAllRegions() { for (size_t i = 0; i < m_region_visible.size(); i++) m_region_visible[i] = false; }
    void turnOnRegion(int i) { m_region_visible[i] = true; }

  protected:
    ElasticShell& m_shell;
    DrawMode m_mode;
//    const Scalar m_refthickness;
    
    // multiphase mode
    int m_current_region;
    int m_nregion;
    std::vector<bool> m_region_visible;
    bool m_solid_boundary_visible;

  };
  
} // namespace BASim

#endif //DEFOOBJRENDERER_H
