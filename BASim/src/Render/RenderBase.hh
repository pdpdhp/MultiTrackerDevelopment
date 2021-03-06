/**
 * \file RenderBase.hh
 *
 * \author miklos@cs.columbia.edu
 * \date 08/30/2009
 */

#ifndef RENDERBASE_HH
#define RENDERBASE_HH

#include <vector>
#include "OpenGLHeaders.hh"
#include "../Core/Definitions.hh"

namespace BASim {

/** Interface for all renderers. */
class RenderBase
{
public:

  RenderBase()
    : m_enableVertices(false)
    , m_enableNormals(false)
    , m_enableColors(false)
  {
    m_enableVertices = true;

    vertices.push_back(0);
    vertices.push_back(0);
    vertices.push_back(0);

    vertices.push_back(1);
    vertices.push_back(0);
    vertices.push_back(0);

    vertices.push_back(0.5);
    vertices.push_back(1);
    vertices.push_back(0);

    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);
  }

  virtual ~RenderBase() {}

  virtual void render();

  bool& enableVertices() { return m_enableVertices; }
  bool& enableNormals() { return m_enableNormals; }
  bool& enableColors() { return m_enableColors; }

  virtual Vec3d calculateObjectCenter() = 0;
  virtual Scalar calculateObjectBoundingRadius(const Vec3d& center) = 0;

  virtual void cycleMode(int inc = 1) {}

  virtual void keyboard(unsigned char key, int x, int y) {}

protected:

  void setNormalArray();
  void unsetNormalArray();
  void setColorArray();
  void unsetColorArray();
  void drawVertices();

  std::vector<GLfloat> vertices;
  std::vector<GLfloat> normals;
  std::vector<GLubyte> colors;
  std::vector<GLuint> indices;

  bool m_enableVertices;
  bool m_enableNormals;
  bool m_enableColors;
};

} // namespace BASim

#endif // RENDERBASE_HH
