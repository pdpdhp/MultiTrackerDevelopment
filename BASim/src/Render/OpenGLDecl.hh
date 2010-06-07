/**
 * \file OpenGLDecl.hh
 *
 * \author miklos@cs.columbia.edu
 * \date 09/22/2009
 */

#ifndef OPENGLDECL_HH
#define OPENGLDECL_HH


/// OpenGL declarations for using OpenGL in C++.
///
/// Mostly useful for situations where the base type is not known, for example,
/// your project uses floating point types called "Real", which is typedef'd to
/// either float or double.  At rendering time, you cannot know which of
/// glVertex3f or glVertex3d to call.  This set of declarations solves this
/// problem.
///
/// Note that this could be achieved using templated functions, but it
/// would still be rather tedious and templates can slow down the compiler
/// at times.
///
/// The following types of functions are provided:
///
/// vertex() overloaded with 2, 3, or 4 parameters of type GLint, GLshort,
/// GLfloat and GLdouble.
///
/// vertexN(), where N is one of 2, 3, or 4 and takes a single const
/// pointer of one of the vertex() types.
///
/// color() overloaded with 3 or 4 parameters of type GLbyte, GLint, GLshort,
/// GLfloat, GLdouble, GLubyte, GLushort, or GLuint.
///
/// colorN(), where N is one of 3 or 4 and takes a single const pointer of one
/// of the color() types.
///
/// normal() overloaded with 3 parameters of type GLbyte, GLint, GLshort,
/// GLfloat, or GLdouble.
///
/// normal() with a single const pointer of one of the color() types pointing
/// to three values.
///
/// template getTypeEnum<T>() returns the OpenGL type enumeration value for
/// the type passed in.  For example, getTypeEnum<float>() will return GL_FLOAT
/// and getTypeEnum<Real>() will return GL_FLOAT if Real is a float, GL_DOUBLE
/// if Real is a double, etc.
///
/// This file was automatically generated by genOpenGLHeader.py.  Do not edit
/// directly, change the script and regenerate!  Copyright 2006 Adrian Secord
/// http://stippling.org

#include "OpenGLHeaders.hh"
#include <cassert>

/// OpenGL declarations for using OpenGL with unknown types.
namespace OpenGL {

/// glVertex with 2 components of type GLint
inline void vertex(GLint x, GLint y) {
  glVertex2i(x, y);
}

/// glVertex with 2 components of type GLshort
inline void vertex(GLshort x, GLshort y) {
  glVertex2s(x, y);
}

/// glVertex with 2 components of type GLdouble
inline void vertex(GLdouble x, GLdouble y) {
  glVertex2d(x, y);
}

/// glVertex with 2 components of type GLfloat
inline void vertex(GLfloat x, GLfloat y) {
  glVertex2f(x, y);
}

/// glVertex with 3 components of type GLint
inline void vertex(GLint x, GLint y, GLint z) {
  glVertex3i(x, y, z);
}

/// glVertex with 3 components of type GLshort
inline void vertex(GLshort x, GLshort y, GLshort z) {
  glVertex3s(x, y, z);
}

/// glVertex with 3 components of type GLdouble
inline void vertex(GLdouble x, GLdouble y, GLdouble z) {
  glVertex3d(x, y, z);
}

/// glVertex with 3 components of type GLfloat
inline void vertex(GLfloat x, GLfloat y, GLfloat z) {
  glVertex3f(x, y, z);
}

/// glVertex with 4 components of type GLint
inline void vertex(GLint x, GLint y, GLint z, GLint w) {
  glVertex4i(x, y, z, w);
}

/// glVertex with 4 components of type GLshort
inline void vertex(GLshort x, GLshort y, GLshort z, GLshort w) {
  glVertex4s(x, y, z, w);
}

/// glVertex with 4 components of type GLdouble
inline void vertex(GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
  glVertex4d(x, y, z, w);
}

/// glVertex with 4 components of type GLfloat
inline void vertex(GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
  glVertex4f(x, y, z, w);
}

/// glVertex with pointer to 2 components of type GLint
inline void vertex2(const GLint* p) {
  glVertex2iv(p);
}

/// glVertex with pointer to 2 components of type GLshort
inline void vertex2(const GLshort* p) {
  glVertex2sv(p);
}

/// glVertex with pointer to 2 components of type GLdouble
inline void vertex2(const GLdouble* p) {
  glVertex2dv(p);
}

/// glVertex with pointer to 2 components of type GLfloat
inline void vertex2(const GLfloat* p) {
  glVertex2fv(p);
}

/// glVertex with pointer to 3 components of type GLint
inline void vertex3(const GLint* p) {
  glVertex3iv(p);
}

/// glVertex with pointer to 3 components of type GLshort
inline void vertex3(const GLshort* p) {
  glVertex3sv(p);
}

/// glVertex with pointer to 3 components of type GLdouble
inline void vertex3(const GLdouble* p) {
  glVertex3dv(p);
}

/// glVertex with pointer to 3 components of type GLfloat
inline void vertex3(const GLfloat* p) {
  glVertex3fv(p);
}

/// glVertex with pointer to 4 components of type GLint
inline void vertex4(const GLint* p) {
  glVertex4iv(p);
}

/// glVertex with pointer to 4 components of type GLshort
inline void vertex4(const GLshort* p) {
  glVertex4sv(p);
}

/// glVertex with pointer to 4 components of type GLdouble
inline void vertex4(const GLdouble* p) {
  glVertex4dv(p);
}

/// glVertex with pointer to 4 components of type GLfloat
inline void vertex4(const GLfloat* p) {
  glVertex4fv(p);
}

/// glVertex passed in a vector
template <typename Vector>
inline void vertex(const Vector& x) {
  if (x.size() == 2) vertex2(x.data());
  else if (x.size() == 3) vertex3(x.data());
  else if (x.size() == 4) vertex4(x.data());
}

/// glColor with 3 components of type GLbyte
inline void color(GLbyte r, GLbyte g, GLbyte b) {
  glColor3b(r, g, b);
}

/// glColor with 3 components of type GLdouble
inline void color(GLdouble r, GLdouble g, GLdouble b) {
  glColor3d(r, g, b);
}

/// glColor with 3 components of type GLfloat
inline void color(GLfloat r, GLfloat g, GLfloat b) {
  glColor3f(r, g, b);
}

/// glColor with 3 components of type GLint
inline void color(GLint r, GLint g, GLint b) {
  glColor3i(r, g, b);
}

/// glColor with 3 components of type GLushort
inline void color(GLushort r, GLushort g, GLushort b) {
  glColor3us(r, g, b);
}

/// glColor with 3 components of type GLshort
inline void color(GLshort r, GLshort g, GLshort b) {
  glColor3s(r, g, b);
}

/// glColor with 3 components of type GLuint
inline void color(GLuint r, GLuint g, GLuint b) {
  glColor3ui(r, g, b);
}

/// glColor with 3 components of type GLubyte
inline void color(GLubyte r, GLubyte g, GLubyte b) {
  glColor3ub(r, g, b);
}

/// glColor with 4 components of type GLbyte
inline void color(GLbyte r, GLbyte g, GLbyte b, GLbyte a) {
  glColor4b(r, g, b, a);
}

/// glColor with 4 components of type GLdouble
inline void color(GLdouble r, GLdouble g, GLdouble b, GLdouble a) {
  glColor4d(r, g, b, a);
}

/// glColor with 4 components of type GLfloat
inline void color(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
  glColor4f(r, g, b, a);
}

/// glColor with 4 components of type GLint
inline void color(GLint r, GLint g, GLint b, GLint a) {
  glColor4i(r, g, b, a);
}

/// glColor with 4 components of type GLushort
inline void color(GLushort r, GLushort g, GLushort b, GLushort a) {
  glColor4us(r, g, b, a);
}

/// glColor with 4 components of type GLshort
inline void color(GLshort r, GLshort g, GLshort b, GLshort a) {
  glColor4s(r, g, b, a);
}

/// glColor with 4 components of type GLuint
inline void color(GLuint r, GLuint g, GLuint b, GLuint a) {
  glColor4ui(r, g, b, a);
}

/// glColor with 4 components of type GLubyte
inline void color(GLubyte r, GLubyte g, GLubyte b, GLubyte a) {
  glColor4ub(r, g, b, a);
}

/// glColor with pointer to 3 components of type GLbyte
inline void color3(const GLbyte* p) {
  glColor3bv(p);
}

/// glColor with pointer to 3 components of type GLdouble
inline void color3(const GLdouble* p) {
  glColor3dv(p);
}

/// glColor with pointer to 3 components of type GLfloat
inline void color3(const GLfloat* p) {
  glColor3fv(p);
}

/// glColor with pointer to 3 components of type GLint
inline void color3(const GLint* p) {
  glColor3iv(p);
}

/// glColor with pointer to 3 components of type GLushort
inline void color3(const GLushort* p) {
  glColor3usv(p);
}

/// glColor with pointer to 3 components of type GLshort
inline void color3(const GLshort* p) {
  glColor3sv(p);
}

/// glColor with pointer to 3 components of type GLuint
inline void color3(const GLuint* p) {
  glColor3uiv(p);
}

/// glColor with pointer to 3 components of type GLubyte
inline void color3(const GLubyte* p) {
  glColor3ubv(p);
}

/// glColor with pointer to 4 components of type GLbyte
inline void color4(const GLbyte* p) {
  glColor4bv(p);
}

/// glColor with pointer to 4 components of type GLdouble
inline void color4(const GLdouble* p) {
  glColor4dv(p);
}

/// glColor with pointer to 4 components of type GLfloat
inline void color4(const GLfloat* p) {
  glColor4fv(p);
}

/// glColor with pointer to 4 components of type GLint
inline void color4(const GLint* p) {
  glColor4iv(p);
}

/// glColor with pointer to 4 components of type GLushort
inline void color4(const GLushort* p) {
  glColor4usv(p);
}

/// glColor with pointer to 4 components of type GLshort
inline void color4(const GLshort* p) {
  glColor4sv(p);
}

/// glColor with pointer to 4 components of type GLuint
inline void color4(const GLuint* p) {
  glColor4uiv(p);
}

/// glColor with pointer to 4 components of type GLubyte
inline void color4(const GLubyte* p) {
  glColor4ubv(p);
}

/// glColor passed in a vector
template <typename Vector>
inline void color(const Vector& x) {
  if (x.size() == 3) color3(x.data());
  else if (x.size() == 4) color4(x.data());
}

/// glNormal with type GLint
inline void normal(GLint x, GLint y, GLint z) {
  glNormal3i(x, y, z);
}

/// glNormal with type GLshort
inline void normal(GLshort x, GLshort y, GLshort z) {
  glNormal3s(x, y, z);
}

/// glNormal with type GLbyte
inline void normal(GLbyte x, GLbyte y, GLbyte z) {
  glNormal3b(x, y, z);
}

/// glNormal with type GLdouble
inline void normal(GLdouble x, GLdouble y, GLdouble z) {
  glNormal3d(x, y, z);
}

/// glNormal with type GLfloat
inline void normal(GLfloat x, GLfloat y, GLfloat z) {
  glNormal3f(x, y, z);
}

/// glNormal with pointer to three components of type GLint
inline void normal(const GLint* p) {
  glNormal3iv(p);
}

/// glNormal with pointer to three components of type GLshort
inline void normal(const GLshort* p) {
  glNormal3sv(p);
}

/// glNormal with pointer to three components of type GLbyte
inline void normal(const GLbyte* p) {
  glNormal3bv(p);
}

/// glNormal with pointer to three components of type GLdouble
inline void normal(const GLdouble* p) {
  glNormal3dv(p);
}

/// glNormal with pointer to three components of type GLfloat
inline void normal(const GLfloat* p) {
  glNormal3fv(p);
}

/// glNormal passed in a vector
template <typename Vector>
inline void normal(const Vector& x) {
  if (x.size() == 3) normal(x.data());
}

/// Return the enumeration corresponding to an OpenGL type
template <typename T> inline int getTypeEnum() {
  assert(!"getTypeEnum undefined for this type.");
  return -1;
}

/// Return the enumeration corresponding to GLint
template <> inline int getTypeEnum<GLint>() {
  return GL_INT;
}

/// Return the enumeration corresponding to GLushort
template <> inline int getTypeEnum<GLushort>() {
  return GL_UNSIGNED_SHORT;
}

/// Return the enumeration corresponding to GLshort
template <> inline int getTypeEnum<GLshort>() {
  return GL_SHORT;
}

/// Return the enumeration corresponding to GLfloat
template <> inline int getTypeEnum<GLfloat>() {
  return GL_FLOAT;
}

/// Return the enumeration corresponding to GLdouble
template <> inline int getTypeEnum<GLdouble>() {
  return GL_DOUBLE;
}

/// Return the enumeration corresponding to GLbyte
template <> inline int getTypeEnum<GLbyte>() {
  return GL_BYTE;
}

/// Return the enumeration corresponding to GLuint
template <> inline int getTypeEnum<GLuint>() {
  return GL_UNSIGNED_INT;
}

/// Return the enumeration corresponding to GLubyte
template <> inline int getTypeEnum<GLubyte>() {
  return GL_UNSIGNED_BYTE;
}

} // namespace OpenGL

#endif // OPENGLDECL_HH
