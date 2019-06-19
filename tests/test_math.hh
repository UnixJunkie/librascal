/**
 * file   test_math.hh
 *
 * @author  Felix Musil <felix.musil@epfl.ch>
 * @author  Max Veit <max.veit@epfl.ch>
 *
 * @date   14 October 2018
 *
 * @brief Test implementation of math functions
 *
 * Copyright  2018  Felix Musil, COSMO (EPFL), LAMMM (EPFL)
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

#ifndef TESTS_TEST_MATH_HH_
#define TESTS_TEST_MATH_HH_

#include "tests.hh"
#include "json_io.hh"
#include "math/math_interface.hh"
#include "math/math_utils.hh"
#include "math/spherical_harmonics.hh"
#include "math/hyp1f1.hh"
#include "rascal_utility.hh"

#include <fstream>
#include <string>
#include <Eigen/Dense>

namespace rascal {

  struct SphericalHarmonicsRefFixture {
    SphericalHarmonicsRefFixture() {
      json ref_data;
      std::ifstream ref_file(this->ref_filename);
      ref_file >> ref_data;
      unit_vectors = ref_data.at("unit_vectors").get<StdVector2Dim_t>();
      harmonics = ref_data.at("harmonics").get<StdVector3Dim_t>();
      alps = ref_data.at("alps").get<StdVector3Dim_t>();
    }

    ~SphericalHarmonicsRefFixture() = default;

    std::string ref_filename = "reference_data/spherical_harmonics_test.json";

    using StdVector2Dim_t = std::vector<std::vector<double>>;
    using StdVector3Dim_t = std::vector<std::vector<std::vector<double>>>;
    StdVector2Dim_t unit_vectors{};
    StdVector3Dim_t harmonics{};
    StdVector3Dim_t alps{};
    bool verbose{false};
  };

  /**
   * Fixture for testing a the gradient of a scalar function of N real arguments
   *
   * (Verifies that the gradient is the same as the converged value of the
   * finite-difference approximation along the given directions)
   *
   * Parameters should be provided in a JSON input file, as follows:
   *
   * @param function_inputs List of vectors of function arguments at which to
   *                        test the gradient
   *
   * @param direction_mode How the finite-difference directions are specified;
   *                       options are "Cartesian" (once along each independent
   *                       argument), "Random" (exactly what it says on the
   *                       tin), and "Provided" (given in input file, see below)
   *
   * @param displacement_directions List of vectors along which to displace the
   *                                inputs, in case "direction_mode" is
   *                                "Provided".
   */
  class GradientTestFixture {

   public:
    explicit GradientTestFixture(std::string input_filename) {
      using Eigen::ArrayXd;
      using Eigen::MatrixXd;
      using Eigen::VectorXd;
      json input_data;

      std::ifstream input_file(input_filename);
      input_file >> input_data;
      this->function_inputs = input_data.at("function_inputs")
                                        .get<StdVector2Dim_t>();
      this->n_arguments = function_inputs[0].size();

      this->displacement_directions =
              this->get_displacement_directions(input_data, this->n_arguments);
      this->verbosity = get_verbosity(input_data);
    }

    ~GradientTestFixture() = default;

    static Eigen::MatrixXd get_displacement_directions(json & input_data,
                                                       size_t n_arguments) {
      using Eigen::MatrixXd;
      using Eigen::VectorXd;
      MatrixXd directions;
      std::string direction_mode =
          input_data.at("direction_mode").get<std::string>();
      if (direction_mode.compare("Cartesian") == 0) {
        directions = MatrixXd::Identity(n_arguments, n_arguments);
      } else if (direction_mode.compare("Random") == 0) {
        size_t n_directions{input_data.at("n_directions").get<size_t>()};
        directions = MatrixXd::Random(n_directions, n_arguments);
        directions.rowwise().normalize();
      } else if (direction_mode.compare("Provided") == 0) {
        StdVector2Dim_t directions_in =
            input_data.at("displacement_directions").get<StdVector2Dim_t>();
        directions.resize(directions_in.size(), n_arguments);
        int row_idx{0};
        for (auto & direction : directions_in) {
          directions.row(row_idx++) =
              Eigen::Map<VectorXd>(direction.data(), 1, n_arguments);
        }
        directions.rowwise().normalize();
      } else {
        std::cerr << "Unknown direction mode \'" << direction_mode;
        std::cerr << "\', assuming Cartesian" << std::endl;
        directions = MatrixXd::Identity(n_arguments, n_arguments);
      }
      return directions;
    }

    enum struct VerbosityValue {
      NORMAL = 0, // Print nothing
      INFO = 10,  // Print one line of info for each gradient step
      DEBUG = 20  // Print as much as possible
    };

    static VerbosityValue get_verbosity(json & input_data) {
      VerbosityValue verbosity_in{VerbosityValue::NORMAL};
      if (input_data.find("verbosity") != input_data.end()) {
        std::string verbosity_str = input_data["verbosity"].get<std::string>();
        if (verbosity_str.compare("INFO") == 0) {
          verbosity_in = VerbosityValue::INFO;
        } else if (verbosity_str.compare("DEBUG") == 0) {
          verbosity_in = VerbosityValue::DEBUG;
        } else if (verbosity_str.compare("NORMAL") == 0) {
          verbosity_in = VerbosityValue::NORMAL;
        } else {
          std::cerr << "Unknown verbosity value \'" << verbosity_str;
          std::cerr << "\', assuming NORMAL" << std::endl;
          verbosity_in = VerbosityValue::NORMAL;
        }
      }
      return verbosity_in;
    }

    using StdVector2Dim_t = std::vector<std::vector<double>>;

    StdVector2Dim_t function_inputs{};
    Eigen::MatrixXd displacement_directions{};
    size_t n_arguments{0};
    VerbosityValue verbosity{VerbosityValue::NORMAL};

   protected:
    GradientTestFixture() {}
  };

  template <int max_angular>
  class SphericalHarmonicsWithGradients {
   public:
    SphericalHarmonicsWithGradients() = default;

    ~SphericalHarmonicsWithGradients() = default;

    Eigen::Array<double, 1, (max_angular + 1) * (max_angular + 1)>
    f(const Eigen::Vector3d & inputs_v) {
      // Renormalize the inputs to project out the r gradients
      Eigen::Vector3d my_inputs = inputs_v / inputs_v.norm();
      return math::compute_spherical_harmonics(my_inputs, max_angular);
    }

    Eigen::Array<double, 3, (max_angular + 1) * (max_angular + 1)>
    grad_f(const Eigen::Vector3d & inputs_v) {
      Eigen::Array<double, 4, (max_angular + 1) * (max_angular + 1)>
          harmonics_derivatives{math::compute_spherical_harmonics_derivatives(
              inputs_v, max_angular)};
      return harmonics_derivatives.bottomRows(3);
    }
  };

  /**
   * Numerically verify that a given function and its gradient are consistent
   *
   * @param function_calculator An object that provides both the function and
   *                            its gradient
   *
   * @param params Test fixture object, e.g a GradientTestFixture or something
   *               providing the same information (i.e. function_inputs,
   *               displacement_directions, n_arguments, and verbosity)
   *
   * The function_calculator object may be of any type, as long as it provides
   * two functions, f() and grad_f(), to calculate the function and its gradient
   * (derivative for functions with one input, Jacobian for functions with
   * multiple outputs -- the output dimension is expected to correspond to
   * columns).  Both functions must accept an Eigen::Vector, corresponding to
   * the function input, of dimension determined in the data file (read by
   * GradientTestFixture).  This function additionally guarantees that f() will
   * be called before grad_f() with the same input.
   */
  template<typename FunctionProvider_t, typename TestFixture_t>
  void test_gradients(FunctionProvider_t function_calculator,
                      TestFixture_t params) {
    Eigen::MatrixXd values;
    Eigen::MatrixXd jacobian;
    Eigen::RowVectorXd argument_vector;
    Eigen::VectorXd displacement_direction;
    Eigen::VectorXd displacement;
    Eigen::MatrixXd directional;
    Eigen::MatrixXd fd_derivatives;
    Eigen::MatrixXd fd_error_cwise;

    using VerbosityValue = typename GradientTestFixture::VerbosityValue;

    // This error isn't going to be arbitrarily small, due to the interaction of
    // finite-difference and finite precision effects.  Just set it to something
    // reasonable and check it explicitly if you really want to be sure (paying
    // attention to the change of the finite-difference gradients from one step
    // to the next).  The automated test is really more intended to be a sanity
    // check on the implementation anyway.
    constexpr double fd_error_tol = 1E-6;
    for (auto inputs : params.function_inputs) {
      argument_vector = Eigen::Map<Eigen::RowVectorXd>(inputs.data(), 1,
                                                       params.n_arguments);
      values = function_calculator.f(argument_vector);
      jacobian = function_calculator.grad_f(argument_vector);
      if (params.verbosity >= VerbosityValue::INFO) {
        std::cout << std::string(30, '-') << std::endl;
        std::cout << "Input vector: " << argument_vector << std::endl;
      }
      if (params.verbosity >= VerbosityValue::DEBUG) {
        std::cout << "Function values:" << values << std::endl;
        std::cout << "Jacobian:" << jacobian << std::endl;
      }
      for (int disp_idx{0}; disp_idx < params.displacement_directions.rows();
           disp_idx++) {
        displacement_direction = params.displacement_directions.row(disp_idx);
        // Compute the directional derivative(s)
        directional = displacement_direction.adjoint() * jacobian;
        if (params.verbosity >= VerbosityValue::INFO) {
          std::cout << "FD direction: " << displacement_direction.adjoint();
          std::cout << std::endl;
        }
        if (params.verbosity >= VerbosityValue::DEBUG) {
          std::cout << "Analytical derivative: " << directional << std::endl;
        }
        double min_error{HUGE_VAL};
        Eigen::MatrixXd fd_last{Eigen::MatrixXd::Zero(1, directional.size())};
        //for (double dx = 1E-2; dx > 1E-10; dx *= 0.1) {
        // Modified displacements so we can see the effects
        for (double dx = 1E-1; dx > 1E-3; dx *= 0.1) {
          if (params.verbosity >= VerbosityValue::INFO) {
            std::cout << "dx = " << dx << "\t";
          }
          displacement = dx * displacement_direction;
          // Compute the finite-difference derivative using a
          // centred-difference approach
          Eigen::MatrixXd fun_plus{
              function_calculator.f(argument_vector + displacement.adjoint())};
          Eigen::MatrixXd fun_minus{
              function_calculator.f(argument_vector - displacement.adjoint())};
          fd_derivatives = 0.5 / dx * (fun_plus - fun_minus);
          double fd_error{0.};
          double fd_quotient{0.};
          size_t nonzero_count{0};
          for (int dim_idx{0}; dim_idx < fd_derivatives.size(); dim_idx++) {
            if (std::abs(directional(dim_idx)) < 10 * math::dbl_ftol) {
              fd_error += fd_derivatives(dim_idx);
            } else {
              fd_quotient += (fd_derivatives(dim_idx) / directional(dim_idx));
              fd_error += (fd_derivatives(dim_idx) - directional(dim_idx)) /
                          directional(dim_idx);
              ++nonzero_count;
            }
          }
          if (nonzero_count > 0) {
            fd_quotient = fd_quotient / nonzero_count;
          }
          fd_error = fd_error / fd_derivatives.size();
          if (params.verbosity >= VerbosityValue::INFO) {
            std::cout << "Average rel FD error: " << fd_error << "\t";
            std::cout << "Average FD quotient:  " << fd_quotient << std::endl;
          }
          if (std::abs(fd_error) < min_error) {
            min_error = std::abs(fd_error);
          }
          if (params.verbosity >= VerbosityValue::DEBUG) {
            fd_error_cwise = (fd_derivatives - directional);
            std::cout << "error            = " << fd_error_cwise << std::endl;
            std::cout << "(FD derivative   = " << fd_derivatives << ")";
            std::cout << std::endl;
            std::cout << "(minus last step = " << fd_derivatives - fd_last;
            std::cout << ")" << std::endl;
          }
          fd_last = fd_derivatives;
        }  // for (double dx...) (displacement magnitudes)
        BOOST_CHECK_SMALL(min_error, fd_error_tol);
      }  // for (int disp_idx...) (displacement directions)
    }    // for (auto inputs...) (function inputs)
  }

  struct Hyp1F1RefFixture {
    Hyp1F1RefFixture() {
      std::vector<std::uint8_t> ref_data_ubjson;
      internal::read_binary_file(this->ref_filename, ref_data_ubjson);
      this->ref_data = json::from_ubjson(ref_data_ubjson);
    }

    ~Hyp1F1RefFixture() = default;

    std::string ref_filename = "reference_data/hyp1f1_reference.ubjson";

    json ref_data{};
    bool verbose{false};
  };

  struct Hyp1f1SphericalExpansionFixture {
    Hyp1f1SphericalExpansionFixture() {
      for (auto & l_max : l_maxs) {
        for (auto & n_max : n_maxs) {
          hyp1f1.emplace_back(false, 1e-14);
          hyp1f1.back().precompute(n_max, l_max);
          hyp1f1_recursion.emplace_back(true, 1e-14);
          hyp1f1_recursion.back().precompute(n_max, l_max);
        }
      }

      for (auto & rc : rcs) {
        facs_b.emplace_back();
        for (size_t il{0}; il < l_maxs.size();) {
          for (auto & n_max : n_maxs) {
            facs_b.back().emplace_back(n_max);
            for (int n{0}; n < n_max; ++n) {
              double sigma_n{(rc - smooth_width) * std::max(std::sqrt(n), 1.) /
                             n_max};
              facs_b.back().back()(n) = 0.5 * math::pow(sigma_n, 2);
            }
          }
          il++;
        }
      }
    }

    ~Hyp1f1SphericalExpansionFixture() = default;

    std::vector<int> l_maxs{{4, 5, 9, 15, 16, 20}};
    std::vector<int> n_maxs{{4, 5, 9, 15, 16, 20}};
    std::vector<math::Hyp1f1SphericalExpansion> hyp1f1{};
    std::vector<math::Hyp1f1SphericalExpansion> hyp1f1_recursion{};
    std::vector<std::vector<Eigen::VectorXd>> facs_b{};
    std::vector<double> r_ijs{1., 2., 3., 4., 5.5, 6.5, 7.5, 7.9};
    std::vector<double> fac_as{0.4};
    std::vector<double> rcs{2., 3., 5., 7., 8.};
    double smooth_width{0.5};
    bool verbose{false};
  };

  struct Hyp1f1GradientProvider {

    Hyp1f1GradientProvider(size_t max_radial, size_t max_angular,
                           double fac_a, Eigen::Ref<Eigen::VectorXd> fac_b) :
    max_radial{max_radial}, max_angular{max_angular}, fac_a{fac_a} {
      this->fac_b.resize(max_angular, 1);
      this->fac_b = fac_b;
      this->hyp1f1_calculator.precompute(max_radial, max_angular);
      // std::cout << "constructor ok" << std::endl;
    }

    ~Hyp1f1GradientProvider() = default;

    Eigen::Ref<Eigen::Array<double, 1, Eigen::Dynamic>>
    f(const Eigen::Matrix<double, 1, 1> & input_v) {
      this->hyp1f1_calculator.calc(input_v(0), this->fac_a, this->fac_b);
      Eigen::MatrixXd result(this->max_radial, this->max_angular + 1);
      result = this->hyp1f1_calculator.get_values();
      Eigen::Map<Eigen::Array<double, 1, Eigen::Dynamic>> result_flat(
          result.data(), 1, result.size());
      return result_flat;
    }

    Eigen::Ref<Eigen::Array<double, 1, Eigen::Dynamic>>
    grad_f(const Eigen::Matrix<double, 1, 1> & input_v) {
      this->hyp1f1_calculator.calc(input_v(0), this->fac_a, this->fac_b, true);
      Eigen::MatrixXd result(this->max_radial, this->max_angular + 1);
      result = this->hyp1f1_calculator.get_derivatives();
      // result.transpose() *=
      //   ((2.*this->fac_a*this->fac_a * input_v(0)) /
      //    (this->fac_a + this->fac_b.array())).matrix().asDiagonal();
      // result -= this->hyp1f1_calculator.get_values() * 2*fac_a*input_v(0);
      Eigen::Map<Eigen::Array<double, 1, Eigen::Dynamic>> result_flat(
          result.data(), 1, result.size());
      return result_flat;
    }

    math::Hyp1f1SphericalExpansion hyp1f1_calculator{true, 1e-13, 200};
    size_t max_radial;
    size_t max_angular;
    double fac_a{};
    Eigen::VectorXd fac_b{};

  };

}  // namespace rascal

#endif  // TESTS_TEST_MATH_HH_
