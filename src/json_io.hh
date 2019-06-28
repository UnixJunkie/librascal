/**
 * file   json_io.hh
 *
 * @author Markus Stricker <markus.stricker@epfl.ch>
 *
 * @date   18 Jun 2018
 *
 * @brief JSON interface from nlohmanns header class
 *
 * Copyright  2018 Markus Stricker, COSMO (EPFL), LAMMM (EPFL)
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

#ifndef SRC_JSON_IO_HH_
#define SRC_JSON_IO_HH_

/*
 * interface to external header-library/header-class, which makes it easy to use
 * the JSON as a first class data type. See https://github.com/nlohmann/json for
 * documentation.
 */
#include "json.hpp"

#include <Eigen/Dense>
#include <fstream>

// For convenience
using json = nlohmann::json;

/**
 * Utilities to convert json to Eigen types and the oposite
 */
namespace Eigen {
  /* ---------------------------------------------------------------------- */
  template<typename _Scalar, int _Rows, int _Cols, int _Options>
  void to_json(json & j, const Matrix<_Scalar, _Rows, _Cols, _Options>& s) {
    for (int irow{0}; irow < s.rows(); ++irow) {
      j.push_back(json::array());
      for (int icol{0}; icol < s.cols(); ++icol) {
        j[irow][icol] = s(irow, icol);
      }
    }
  }

  /* ---------------------------------------------------------------------- */
  template<typename _Scalar, int _Rows, int _Cols, int _Options>
  void from_json(const json & j, Matrix<_Scalar, _Rows, _Cols, _Options>& s) {
    if (j.is_array() != true) {
      std::string error{"the input json is not an array "};
      std::string error2{j.dump(2)};
      throw std::runtime_error(error + error2);
    }
    int n_rows{j.size()};
    int n_cols{j[0].size()};
    for (int irow{0}; irow < n_rows; ++irow) {
      if (n_cols != j[irow].size()) {
        std::string error{"the input json does not have cols of same sizes"};
        throw std::runtime_error(error);
      }
    }
    s.resize(n_rows, n_cols);
    for (int irow{0}; irow < n_rows; ++irow) {
      for (int icol{0}; icol < n_cols; ++icol) {
        s(irow, icol) = j[irow][icol];
      }
    }
  }

   /* ---------------------------------------------------------------------- */
  template<typename _Scalar, int _Rows, int _Cols, int _Options>
  void to_json(json & j, const Array<_Scalar, _Rows, _Cols, _Options>& s) {
    for (int irow{0}; irow < s.rows(); ++irow) {
      j.push_back(json::array());
      for (int icol{0}; icol < s.cols(); ++icol) {
        j[irow][icol] = s(irow, icol);
      }
    }
  }

  /* ---------------------------------------------------------------------- */
  template<typename _Scalar, int _Rows, int _Cols, int _Options>
  void from_json(const json & j, Array<_Scalar, _Rows, _Cols, _Options>& s) {
    if (j.is_array() != true) {
      std::string error{"the input json is not an array "};
      std::string error2{j.dump(2)};
      throw std::runtime_error(error + error2);
    }
    int n_rows{j.size()};
    int n_cols{j[0].size()};
    for (int irow{0}; irow < n_rows; ++irow) {
      if (n_cols != j[irow].size()) {
        std::string error{"the input json does not have cols of same sizes"};
        throw std::runtime_error(error);
      }
    }
    s.resize(n_rows, n_cols);
    for (int irow{0}; irow < n_rows; ++irow) {
      for (int icol{0}; icol < n_cols; ++icol) {
        s(irow, icol) = j[irow][icol];
      }
    }
  }

}

/*
 * All functions and classes are in the namespace <code>rascal</code>, which
 * ensures that they don't clash with other libraries one might use in
 * conjunction.
 */
namespace rascal {
  namespace json_io {
    /**
     * Object to deserialize the content of a JSON file containing Atomic
     * Simulation Environment (ASE) type atomic structures, the nlohmann::json
     * needs a <code>struct</code> with standard data types for deserialization
     */
    struct AtomicJsonData {
      /**
       *  @param cell is a vector a vector of vectors which holds the cell unit
       *  vectors.
       *
       *  @param type a vector of integers which holds the atomic type (atomic
       *  number from periodic table).
       *
       *  @param pbc is a 0/1 vector which says, where periodic boundary
       *  conditions are applied.
       *
       *  @param position is a vector of vectors which holds the atomic
       *  positions.
       */
      std::vector<std::vector<double>> cell{};
      std::vector<int> type{};
      std::vector<int> pbc{};
      std::vector<std::vector<double>> position{};
    };

    /**
     * Function to convert to a JSON object format with the given keywords. It
     * is an overload of the function defined in the header class
     * json.hpp.
     */
    void to_json(json & j, AtomicJsonData & s);

    /**
     * Function used to read from the JSON file, given the keywords and convert
     * the data into standard types. Overload of the function defined in
     * json.hpp class header.
     */
    void from_json(const json & j, AtomicJsonData & s);

    /**
     * checks a value-unit pair of form {"value": 5.6, "unit": "Å"} against the
     * expected unit, returns value if successful and throws an error if not
     */
    double check_units(const std::string & expected_unit,
                       const json & parameter);
  }  // namespace json_io
}  // namespace rascal

#endif  // SRC_JSON_IO_HH_
