/**
 * \file Definitions.hh
 *
 * \author miklos@cs.columbia.edu
 * \date 08/31/2009
 */

#ifndef DEFINITIONS_HH
#define DEFINITIONS_HH

#include "EigenIncludes.hh"
#include "STLIncludes.hh"

namespace BASim {

typedef double Scalar; ///< the scalar type

typedef std::vector<Scalar> ScalarArray; ///< an array of scalars
typedef std::vector<int> IntArray; ///< an array of ints
typedef std::vector<unsigned int> UIntArray; ///< an array of unsigned ints
typedef Eigen::Matrix<int, Eigen::Dynamic, 1> IndexArray; ///< array storing indices

typedef Eigen::Matrix<Scalar, 2, 1> Vec2d; ///< 2d scalar vector
typedef Eigen::Matrix<Scalar, 3, 1> Vec3d; ///< 3d scalar vector
typedef Eigen::Matrix<Scalar, 4, 1> Vec4d; ///< 3d scalar vector
typedef Eigen::Matrix<int, 2, 1> Vec2i; ///< 2d int vector
typedef Eigen::Matrix<int, 3, 1> Vec3i; ///< 3d int vector
typedef Eigen::Matrix<int, 4, 1> Vec4i; ///< 4d int vector
typedef std::vector<Vec2d> Vec2dArray; ///< an array of 2d scalar vectors
typedef std::vector<Vec3d> Vec3dArray; ///< an array of 3d scalar vectors

typedef Eigen::Matrix<Scalar, Eigen::Dynamic, 1> VecXd; ///< arbitrary dimension scalar vector

typedef Eigen::Matrix<Scalar, 2, 2> Mat2d; ///< 2x2 scalar matrix
typedef Eigen::Matrix<Scalar, 3, 3> Mat3d; ///< 3x3 scalar matrix
typedef Eigen::Matrix<Scalar, 4, 4> Mat4d; ///< 4x4 scalar matrix
typedef std::vector<Mat2d> Mat2dArray; ///< an array of 2x2 scalar matrices
typedef std::vector<Mat3d> Mat3dArray; ///< an array of 3x3 scalar matrices

typedef Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> MatXd; ///< arbitrary dimension scalar matrix

typedef Eigen::Quaternion<Scalar> Quaternion;


} // namespace BASim

#endif // DEFINITIONS_HH
