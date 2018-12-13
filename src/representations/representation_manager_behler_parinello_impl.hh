/**
 * file   representation_manager_behler_parinello_impl.hh
 *
 * @author Till Junge <till.junge@epfl.ch>
 *
 * @date   13 Dec 2018
 *
 * @brief  implementation for Behler-Parinello representation manager
 *
 * Copyright © 2018 Till Junge, COSMO (EPFL), LAMMM (EPFL)
 *
 * librascal is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3, or (at
 * your option) any later version.
 *
 * librascal is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with librascal; see the file COPYING. If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

namespace rascal {

  template <class StructureManager>
  BehlerParinello<StructureManager>::BehlerParinello(StructureManager & structure,
                                                     const json & hypers)
    : structure{structure}, species{structure} {
  }

}  // rascal
