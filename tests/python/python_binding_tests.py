#!/usr/bin/env python3

import unittest
import faulthandler

from python_structure_manager_test import (
    TestStructureManagerCenters,
    TestNL,
    TestNLStrict,
    CenterSelectTest,
)
from python_representation_calculator_test import (
    TestSortedCoulombRepresentation,
    TestSphericalExpansionRepresentation,
    TestSphericalInvariantsRepresentation,
)
from python_models_test import TestNumericalKernelGradient, TestCosineKernel
from python_math_test import TestMath
from test_filter import FPSTest, CURTest
from python_utils_test import TestOptimalRadialBasis
from python_cg_test import TestCGUtils
from md_calculator_test import TestGenericMD


class SimpleCheck(unittest.TestCase):
    def setUp(self):
        self.truth = True

    def test_simple_example(self):
        self.assertTrue(self.truth)


if __name__ == "__main__":
    faulthandler.enable()

    unittest.main()
