/**
 * @file   rascal/structure_managers/structure_manager_lammps.cc
 *
 * @author Till Junge <till.junge@epfl.ch>
 *
 * @date   05 Apr 2018
 *
 * @brief Implementation of the neighbourhood manager for lammps
 *        neighbourhood lists
 *
 * Copyright  2018 Till Junge, COSMO (EPFL), LAMMM (EPFL)
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

#include "rascal/structure_managers/structure_manager_lammps.hh"

#include <numeric>

namespace rascal {

  /* ---------------------------------------------------------------------- */
  void StructureManagerLammps::update_self(int inum, int tot_num, int * ilist,
                                           int * numneigh, int ** firstneigh,
                                           double ** x, double ** f, int * type,
                                           double * eatom, double ** vatom, std::vector<int> atom_types,
                                           int * atom_ghost_tag) {
    // setting the class variables
    this->inum = inum;
    this->tot_num = tot_num;
    this->ilist = ilist;
    this->numneigh = numneigh;
    this->firstneigh = firstneigh;
    this->x = x;
    this->f = f;
    this->type = type;
    this->eatom = eatom;
    this->vatom = vatom;
    //std::cout << "atom_types.size() " << atom_types.size() << std::endl;
    this->atom_types = atom_types;
    this->offsets.reserve(inum);
    this->offsets.resize(1);
    //std::cout << "atom_index_from_atom_tag_list() " << std::endl;
    this->atom_index_from_atom_tag_list.clear();
    //std::cout << "offsets " << std::endl;
    for (int i{0}; i < this->inum; ++i) {
      this->offsets.emplace_back(this->offsets[i] + this->numneigh[i]);
    }
    this->nb_pairs = std::accumulate(numneigh, numneigh + this->inum, 0);
    //std::cout << "nb pairs " << this->nb_pairs << std::endl;

    //std::cout << "cluster indices" << std::endl;
    auto & atom_cluster_indices{std::get<0>(this->cluster_indices_container)};
    auto & pair_cluster_indices{std::get<1>(this->cluster_indices_container)};


    // TODO(alex) here we assume that ilist does count ascending without gaps
    //            I think this is generally true, but it would be good to double check this
    this->atom_tag_list.resize(this->tot_num);
    this->atom_index_from_atom_tag_list.resize(this->tot_num);
    for (int i{0}; i < this->tot_num; ++i) {
      this->atom_tag_list[i] = i;
      this->atom_index_from_atom_tag_list[i] = atom_ghost_tag[i]-1; // atom tags in lammps start with 1
    }

    atom_cluster_indices.fill_sequence();
    pair_cluster_indices.fill_sequence();
    std::cout << "StructureManagerLammps offsets: ";
    for (unsigned int p=0; p < offsets.size(); p++) {
      std::cout << offsets[p] << ", ";
    }
    std::cout << std::endl;
  }

  /* ---------------------------------------------------------------------- */
  /*
   * Return the number of clusters of size cluster_size.  Can only handle
   * cluster_size 1 (atoms) and cluster_size 2 (pairs).
   */
  size_t StructureManagerLammps::get_nb_clusters(int order) const {
    switch (order) {
    case 1:
      return inum;
      /**
       * Note: The case for order=1 is abmiguous: one possible answer is the
       * number of centers the other possibility is the number of centers +
       * ghost atoms. Please use the get_size or get_size_with_ghosts member
       * functions
       */
    case 2:
      return nb_pairs;
    default:
      throw std::runtime_error("Can only handle single atoms and pairs");
    }
  }
}  // namespace rascal
