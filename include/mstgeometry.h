#ifndef _MSTGEOMETRY_H
#define _MSTGEOMETRY_H

/* More complex geometry than in "msttypes.h", which often includes some linear
 * algebra and requires additional #includes and capabilities. */

#include "mstlinalg.h"
#include <chrono>

using namespace MST;

class MstGeometry {
  public:
    /* gradient vector must be of length 6, and will be filled with partial
     * derivatives d(distance)/dc, where c runs over x, y, and z of the first
     * atom, then second atom. */
    template <class T>
    static mstreal distance(const CartesianPoint& atom1, const CartesianPoint& atom2, T& grad);

    /* gradient vector must be of length 9, and will be filled with partial
     * derivatives d(distance)/dc, where c runs over x, y, and z of the first
     * atom, then second atom, then third atom. */
    template <class T>
    static mstreal angle(const CartesianPoint& atom1, const CartesianPoint& atom2, const CartesianPoint& atom3, T& grad);

    /* gradient vector must be of length 12, and will be filled with partial
     * derivatives d(distance)/dc, where c runs over x, y, and z of the first
     * atom, then second atom, then third atom, then fourth atom. */
    template <class T>
    static mstreal dihedral(const CartesianPoint& atom1, const CartesianPoint& atom2, const CartesianPoint& atom3, const CartesianPoint& atom4, T& grad);

    template <class T>
    static mstreal qcpRMSD(const T& A, const T& B);

    /* Tests implementation of analytical gradients of bond, angle, and dihedral
     * using finite difference for comparison. */
    static bool testPrimitiveGradients();

    /* Tests QCP implementation for RMSD and RMSD gradient calculation. */
    static bool testQCP();

  protected:
    template <class T>
    static T& ref(T& obj) { return obj; }
    template <class T>
    static T& ref(T* obj) { return *obj; }
};

// see derivation in http://grigoryanlab.org/docs/dynamics_derivatives.pdf
template <class T>
mstreal MstGeometry::distance(const CartesianPoint& atom1, const CartesianPoint& atom2, T& grad) {
  mstreal x12 = atom1.getX() - atom2.getX();
  mstreal y12 = atom1.getY() - atom2.getY();
  mstreal z12 = atom1.getZ() - atom2.getZ();
  mstreal d = sqrt(x12*x12 + y12*y12 + z12*z12);
  if (MstUtils::closeEnough(d, 0.0)) {
    // by convention, set gradient to unity when the two atoms are on top of each other
    for (int i = 0; i < grad.size(); i++) grad[i] = 1.0;
  } else {
    grad[0] = x12/d;
    grad[1] = y12/d;
    grad[2] = z12/d;
    grad[3] = -grad[0];
    grad[4] = -grad[1];
    grad[5] = -grad[2];
  }
  return d;
}

// see derivation in http://grigoryanlab.org/docs/dynamics_derivatives.pdf
template <class T>
mstreal MstGeometry::angle(const CartesianPoint& atom1, const CartesianPoint& atom2, const CartesianPoint& atom3, T& grad) {
  mstreal x12 = atom1.getX() - atom2.getX();
  mstreal y12 = atom1.getY() - atom2.getY();
  mstreal z12 = atom1.getZ() - atom2.getZ();
  mstreal x32 = atom3.getX() - atom2.getX();
  mstreal y32 = atom3.getY() - atom2.getY();
  mstreal z32 = atom3.getZ() - atom2.getZ();
  mstreal L1 = sqrt(x12*x12 + y12*y12 + z12*z12);
  mstreal L2 = sqrt(x32*x32 + y32*y32 + z32*z32);
  if (MstUtils::closeEnough(L1, 0.0) || MstUtils::closeEnough(L2, 0.0)) {
    // angle is not really defined, when two subsequent atoms coicide, so just
    // set the derivative to something for the sake of code stability
    for (int i = 0; i < grad.size(); i++) grad[i] = 0.0;
  }
  mstreal p = x12*x32 + y12*y32 + z12*z32;
  mstreal d = p/(L1*L2);
  if (MstUtils::closeEnough(fabs(d), 1.0)) {
    // signs are arbitrary in this case (set to + by convention)
    grad[0] = (sqrt(y12*y12 + z12*z12)/L1)/L1;
    grad[1] = (sqrt(x12*x12 + z12*z12)/L1)/L1;
    grad[2] = (sqrt(x12*x12 + y12*y12)/L1)/L1;
    grad[6] = (sqrt(y32*y32 + z32*z32)/L2)/L2;
    grad[7] = (sqrt(x32*x32 + z32*z32)/L2)/L2;
    grad[8] = (sqrt(x32*x32 + y32*y32)/L2)/L2;
  } else {
    mstreal L12 = L1*L2;
    mstreal C = -1/(sqrt(1 - d*d)*L12*L12);
    mstreal pL12i = p*L1/L2;
    mstreal pL21i = p*L2/L1;
    grad[0] = C*(x32*L12 - pL21i*x12);
    grad[1] = C*(y32*L12 - pL21i*y12);
    grad[2] = C*(z32*L12 - pL21i*z12);
    grad[6] = C*(x12*L12 - pL12i*x32);
    grad[7] = C*(y12*L12 - pL12i*y32);
    grad[8] = C*(z12*L12 - pL12i*z32);
  }

  // gardient with respect to the middle point is always the minus sum of that
  // of the first and the third points
  grad[3] = -(grad[0] + grad[6]);
  grad[4] = -(grad[1] + grad[7]);
  grad[5] = -(grad[2] + grad[8]);
  return acos(d);
}

// see derivation in http://grigoryanlab.org/docs/dynamics_derivatives.pdf
template <class T>
mstreal MstGeometry::dihedral(const CartesianPoint& atom1, const CartesianPoint& atom2, const CartesianPoint& atom3, const CartesianPoint& atom4, T& grad) {
  mstreal x21 = atom2.getX() - atom1.getX();
  mstreal y21 = atom2.getY() - atom1.getY();
  mstreal z21 = atom2.getZ() - atom1.getZ();
  mstreal x32 = atom3.getX() - atom2.getX();
  mstreal y32 = atom3.getY() - atom2.getY();
  mstreal z32 = atom3.getZ() - atom2.getZ();
  mstreal x43 = atom4.getX() - atom3.getX();
  mstreal y43 = atom4.getY() - atom3.getY();
  mstreal z43 = atom4.getZ() - atom3.getZ();
  mstreal x31 = atom3.getX() - atom1.getX();
  mstreal y31 = atom3.getY() - atom1.getY();
  mstreal z31 = atom3.getZ() - atom1.getZ();
  mstreal x42 = atom4.getX() - atom2.getX();
  mstreal y42 = atom4.getY() - atom2.getY();
  mstreal z42 = atom4.getZ() - atom2.getZ();

  CartesianPoint N1(z21*y32 - y21*z32, x21*z32 - z21*x32, y21*x32 - x21*y32);
  CartesianPoint N2(y43*z32 - z43*y32, z43*x32 - x43*z32, x43*y32 - y43*x32);
  CartesianPoint angleGrad(9);
  mstreal th = MstGeometry::angle(N1, CartesianPoint(0, 0, 0), N2, angleGrad);

  grad[0]  = -angleGrad[1]*z32 + angleGrad[2]*y32;
  grad[1]  =  angleGrad[0]*z32 - angleGrad[2]*x32;
  grad[2]  = -angleGrad[0]*y32 + angleGrad[1]*x32;

  grad[3]  =  angleGrad[1]*z31 - angleGrad[2]*y31 - angleGrad[7]*z43 + angleGrad[8]*y43;
  grad[4]  = -angleGrad[0]*z31 + angleGrad[2]*x31 + angleGrad[6]*z43 - angleGrad[8]*x43;
  grad[5]  =  angleGrad[0]*y31 - angleGrad[1]*x31 - angleGrad[6]*y43 + angleGrad[7]*x43;

  grad[6]  = -angleGrad[1]*z21 + angleGrad[2]*y21 + angleGrad[7]*z42 - angleGrad[8]*y42;
  grad[7]  =  angleGrad[0]*z21 - angleGrad[2]*x21 - angleGrad[6]*z42 + angleGrad[8]*x42;
  grad[8]  = -angleGrad[0]*y21 + angleGrad[1]*x21 + angleGrad[6]*y42 - angleGrad[7]*x42;

  grad[9]  = -angleGrad[7]*z32 + angleGrad[8]*y32;
  grad[10] =  angleGrad[6]*z32 - angleGrad[8]*x32;
  grad[11] = -angleGrad[6]*y32 + angleGrad[7]*x32;

  if (N1 * CartesianPoint(x43, y43, z43) > 0) {
    for (int i = 0; i < grad.size(); i++) grad[i] = -grad[i];
    th = -th;
  }

  return th;
}

// QCP algorithm from: http://onlinelibrary.wiley.com/doi/10.1002/jcc.21439/epdf
template <class T>
mstreal MstGeometry::qcpRMSD(const T& A, const T& B) {
  int i, j;
  int N = A.size();
  if (N != B.size()) MstUtils::error("structures are of different length", "MstGeometry::qcpRMSD");

  //compute centers for vector sets x, y
  vector<mstreal> cA(3, 0.0), cB(3, 0.0);
  for (i = 0; i < N; i++){
    for (j = 0; j < 3; j++) {
      cA[j] += ref(A[i])[j];
      cB[j] += ref(B[i])[j];
    }
  }
  for (i = 0; i < 3; i++){
    cA[i] = cA[i]/N;
    cB[i] = cB[i]/N;
  }

  // compute the correlation matrix S and the inner products of the two structures
  vector<vector<mstreal> > S(3, vector<mstreal>(3, 0.0));
  mstreal ax, ay, az, bx, by, bz;
  mstreal GA = 0, GB = 0;
  for (i = 0; i < N; i++) {
    ax = ref(A[i])[0] - cA[0];
    ay = ref(A[i])[1] - cA[1];
    az = ref(A[i])[2] - cA[2];
    bx = ref(B[i])[0] - cB[0];
    by = ref(B[i])[1] - cB[1];
    bz = ref(B[i])[2] - cB[2];
    GA += ax*ax + ay*ay + az*az;
    GB += bx*bx + by*by + bz*bz;
    S[0][0] += bx * ax;
    S[0][1] += bx * ay;
    S[0][2] += bx * az;
    S[1][0] += by * ax;
    S[1][1] += by * ay;
    S[1][2] += by * az;
    S[2][0] += bz * ax;
    S[2][1] += bz * ay;
    S[2][2] += bz * az;
  }

  // square of S
  vector<vector<mstreal> > S2(3, vector<mstreal>(3, 0.0));
  for (i = 0; i < 3; i++) {
    for (j = 0; j < 3; j++) S2[i][j] = S[i][j]*S[i][j];
  }

  // calculate characteristic polynomial coefficients
  // NOTE: F, G, H, I could be made more efficient!
  mstreal C2 = -2*(S2[0][0] + S2[0][1] + S2[0][2] + S2[1][0] + S2[1][1] + S2[1][2] + S2[2][0] + S2[2][1] + S2[2][2]);
  mstreal C1 = 8*(S[0][0]*S[1][2]*S[2][1] + S[1][1]*S[2][0]*S[0][2] + S[2][2]*S[0][1]*S[1][0] -
                  S[0][0]*S[1][1]*S[2][2] - S[1][2]*S[2][0]*S[0][1] - S[2][1]*S[1][0]*S[0][2]);
  mstreal D = (S2[0][1] + S2[0][2] - S2[1][0] - S2[2][0]); D = D*D;
  mstreal E1 = -S2[0][0] + S2[1][1] + S2[2][2] + S2[1][2] + S2[2][1];
  mstreal E2 = 2*(S[1][1]*S[2][2] - S[1][2]*S[2][1]);
  mstreal E = (E1 - E2) * (E1 + E2);
  mstreal F = (-(S[0][2] + S[2][0])*(S[1][2] - S[2][1]) + (S[0][1] - S[1][0])*(S[0][0] - S[1][1] - S[2][2])) *
              (-(S[0][2] - S[2][0])*(S[1][2] + S[2][1]) + (S[0][1] - S[1][0])*(S[0][0] - S[1][1] + S[2][2]));
  mstreal G = (-(S[0][2] + S[2][0])*(S[1][2] + S[2][1]) - (S[0][1] + S[1][0])*(S[0][0] + S[1][1] - S[2][2])) *
              (-(S[0][2] - S[2][0])*(S[1][2] - S[2][1]) - (S[0][1] + S[1][0])*(S[0][0] + S[1][1] + S[2][2]));
  mstreal H = ( (S[0][1] + S[1][0])*(S[1][2] + S[2][1]) + (S[0][2] + S[2][0])*(S[0][0] - S[1][1] + S[2][2])) *
              (-(S[0][1] - S[1][0])*(S[1][2] - S[2][1]) + (S[0][2] + S[2][0])*(S[0][0] + S[1][1] + S[2][2]));
  mstreal I = ( (S[0][1] + S[1][0])*(S[1][2] - S[2][1]) + (S[0][2] - S[2][0])*(S[0][0] - S[1][1] - S[2][2])) *
              (-(S[0][1] - S[1][0])*(S[1][2] + S[2][1]) + (S[0][2] - S[2][0])*(S[0][0] + S[1][1] - S[2][2]));
  mstreal C0 = D + E + F + G + H + I;

  // now iterate Newton–Raphson method to find max eigenvalue
  mstreal tol = 10E-11;
  mstreal L = (GA + GB)/2; // this initial guess is key (see http://journals.iucr.org/a/issues/2005/04/00/sh5029/sh5029.pdf)
  mstreal Lold, L2, L3, L4, C22 = 2*C2;
  do {
    Lold = L;
    L2 = L*L;
    L3 = L2*L;
    L4 = L3*L;
    L = L - (L4 + C2*L2 + C1*L + C0)/(4*L3 + C22*L + C1);
  } while (fabs(L - Lold) > tol*L);

  return sqrt((GA + GB - 2*L)/N);
}


#endif
