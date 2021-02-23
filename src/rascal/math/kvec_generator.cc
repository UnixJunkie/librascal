/**
 * @file   kvec_generator.cc
 *
 * @author  Andrea Grisafi <andrea.grisafi@epfl.ch>
 * @author  Kevin Kazuki Huguenin-Dumittan <kevin.huguenin-dumittan@epfl.ch>
 *
 * @date   10 February 2021
 *
 * @brief implementation of k-vectors generation
 *
 * Copyright  2021  Andrea Grisafi, Kevin Kazuki Huguenin-Dumittan, COSMO(EPFL)
 *
 * Rascal is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3, or (at
 * your option) any later version.
 *
 * Rascal is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this software; see the file LICENSE. If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "rascal/math/kvec_generator.hh"

#include <iostream>

using namespace rascal::math;  // NOLINT

void Kvectors::precompute(int n1max, int n2max, int n3max, 
		          Matrix_t basisvecs,
			  double kcut) {
 
  /*  INPUTS
    *   - basisvecs[3][3]: matrix containing the three basis vectors b1, b2, b3 of a 3D lattice, stored in the format b1 = basisvecs[0], b2=basisvecs[1] etc.
    *     a general vector in the lattice will then be of the form k = n1*b1 + n2*b2 + n3*b3, where n1-n3 are integers
    *   - kcut: length of the cutoff
    *
    *   OUTPUTS
    *   - Half the lattice points k excluding the origin lying in a sphere of radius kcut,
    *     where pairs related by k1 = -k2 are only included once.
    *     Mathematically, these are the points satisfying norm(k) <= kcut.
    *     More specifically, all points within the sphere are selected which are in the hemisphere
    *     lying in the b1-direction from the plane spanned by b2 and b3 that cuts the sphere in half.
  */

  // Auxiliary variables
  double kcutsq = kcut*kcut; // Computing the squared norm is faster than the norm
  double kx, ky, kz; // Components of the current grid point
  double normsq; // Store the current squared norm
  int nn2;

  /* EXECUTION:
    Begin loops to find the points within the search box contained in the sphere
    In order to avoid double counting pairs of points related by k2=-k1,
    the loops are chosen carefully, and separated into parts dealing with
    the cases
    - n1>0
    - n1=0, n2>0
    - n1=0, n2=0, n3>0
    in reverse order (order of increasing complexity of code)
  */

  // Step 1: Find all points of the form (0, 0, n3>0)
  // Initialize current grid point 
  kx = 0;
  ky = 0;
  kz = 0;
  for (int n3 = 1; n3 <= n3max; ++n3)
  {
      // Update the current grid point
      kx += basisvecs(2,0);
      ky += basisvecs(2,1);
      kz += basisvecs(2,2);
      normsq = kx*kx + ky*ky + kz*kz;
      if (normsq <= kcutsq) // Point inside sphere
      {
          kval(nk) = sqrt(normsq);
          kvec(nk, 0) = kx;
          kvec(nk, 1) = ky;
          kvec(nk, 2) = kz;
          nk += 1;
      }
  }

  // Step 2: Find all points of the form (0, n2>0, n3)
  for (int n2 = 1; n2 <= n2max; ++n2)
  {
      // Update current vector for new n2 value
      // We subtract (n3max+1)*b3 s.t. we only have to add b3 at each iteration
      // to get the correct vector
      kx = n2 * basisvecs(1,0) - n3max * basisvecs(2,0);
      ky = n2 * basisvecs(1,1) - n3max * basisvecs(2,1);
      kz = n2 * basisvecs(1,2) - n3max * basisvecs(2,2);

      for (int n3 = 0; n3 <= 2*n3max; ++n3)
      {
          // Update the current grid point
          kx += basisvecs(2,0);
          ky += basisvecs(2,1);
          kz += basisvecs(2,2);
          double normsq = kx*kx + ky*ky + kz*kz;
          if (normsq <= kcutsq) // Point inside sphere
          {
              kval(nk) = sqrt(normsq);
              kvec(nk, 0) = kx;
              kvec(nk, 1) = ky;
              kvec(nk, 2) = kz;
              nk += 1;
          }
      }
  }

  // Step 3: Find all remaining points of the form (n1>0, n2, n3)
  for (int n1 = 1; n1 <= n1max; ++n1)
  {
      for (int n2 = 0; n2 <= 2*n2max; ++n2)
      {
          nn2 = n2 - n2max;
          // Update current vector for new n2 value
          // We subtract (n3max+1)*b3 s.t. we only have to add b3 at each iteration
          // to get the correct vector
          kx = n1 * basisvecs(0,0) + nn2 * basisvecs(1,0) - n3max * basisvecs(2,0);
          ky = n1 * basisvecs(0,1) + nn2 * basisvecs(1,1) - n3max * basisvecs(2,1);
          kz = n1 * basisvecs(0,2) + nn2 * basisvecs(1,2) - n3max * basisvecs(2,2);

          for (int n3 = 0; n3 <= 2*n3max; ++n3)
          {
              // Update the current grid point
              kx += basisvecs(2,0);
              ky += basisvecs(2,1);
              kz += basisvecs(2,2);
              double normsq = kx*kx + ky*ky + kz*kz;
              if (normsq <= kcutsq) // Point inside sphere
              {
                  kval(nk) = sqrt(normsq);
                  kvec(nk, 0) = kx;
                  kvec(nk, 1) = ky;
      	          kvec(nk, 2) = kz;
                  nk += 1;
              }
          }
      }
  }

}
