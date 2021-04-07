/**
 * @file   rascal/structure_managers/adaptor_kspace
 *
 * @author Felix Musil <felix.musil@epfl.ch>
 *
 * @date   08 Apr 2021
 *
 * @brief implements an adaptor
 *
 * Copyright  2021 Felix Musil COSMO (EPFL), LAMMM (EPFL)
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

#ifndef SRC_RASCAL_STRUCTURE_MANAGERS_ADAPTOR_KSPACE_HH_
#define SRC_RASCAL_STRUCTURE_MANAGERS_ADAPTOR_KSPACE_HH_

#include "rascal/structure_managers/atomic_structure.hh"
#include "rascal/structure_managers/lattice.hh"
#include "rascal/structure_managers/property.hh"
#include "rascal/structure_managers/structure_manager.hh"
#include "rascal/utils/basic_types.hh"
#include "rascal/utils/utils.hh"

#include <set>
#include <vector>

namespace rascal {
  /**
   * Forward declaration for traits
   */
  template <class ManagerImplementation>
  class AdaptorKspace;

  /**
   * Specialisation of traits for increase <code>MaxOrder</code> adaptor
   */
  template <class ManagerImplementation>
  struct StructureManager_traits<AdaptorKspace<ManagerImplementation>> {
    using parent_traits = StructureManager_traits<ManagerImplementation>;
    constexpr static AdaptorTraits::Strict Strict{AdaptorTraits::Strict::no};
    constexpr static bool HasDistances{false};
    constexpr static bool HasDirectionVectors{false};
    constexpr static int Dim{parent_traits::Dim};
    constexpr static bool HasCenterPair{parent_traits::HasCenterPair};
    constexpr static int StackLevel{parent_traits::StackLevel + 1};
    // New MaxOrder upon construction, by construction should be 2
    constexpr static size_t MaxOrder{parent_traits::MaxOrder + 1};
    // When using periodic boundary conditions, it is possible that atoms are
    // added upon construction of the neighbour list. Therefore the layering
    // sequence is reset: here is layer 0 again.
    using LayerByOrder = std::index_sequence<0, 0>;
    using PreviousManager_t = ManagerImplementation;
    constexpr static AdaptorTraits::NeighbourListType NeighbourListType{
        AdaptorTraits::NeighbourListType::full};
  };

  /* ---------------------------------------------------------------------- */
  /**
   * Adaptor that increases the MaxOrder of an existing StructureManager. This
   * means, if the manager does not have a neighbourlist, it is created, if it
   * exists, triplets, quadruplets, etc. lists are created.
   */
  template <class ManagerImplementation>
  class AdaptorKspace
      : public StructureManager<AdaptorKspace<ManagerImplementation>>,
        public std::enable_shared_from_this<
            AdaptorKspace<ManagerImplementation>> {
   public:
    using Manager_t = AdaptorKspace<ManagerImplementation>;
    using Parent = StructureManager<Manager_t>;
    using ManagerImplementation_t = ManagerImplementation;
    using ImplementationPtr_t = std::shared_ptr<ManagerImplementation>;
    using ConstImplementationPtr_t =
        const std::shared_ptr<const ManagerImplementation>;
    using traits = StructureManager_traits<AdaptorKspace>;
    using PreviousManager_t = typename traits::PreviousManager_t;
    using AtomRef_t = typename ManagerImplementation::AtomRef_t;
    using Vector_ref = typename Parent::Vector_ref;
    using Vector_t = typename Parent::Vector_t;
    using Positions_ref =
        Eigen::Map<Eigen::Matrix<double, traits::Dim, Eigen::Dynamic>>;
    using Hypers_t = typename Parent::Hypers_t;

    static_assert(traits::MaxOrder == 2,
                  "ManagerImplementation needs an atom list "
                  " and can only build a neighbour list (pairs).");

    //! Default constructor
    AdaptorKspace() = delete;

    /**
     * Constructs a full neighbourhood list from a given manager and cut-off
     * radius or extends an existing neighbourlist to the next order
     */
    AdaptorKspace(ImplementationPtr_t manager);

    AdaptorKspace(ImplementationPtr_t manager,
                         const Hypers_t & adaptor_hypers)
        : AdaptorKspace(manager) {}

    //! Copy constructor
    AdaptorKspace(const AdaptorKspace & other) = delete;

    //! Move constructor
    AdaptorKspace(AdaptorKspace && other) = default;

    //! Destructor
    virtual ~AdaptorKspace() = default;

    //! Copy assignment operator
    AdaptorKspace &
    operator=(const AdaptorKspace & other) = delete;

    //! Move assignment operator
    AdaptorKspace & operator=(AdaptorKspace && other) = default;

    /**
     * Updates just the adaptor assuming the underlying manager was
     * updated. this function invokes building either the neighbour list or to
     * make triplets, quadruplets, etc. depending on the MaxOrder
     */
    void update_self();

    //! Updates the underlying manager as well as the adaptor
    template <class... Args>
    void update(Args &&... arguments);


    /**
     * Returns the linear indices of the clusters (whose atom tags are stored
     * in counters). For example when counters is just the list of atoms, it
     * returns the index of each atom. If counters is a list of pairs of indices
     * (i.e. specifying pairs), for each pair of indices i,j it returns the
     * number entries in the list of pairs before i,j appears.
     */
    template <size_t Order>
    size_t get_offset_impl(const std::array<size_t, Order> & counters) const;

    //! Returns the number of clusters of size cluster_size
    size_t get_nb_clusters(size_t order) const {
      switch (order) {
      /**
       * Note: The case for order=1 is abmiguous: one possible answer is the
       * number of centers the other possibility is the number of centers +
       * ghost atoms. Please use the get_size or get_size_with_ghosts member
       * functions
       */
      case 2: {
        return this->neighbours_atom_tag.size();
        break;
      }
      default:
        throw std::runtime_error("Can only handle pairs.");
        break;
      }
    }

    //! Returns number of clusters of the original manager
    size_t get_size() const { return this->n_centers; }

    size_t get_size_with_ghosts() const { return this->get_size(); }

    //! Returns position of an atom with index atom_tag
    Vector_ref get_position(size_t atom_tag) {
      return Vector_ref(&this->positions[atom_tag*traits::Dim]);
    }

    //! Returns position of the given atom object (useful for users)
    Vector_ref get_position(const AtomRef_t & atom) {
      return this->manager->get_position(atom.get_index());
    }

    /**
     * Returns the id of the index-th (neighbour) atom of the cluster that is
     * the full structure/atoms object, i.e. simply the id of the index-th atom
     *
     * This is called when ClusterRefKey<1, Layer> so we refer to a center
     * atoms. this function does the same job as get_atom_tag would do.
     */
    int get_neighbour_atom_tag(const Parent &, size_t iteration_index) const {
      return this->atom_tag_list[iteration_index];
    }

    //! Returns the id of the index-th neighbour atom of a given cluster
    template <size_t Order, size_t Layer>
    int get_neighbour_atom_tag(const ClusterRefKey<Order, Layer> & cluster,
                               size_t iteration_index) const {
      static_assert(Order < traits::MaxOrder,
                    "this implementation only handles up to traits::MaxOrder");

      // necessary helper construct for static branching
      using IncreaseHelper_t =
          internal::IncreaseHelper<Order == (traits::MaxOrder - 1)>;

      if (Order < (traits::MaxOrder - 1)) {
        return IncreaseHelper_t::get_neighbour_atom_tag(this->manager, cluster,
                                                        iteration_index);
      } else {
        auto && offset = this->offsets[cluster.get_cluster_index(Layer)];
        return this->neighbours_atom_tag[offset + iteration_index];
      }
    }

    //! Returns atom type given an atom tag, also works for ghost atoms
    int get_atom_type(int atom_tag) const { return this->atom_types[atom_tag]; }

    /** The atom tag corresponds to an ghost atom, then it returns it cluster
     * index of the atom in the original cell.
     */
    size_t get_atom_index(const int atom_tag) const {
      return this->atom_index_from_atom_tag_list[atom_tag];
    }


    //! Returns the number of neighbours of a given atom at a given TargetOrder
    //! Returns the number of pairs of a given center
    template <size_t TargetOrder, size_t Order, size_t Layer>
    typename std::enable_if_t<TargetOrder == 2, size_t>
    get_cluster_size_impl(const ClusterRefKey<Order, Layer> & cluster) const {
      constexpr auto nb_neigh_layer{
          get_layer<TargetOrder>(typename traits::LayerByOrder{})};
      auto && access_index = cluster.get_cluster_index(nb_neigh_layer);
      return nb_neigh[access_index];
    }

    //! Get the manager used to build the instance
    ImplementationPtr_t get_previous_manager_impl() {
      return this->manager->get_shared_ptr();
    }

    //! Get the manager used to build the instance
    ConstImplementationPtr_t get_previous_manager_impl() const {
      return this->manager->get_shared_ptr();
    }

    size_t get_n_update() const { return this->n_update; }

   protected:
    /* ---------------------------------------------------------------------- */

    //! Extends the list containing the number of neighbours with a 0
    void add_entry_number_of_neighbours() { this->nb_neigh.push_back(0); }

    //! Sets the correct offsets for accessing neighbours
    void set_offsets() {
      auto && n_tuples{nb_neigh.size()};
      if (n_tuples > 0) {
        this->offsets.reserve(n_tuples);
        this->offsets.resize(1);
        for (size_t i{0}; i < n_tuples - 1; ++i) {
          this->offsets.emplace_back(this->offsets[i] + this->nb_neigh[i]);
        }
      }
    }

    /* ---------------------------------------------------------------------- */
    //! full neighbour list with linked cell algorithm
    void make_full_neighbour_list();

    /* ---------------------------------------------------------------------- */
    //! pointer to underlying structure manager
    ImplementationPtr_t manager;

    //! stores i-atom tags
    std::vector<int> atom_tag_list{};

    std::vector<int> atom_types{};

    // //! Stores additional atom tags of current Order (only ghost atoms)
    // std::vector<int> ghost_atom_tag_list{};

    //! Stores the number of neighbours for every atom
    std::vector<size_t> nb_neigh{};

    //! Stores neighbour's atom tag in a list in sequence of atoms
    std::vector<int> neighbours_atom_tag{};

    /**
     * Returns the atoms cluster index when accessing it with the atom's atomic
     * index in a list in sequence of atoms.  List of atom tags which have a
     * correpsonding cluster index of order 1.  If ghost atoms have been added
     * they have their own new index.
     */
    std::vector<size_t> atom_index_from_atom_tag_list{};

    //! Stores the offset for each atom to accessing `neighbours`, this variable
    //! provides the entry point in the neighbour list, `nb_neigh` the number
    //! from the entry point
    std::vector<size_t> offsets{};

    size_t cluster_counter{0};

    //! number of i atoms, i.e. centers from underlying manager
    size_t n_centers;

    //! counts the number of time the neighbour list has been updated
    size_t n_update{0};

    /**
     * on top of the main update signal, the skin parameter allow to skip
     * the update. So this variable records this possiblity.
     */
    bool need_update{true};

    //! atom positions
    std::vector<double> positions{};

    // //! atom type
    // std::vector<int> atom_types{};

   private:
  };

  /* ---------------------------------------------------------------------- */
  //! Constructor of the pair list manager
  template <class ManagerImplementation>
  AdaptorKspace<ManagerImplementation>::AdaptorKspace(
      std::shared_ptr<ManagerImplementation> manager)
      : manager{std::move(manager)},
        atom_tag_list{}, atom_types{}, nb_neigh{},
        neighbours_atom_tag{}, offsets{}, n_centers{0} {
    static_assert(not(traits::MaxOrder < 1), "No atom list in manager");
  }

  /* ---------------------------------------------------------------------- */
  /**
   * Update function that recursively pass its argument to the base
   * (Centers or Lammps). The base will then update the whole tree from the top.
   */
  template <class ManagerImplementation>
  template <class... Args>
  void
  AdaptorKspace<ManagerImplementation>::update(Args &&... arguments) {
    if (sizeof...(arguments) > 0) {
      // TODO(felix) should not have to assume that the underlying manager is
      // manager centers.
      auto && atomic_structure{this->manager->get_atomic_structure()};
      // if the structure has not changed by more than skin**2
      this->need_update = true;
    }
    this->manager->update(std::forward<Args>(arguments)...);
  }
  /* ---------------------------------------------------------------------- */
  /**
   * build a neighbour list based on atomic positions, types and indices, in the
   * following the needed data structures are initialized, after construction,
   * this function must be called to invoke the neighbour list algorithm
   */
  template <class ManagerImplementation>
  void AdaptorKspace<ManagerImplementation>::update_self() {
    if (this->need_update) {
      // set the number of centers
      this->n_centers = this->manager->get_size();

      //! Reset cluster_indices for adaptor to fill with sequence
      internal::for_each(this->cluster_indices_container,
                         internal::ResizePropertyToZero());

      // initialize necessary data structure
      this->atom_tag_list.clear();
      this->atom_types.clear();
      this->nb_neigh.clear();
      this->neighbours_atom_tag.clear();
      this->offsets.clear();
      this->positions.clear();
      this->atom_index_from_atom_tag_list.clear();
      // actual call for building the neighbour list
      this->make_full_neighbour_list();
      this->set_offsets();

      // layering is started from the scratch, therefore all clusters and
      // centers+ghost atoms are in the right order.
      auto & atom_cluster_indices{std::get<0>(this->cluster_indices_container)};
      auto & pair_cluster_indices{std::get<1>(this->cluster_indices_container)};

      atom_cluster_indices.fill_sequence();
      pair_cluster_indices.fill_sequence();
      ++this->n_update;
    }
  }

  /* ---------------------------------------------------------------------- */
  /**
   * Build a neighbor list using a linked cell algorithm for finite cutoff
   * interaction computations of lenght \f$r_c\f$. There is no restriction
   * regarding the type of lattice and periodic boundary conditions (triclinic
   * lattices and mixed periodicity are handled by design).
   *
   * To do so we build a cubic box that contains the unit cell and one
   * \f$r_c\f$ in each directions. The length of the box is a multiple of
   * \f$r_c\f$ and it is partitioned into cubic bins of size \f$r_c\f$. The
   * resulting mesh encompass the unit cell and its surrounding up to \f$2
   * r_c\f$ in each directions so that atoms can be binned in it. The
   * additional layer of bins is kept empty so that the connectivity assignment
   * criteria of the linked cell, i.e. atoms belonging to neighbor bins are
   * neighbors, can be applied uniformly (latter referred are stencil).
   * note(felix): the mesh going up to \f$2 r_c\f$ is probably not really
   *  necessary for the connectivity assignment and \f$r_c\f$ would work too.
   *
   * Atoms are assumed to be inside the unit cell (otherwise it will throw an
   * error) and they are binned. Then depending on the periodicity of the
   * system, the periodic images or ghost atoms that fall within the bounds of
   * the mesh are also binned.
   * Then each binned atoms is assigned its neighbor depending on the bin's
   * connectivity criteria (in 3d the 27 nearest bins).
   */
  template <class ManagerImplementation>
  void AdaptorKspace<ManagerImplementation>::make_full_neighbour_list() {
    using Vector_t = Eigen::Matrix<double, traits::Dim, 1>;

    // short hands for parameters and inputs
    constexpr auto dim{traits::Dim};

    auto periodicity = this->manager->get_periodic_boundary_conditions();
    size_t ntot{1};

    // calculate number of actual repetitions of cell, depending on periodicity
    for (auto i{0}; i < dim; ++i) {
      if (not periodicity[i]) {
        throw std::runtime_error(
                R"(The structure should be fully periodic.)");
      }
    }
    size_t n_atoms{this->get_manager().size()};

    for (auto center : this->get_manager()) {
      int atom_tag = center.get_atom_tag();
      int atom_type = center.get_atom_type();
      auto atom_index = this->manager->get_atom_index(atom_tag);
      int nneigh{0};

      Vector_t pos = center.get_position();

      this->atom_tag_list.push_back(atom_tag);
      this->atom_types.push_back(atom_type);
      this->atom_index_from_atom_tag_list.push_back(atom_index);
      for (auto i_dim{0}; i_dim < traits::Dim; ++i_dim) {
        this->positions.push_back(pos(i_dim));
      }

      nneigh += n_atoms;
      for (auto center_j : this->get_manager()) {
        int atom_tag_j = center_j.get_atom_tag();
        int atom_type_j = center_j.get_atom_type();
        auto atom_index_j = this->manager->get_atom_index(atom_tag_j);
        Vector_t pos_j = center_j.get_position();

        this->atom_tag_list.push_back(atom_tag_j);
        this->atom_types.push_back(atom_type_j);
        this->atom_index_from_atom_tag_list.push_back(atom_index_j);
        this->neighbours_atom_tag.push_back(atom_tag_j);
        for (auto i_dim{0}; i_dim < traits::Dim; ++i_dim) {
          this->positions.push_back(pos_j(i_dim));
        }
      }

      this->nb_neigh.push_back(nneigh);
    }


    // // Before generating periodic replicas atoms (also termed ghost atoms), all
    // // existing center atoms are added to the list of current atoms to start the
    // // full list of current i-atoms to have them all contiguously at the
    // // beginning of the list.
    // for (size_t atom_tag{0}; atom_tag < this->manager->get_size(); ++atom_tag) {
    //   auto atom_type = this->manager->get_atom_type(atom_tag);
    //   auto atom_index = this->manager->get_atom_index(atom_tag);
    //   this->atom_tag_list.push_back(atom_tag);
    //   this->atom_types.push_back(atom_type);
    //   this->atom_index_from_atom_tag_list.push_back(atom_index);
    // }

    // for (size_t atom_tag_i{0}; atom_tag_i < this->manager->get_size(); ++atom_tag_i) {
    //   for (size_t atom_tag_j{0}; atom_tag_j < this->manager->get_size(); ++atom_tag_j) {
    //     int nneigh{0};

    //   Vector_t pos = center.get_position();
    //   Vector_t dpos = pos - mesh_min;
    //   auto box_index = internal::get_box_index(dpos, cutoff);
    //   internal::fill_neighbours_atom_tag(atom_tag, box_index, atom_id_cell,
    //                                      current_j_atoms);

    //   nneigh += current_j_atoms.size();
    //   for (auto & j_atom_tag : current_j_atoms) {
    //     this->neighbours_atom_tag.push_back(j_atom_tag);
    //   }
    //   }
    // }


    // // this->atom_tag_list.push_back(atom_tag);
    // //   this->atom_types.push_back(atom_type);
    // //   // add it to the ghost atom container
    // //   this->ghost_atom_tag_list.push_back(atom_tag);
    // //   this->ghost_types.push_back(atom_type);
    // //   for (auto dim{0}; dim < traits::Dim; ++dim) {
    // //     this->ghost_positions.push_back(position(dim));
    // //   }

    // // generate ghost atom tags and positions
    // for (size_t atom_tag{0}; atom_tag < this->manager->get_size_with_ghosts();
    //      ++atom_tag) {
    //   auto pos = this->manager->get_position(atom_tag);
    //   auto atom_type = this->manager->get_atom_type(atom_tag);

    //   for (auto && p_image :
    //        internal::PeriodicImages<dim>{periodic_min, repetitions, ntot}) {
    //     // exclude the original unit cell
    //     //! assumption: this assumes atoms were inside the cell initially
    //     if (not(p_image.array() == 0).all()) {
    //       Vector_t pos_ghost{pos + cell * p_image.template cast<double>()};
    //       auto flag_inside = internal::position_in_bounds(ghost_min, ghost_max,
    //                                                       pos_ghost, bound_tol);

    //       if (flag_inside) {
    //         // next atom tag is size, since start is at index = 0
    //         auto new_atom_tag{this->n_centers + this->n_ghosts};
    //         this->add_ghost_atom(new_atom_tag, pos_ghost, atom_type);
    //         // adds origin atom cluster_index if true
    //         // adds ghost atom cluster index if false
    //         size_t atom_index = this->manager->get_atom_index(atom_tag);
    //         this->atom_index_from_atom_tag_list.push_back(atom_index);
    //       }
    //     }
    //   }
    // }


    // // go through all atoms and/or ghosts to build neighbour list, depending on
    // // the runtime decision flag
    // std::vector<int> current_j_atoms{};
    // for (auto center : this->get_manager()) {
    //   int atom_tag = center.get_atom_tag();
    //   int nneigh{0};

    //   Vector_t pos = center.get_position();
    //   Vector_t dpos = pos - mesh_min;
    //   auto box_index = internal::get_box_index(dpos, cutoff);
    //   internal::fill_neighbours_atom_tag(atom_tag, box_index, atom_id_cell,
    //                                      current_j_atoms);

    //   nneigh += current_j_atoms.size();
    //   for (auto & j_atom_tag : current_j_atoms) {
    //     this->neighbours_atom_tag.push_back(j_atom_tag);
    //   }

    //   this->nb_neigh.push_back(nneigh);
    // }

    // /**
    //  * All the ghost atom neighbours have to be added explicitly as zero. This
    //  * is done after adding the neighbours of centers because ghost atoms are
    //  * listed after the center atoms in the respective data
    //  * structures. Technically ghost atoms can not have any neighbour, i.e. not
    //  * even '0'. It should be _nothing_. But that is not possible with our data
    //  * structure.
    //  */
    // int nneigh{0};
    // for (auto && dummy : this->get_manager().only_ghosts()) {
    //   std::ignore = dummy;
    //   this->nb_neigh.push_back(nneigh);
    // }
  }

  /* ---------------------------------------------------------------------- */
  /**
   * Returns the linear indices of the clusters (whose atom tags
   * are stored in counters). For example when counters is just the list
   * of atoms, it returns the index of each atom. If counters is a list of pairs
   * of indices (i.e. specifying pairs), for each pair of indices i,j it returns
   * the number entries in the list of pairs before i,j appears.
   */
  template <class ManagerImplementation>
  template <size_t Order>
  size_t AdaptorKspace<ManagerImplementation>::get_offset_impl(
      const std::array<size_t, Order> & counters) const {
    // The static assert with <= is necessary, because the template parameter
    // ``Order`` is one Order higher than the MaxOrder at the current
    // level. The return type of this function is used to build the next Order
    // iteration.
    static_assert(Order <= traits::MaxOrder,
                  "this implementation handles only up to the respective"
                  " MaxOrder");
    return this->offsets[counters.front()];
  }

}  // namespace rascal

#endif  // SRC_RASCAL_STRUCTURE_MANAGERS_ADAPTOR_KSPACE_HH_
