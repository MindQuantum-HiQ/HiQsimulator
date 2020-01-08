//   Copyright 2019 <Huawei Technologies Co., Ltd>
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

#define _STABSIM_TEST_ENV
#include "stabilizer-simulator/StabilizerSimulator.hpp"
#undef _STABSIM_TEST_ENV

#define BOOST_TEST_MODULE stabilizer_simulator_test
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <random>

#ifdef HAS_PYTHON
#     include <boost/process/system.hpp>
#     include <fstream>
#endif  // HAS_PYTHON

struct default_setup
{
     static auto create_simulator(unsigned num_qubits)
     {
          std::random_device rd;
          std::mt19937 gen(rd());
          std::uniform_int_distribution<unsigned> dist(0, 4294967295);

          StabilizerSimulator sim(num_qubits, dist(gen));
          sim.allocate_all_qubits();
          return sim;
     }

     default_setup(unsigned num_qubits_a = 500)
         : num_qubits(num_qubits_a), sim(create_simulator(num_qubits))
     {}

     unsigned num_qubits;
     StabilizerSimulator sim;
};

BOOST_FIXTURE_TEST_CASE(cnot_gate, default_setup)
{
     std::vector<std::vector<int>> test_set{
         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
         {0, 0, 0, 1, 0, 0, 1, 0, 1, 0}, {0, 0, 0, 1, 1, 0, 1, 0, 1, 1},
         {0, 0, 1, 0, 0, 0, 0, 1, 0, 0}, {0, 0, 1, 0, 1, 0, 0, 1, 0, 1},
         {0, 0, 1, 1, 0, 0, 1, 1, 1, 0}, {0, 0, 1, 1, 1, 0, 1, 1, 1, 1},
         {0, 1, 0, 0, 0, 0, 1, 0, 0, 0}, {0, 1, 0, 0, 1, 0, 1, 0, 0, 1},
         {0, 1, 0, 1, 0, 0, 0, 0, 1, 0}, {0, 1, 0, 1, 1, 0, 0, 0, 1, 1},
         {0, 1, 1, 0, 0, 0, 1, 1, 0, 0}, {0, 1, 1, 0, 1, 0, 1, 1, 0, 1},
         {0, 1, 1, 1, 0, 0, 0, 1, 1, 0}, {0, 1, 1, 1, 1, 0, 0, 1, 1, 1},
         {1, 0, 0, 0, 0, 1, 0, 1, 0, 0}, {1, 0, 0, 0, 1, 1, 0, 1, 0, 1},
         {1, 0, 0, 1, 0, 1, 1, 1, 1, 1}, {1, 0, 0, 1, 1, 1, 1, 1, 1, 0},
         {1, 0, 1, 0, 0, 1, 0, 0, 0, 0}, {1, 0, 1, 0, 1, 1, 0, 0, 0, 1},
         {1, 0, 1, 1, 0, 1, 1, 0, 1, 0}, {1, 0, 1, 1, 1, 1, 1, 0, 1, 1},
         {1, 1, 0, 0, 0, 1, 1, 1, 0, 0}, {1, 1, 0, 0, 1, 1, 1, 1, 0, 1},
         {1, 1, 0, 1, 0, 1, 0, 1, 1, 0}, {1, 1, 0, 1, 1, 1, 0, 1, 1, 1},
         {1, 1, 1, 0, 0, 1, 1, 0, 0, 0}, {1, 1, 1, 0, 1, 1, 1, 0, 0, 1},
         {1, 1, 1, 1, 0, 1, 0, 0, 1, 1}, {1, 1, 1, 1, 1, 1, 0, 0, 1, 0}};

     unsigned qubit0 = 0;
     unsigned qubit1 = 1;
     unsigned test_stabilizers[] = {0, num_qubits + 7};

     for (auto test_stab: test_stabilizers) {
          // only test_stab stabilizer is used all others are irrelevant
          for (auto test: test_set) {
               sim.set_stab_x_(test_stab, qubit0, test[0]);
               sim.set_stab_z_(test_stab, qubit0, test[1]);
               sim.set_stab_x_(test_stab, qubit1, test[2]);
               sim.set_stab_z_(test_stab, qubit1, test[3]);
               sim.set_phase_bit_(test_stab, test[4]);

               sim.CNOT(qubit0, qubit1);
               sim.sync();

               BOOST_TEST(sim.get_stab_x_(test_stab, qubit0) == test[5]);
               BOOST_TEST(sim.get_stab_z_(test_stab, qubit0) == test[6]);
               BOOST_TEST(sim.get_stab_x_(test_stab, qubit1) == test[7]);
               BOOST_TEST(sim.get_stab_z_(test_stab, qubit1) == test[8]);
               BOOST_TEST(sim.get_phase_bit_(test_stab) == test[9]);
          }
     }
}

BOOST_FIXTURE_TEST_CASE(hadamard_gate, default_setup)
{
     std::vector<std::vector<int>> test_set{
         {0, 0, 0, 0, 0, 0}, {0, 0, 1, 0, 0, 1}, {0, 1, 0, 1, 0, 0},
         {0, 1, 1, 1, 0, 1}, {1, 0, 0, 0, 1, 0}, {1, 0, 1, 0, 1, 1},
         {1, 1, 0, 1, 1, 1}, {1, 1, 1, 1, 1, 0}};

     unsigned qubit0 = 3;
     unsigned test_stabilizers[] = {0, num_qubits + 7};

     for (auto test_stab: test_stabilizers) {
          // only test_stab stabilizer is used all others are irrelevant
          for (auto test: test_set) {
               sim.set_stab_x_(test_stab, qubit0, test[0]);
               sim.set_stab_z_(test_stab, qubit0, test[1]);
               sim.set_phase_bit_(test_stab, test[2]);

               sim.H(qubit0);
               sim.sync();

               BOOST_TEST(sim.get_stab_x_(test_stab, qubit0) == test[3]);
               BOOST_TEST(sim.get_stab_z_(test_stab, qubit0) == test[4]);
               BOOST_TEST(sim.get_phase_bit_(test_stab) == test[5]);
          }
     }
}

BOOST_FIXTURE_TEST_CASE(s_gate, default_setup)
{
     constexpr unsigned num_qubits = 100;

     std::random_device rd;
     std::mt19937 gen(rd());
     std::uniform_int_distribution<unsigned> dist(0, 4294967295);

     StabilizerSimulator sim(num_qubits, dist(gen));
     sim.allocate_all_qubits();

     std::vector<std::vector<int>> test_set{
         {0, 0, 0, 0, 0, 0}, {0, 0, 1, 0, 0, 1}, {0, 1, 0, 0, 1, 0},
         {0, 1, 1, 0, 1, 1}, {1, 0, 0, 1, 1, 0}, {1, 0, 1, 1, 1, 1},
         {1, 1, 0, 1, 0, 1}, {1, 1, 1, 1, 0, 0}};

     unsigned qubit0 = 3;
     unsigned test_stabilizers[] = {0, num_qubits + 7};

     for (auto test_stab: test_stabilizers) {
          for (auto test: test_set) {
               sim.set_stab_x_(test_stab, qubit0, test[0]);
               sim.set_stab_z_(test_stab, qubit0, test[1]);
               sim.set_phase_bit_(test_stab, test[2]);

               sim.S(qubit0);
               sim.sync();

               BOOST_TEST(sim.get_stab_x_(test_stab, qubit0) == test[3]);
               BOOST_TEST(sim.get_stab_z_(test_stab, qubit0) == test[4]);
               BOOST_TEST(sim.get_phase_bit_(test_stab) == test[5]);
          }
     }
}

BOOST_FIXTURE_TEST_CASE(compute_phase_and_rowsum, default_setup)
{
     unsigned stabilizer_h = 10;
     unsigned stabilizer_i = 493;
     unsigned test_qubits[] = {0, num_qubits - 7};
     // Set initial state (x_h, z_h, r_h, x_i, z_i, r_i) and final state (x_h,
     // z_h, phase) We compute stabilizer0 <- stabilizer1 * stabilizer phase is
     // 0,1,2,3 (the actual phase is i^phase)
     std::vector<std::vector<unsigned>> test_set{
         {0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 1, 0, 0, 2},
         {0, 0, 0, 0, 1, 0, 0, 1, 0}, {0, 0, 0, 0, 1, 1, 0, 1, 2},
         {0, 0, 0, 1, 0, 0, 1, 0, 0}, {0, 0, 0, 1, 0, 1, 1, 0, 2},
         {0, 0, 0, 1, 1, 0, 1, 1, 0}, {0, 0, 0, 1, 1, 1, 1, 1, 2},
         {0, 0, 1, 0, 0, 0, 0, 0, 2}, {0, 0, 1, 0, 0, 1, 0, 0, 0},
         {0, 0, 1, 0, 1, 0, 0, 1, 2}, {0, 0, 1, 0, 1, 1, 0, 1, 0},
         {0, 0, 1, 1, 0, 0, 1, 0, 2}, {0, 0, 1, 1, 0, 1, 1, 0, 0},
         {0, 0, 1, 1, 1, 0, 1, 1, 2}, {0, 0, 1, 1, 1, 1, 1, 1, 0},
         {0, 1, 0, 0, 0, 0, 0, 1, 0}, {0, 1, 0, 0, 0, 1, 0, 1, 2},
         {0, 1, 0, 0, 1, 0, 0, 0, 0}, {0, 1, 0, 0, 1, 1, 0, 0, 2},
         {0, 1, 0, 1, 0, 0, 1, 1, 3}, {0, 1, 0, 1, 0, 1, 1, 1, 1},
         {0, 1, 0, 1, 1, 0, 1, 0, 1}, {0, 1, 0, 1, 1, 1, 1, 0, 3},
         {0, 1, 1, 0, 0, 0, 0, 1, 2}, {0, 1, 1, 0, 0, 1, 0, 1, 0},
         {0, 1, 1, 0, 1, 0, 0, 0, 2}, {0, 1, 1, 0, 1, 1, 0, 0, 0},
         {0, 1, 1, 1, 0, 0, 1, 1, 1}, {0, 1, 1, 1, 0, 1, 1, 1, 3},
         {0, 1, 1, 1, 1, 0, 1, 0, 3}, {0, 1, 1, 1, 1, 1, 1, 0, 1},
         {1, 0, 0, 0, 0, 0, 1, 0, 0}, {1, 0, 0, 0, 0, 1, 1, 0, 2},
         {1, 0, 0, 0, 1, 0, 1, 1, 1}, {1, 0, 0, 0, 1, 1, 1, 1, 3},
         {1, 0, 0, 1, 0, 0, 0, 0, 0}, {1, 0, 0, 1, 0, 1, 0, 0, 2},
         {1, 0, 0, 1, 1, 0, 0, 1, 3}, {1, 0, 0, 1, 1, 1, 0, 1, 1},
         {1, 0, 1, 0, 0, 0, 1, 0, 2}, {1, 0, 1, 0, 0, 1, 1, 0, 0},
         {1, 0, 1, 0, 1, 0, 1, 1, 3}, {1, 0, 1, 0, 1, 1, 1, 1, 1},
         {1, 0, 1, 1, 0, 0, 0, 0, 2}, {1, 0, 1, 1, 0, 1, 0, 0, 0},
         {1, 0, 1, 1, 1, 0, 0, 1, 1}, {1, 0, 1, 1, 1, 1, 0, 1, 3},
         {1, 1, 0, 0, 0, 0, 1, 1, 0}, {1, 1, 0, 0, 0, 1, 1, 1, 2},
         {1, 1, 0, 0, 1, 0, 1, 0, 3}, {1, 1, 0, 0, 1, 1, 1, 0, 1},
         {1, 1, 0, 1, 0, 0, 0, 1, 1}, {1, 1, 0, 1, 0, 1, 0, 1, 3},
         {1, 1, 0, 1, 1, 0, 0, 0, 0}, {1, 1, 0, 1, 1, 1, 0, 0, 2},
         {1, 1, 1, 0, 0, 0, 1, 1, 2}, {1, 1, 1, 0, 0, 1, 1, 1, 0},
         {1, 1, 1, 0, 1, 0, 1, 0, 1}, {1, 1, 1, 0, 1, 1, 1, 0, 3},
         {1, 1, 1, 1, 0, 0, 0, 1, 3}, {1, 1, 1, 1, 0, 1, 0, 1, 1},
         {1, 1, 1, 1, 1, 0, 0, 0, 2}, {1, 1, 1, 1, 1, 1, 0, 0, 0}};
     for (auto test_qubit: test_qubits) {
          for (auto test: test_set) {
               for (unsigned qubit = 0; qubit < num_qubits; ++qubit) {
                    sim.set_stab_x_(stabilizer_h, qubit, 0);
                    sim.set_stab_z_(stabilizer_h, qubit, 0);
                    sim.set_stab_x_(stabilizer_i, qubit, 0);
                    sim.set_stab_z_(stabilizer_i, qubit, 0);
               }
               sim.set_stab_x_(stabilizer_h, test_qubit, test[0]);
               sim.set_stab_z_(stabilizer_h, test_qubit, test[1]);
               sim.set_phase_bit_(stabilizer_h, test[2]);
               sim.set_stab_x_(stabilizer_i, test_qubit, test[3]);
               sim.set_stab_z_(stabilizer_i, test_qubit, test[4]);
               sim.set_phase_bit_(stabilizer_i, test[5]);

               auto result = sim.compute_phase_(stabilizer_h, stabilizer_i);
               BOOST_TEST(result == test[8]);

               sim.rowsum_(stabilizer_h, stabilizer_i);

               BOOST_TEST(sim.get_stab_x_(stabilizer_h, test_qubit) == test[6]);
               BOOST_TEST(sim.get_stab_z_(stabilizer_h, test_qubit) == test[7]);
               BOOST_TEST(sim.get_stab_x_(stabilizer_i, test_qubit) == test[3]);
               BOOST_TEST(sim.get_stab_z_(stabilizer_i, test_qubit) == test[4]);
          }
     }
}

BOOST_FIXTURE_TEST_CASE(deterministic_measurement, default_setup)
{
     for (auto i(0UL); i < num_qubits; ++i) {
          BOOST_TEST(sim.measure(i) == 0);

          sim.X(i);

          BOOST_TEST(sim.measure(i) == 1);
     }
}

#ifdef HAS_PYTHON

BOOST_AUTO_TEST_CASE(random_deterministic_measurement)
{
     std::random_device rd;
     std::mt19937 gen(rd());

     unsigned num_qubits = 300;
     unsigned num_measurements = 20;
     unsigned num_gates_between_meas = 800;

     std::uniform_int_distribution<unsigned> dist(0, 4294967295);
     std::uniform_int_distribution<int> bin_dist(0, 1);
     std::uniform_int_distribution<int> qubit_dist(0, num_qubits - 1);

     StabilizerSimulator sim(num_qubits, dist(gen));
     sim.allocate_all_qubits();

     std::ofstream outputfile("random_deterministic_measurement.py");
     outputfile << "import projectq\n"
                << "from projectq import MainEngine\n"
                << "from projectq.ops import X, CNOT, Measure, All\n"
                << "from projectq.backends import ClassicalSimulator\n\n"
                << "eng = MainEngine(ClassicalSimulator(), [])\n"
                << "x = eng.allocate_qureg(" << num_qubits << ")\n";

     for (unsigned cycle = 0; cycle < num_measurements; ++cycle) {
          // Apply random gates to random qubits
          for (unsigned gate = 0; gate < num_gates_between_meas; ++gate) {
               if (bin_dist(gen) == 0) {  // X
                    auto qubit_id = qubit_dist(gen);
                    sim.X(qubit_id);
                    outputfile << "X | x[" << qubit_id << "]\n";
               }
               else {  // CNOT
                    auto qubit0_id = qubit_dist(gen);
                    auto qubit1_id = qubit_dist(gen);
                    if (qubit0_id == qubit1_id)
                         qubit1_id = (qubit1_id + 1) % num_qubits;
                    sim.CNOT(qubit0_id, qubit1_id);
                    outputfile << "CNOT | (x[" << qubit0_id << "], x["
                               << qubit1_id << "])\n";
               }
          }
          outputfile << "All(Measure) | x\neng.flush()\n";
          for (unsigned id = 0; id < num_qubits; ++id) {
               outputfile << "assert int(x[" << id
                          << "]) == " << sim.measure(id) << "\n";
          }
          outputfile << "\n";
     }
     outputfile.close();

     BOOST_TEST(boost::process::system(PYTHON_EXECUTABLE,
                                       "random_deterministic_measurement.py")
                == 0);
}

BOOST_AUTO_TEST_CASE(test_random_measurement)
{
     std::random_device rd;
     std::mt19937 gen(rd());

     unsigned num_qubits = 15;
     unsigned num_cycles = 20;
     unsigned num_gates = 75;

     std::uniform_int_distribution<unsigned> dist(0, 4294967295);
     std::uniform_int_distribution<int> tri_dist(0, 2);
     std::uniform_int_distribution<int> bitstring_dist(0, 1);
     std::uniform_int_distribution<int> qubit_dist(0, num_qubits - 1);

     StabilizerSimulator sim(num_qubits, dist(gen));
     sim.allocate_all_qubits();

     std::ofstream outputfile("random_measurement.py");
     outputfile << "import numpy as np\n"
                << "import projectq\n"
                << "from projectq import MainEngine\n"
                << "from projectq.ops import H, S, CNOT, QubitOperator\n"
                << "eng = MainEngine(engine_list=[])\n"
                << "x = eng.allocate_qureg(" << num_qubits << ")\n";

     for (unsigned cycle = 0; cycle < num_cycles; ++cycle) {
          // Apply random gates to random qubits
          for (unsigned gate = 0; gate < num_gates; ++gate) {
               unsigned r = tri_dist(gen);
               if (r == 0) {  // H
                    auto qubit_id = qubit_dist(gen);
                    sim.H(qubit_id);
                    outputfile << "H | x[" << qubit_id << "]\n";
               }
               else if (r == 1) {  // S
                    auto qubit_id = qubit_dist(gen);
                    sim.S(qubit_id);
                    outputfile << "S | x[" << qubit_id << "]\n";
               }
               else {  // CNOT
                    auto qubit0_id = qubit_dist(gen);
                    auto qubit1_id = qubit_dist(gen);
                    if (qubit0_id == qubit1_id)
                         qubit1_id = (qubit1_id + 1) % num_qubits;
                    sim.CNOT(qubit0_id, qubit1_id);
                    outputfile << "CNOT | (x[" << qubit0_id << "], x["
                               << qubit1_id << "])\n";
               }
          }
          outputfile << "eng.flush()\n";
          sim.sync();

          for (unsigned stab = num_qubits; stab < 2 * num_qubits; ++stab) {
               outputfile << "op = (-1) ** " << sim.get_phase_bit_(stab)
                          << " * QubitOperator('";
               for (unsigned id = 0; id < num_qubits; ++id) {
                    char G[] = "IZXY";
                    unsigned idx = 2 * sim.get_stab_x_(stab, id)
                                   | sim.get_stab_z_(stab, id);
                    if (idx > 0)
                         outputfile << G[idx] << id << " ";
               }
               outputfile << "')\n"
                          << "assert np.isclose(1., "
                             "eng.backend.get_expectation_value(op, x))\n";
          }
          outputfile << "eng.flush()\n";
          sim.sync();

          for (unsigned int i(0); i < 100; ++i) {
               unsigned qb[4];
               for (unsigned int j(0); j < 4; ++j) {
                    bool ok(true);
                    int r(0);
                    do {
                         ok = true;
                         r = qubit_dist(gen);
                         for (auto el: qb) {
                              if (r == el) {
                                   ok = false;
                                   break;
                              }
                         }
                    } while (!ok);
                    qb[j] = r;
               }

               bool r1 = bitstring_dist(gen);
               bool r2 = bitstring_dist(gen);
               bool r3 = bitstring_dist(gen);
               bool r4 = bitstring_dist(gen);
               outputfile << "try:\n\t";
               outputfile << "assert np.isclose(eng.backend.get_probability('"
                          << r1 << r2 << r3 << r4 << "', "
                          << "[x[" << qb[0] << "]"
                          << ", x[" << qb[1] << "]"
                          << ", x[" << qb[2] << "]"
                          << ", x[" << qb[3] << "]]), "
                          << sim.get_probability({r1, r2, r3, r4},
                                                 {qb[0], qb[1], qb[2], qb[3]})
                          << ")\n";
               outputfile << "except:\n\tprint("
                          << "eng.backend.get_probability('" << r1 << r2 << r3
                          << r4 << "', "
                          << "[x[" << qb[0] << "]"
                          << ", x[" << qb[1] << "]"
                          << ", x[" << qb[2] << "]"
                          << ", x[" << qb[3] << "]])"
                          << ")\n\traise\n";
          }

          for (unsigned stab = num_qubits; stab < 2 * num_qubits; ++stab) {
               outputfile << "op = (-1) ** " << sim.get_phase_bit_(stab)
                          << " * QubitOperator('";
               for (unsigned id = 0; id < num_qubits; ++id) {
                    char G[] = "IZXY";
                    unsigned idx = 2 * sim.get_stab_x_(stab, id)
                                   | sim.get_stab_z_(stab, id);
                    if (idx > 0)
                         outputfile << G[idx] << id << " ";
               }
               outputfile << "')\n"
                          << "assert np.isclose(1., "
                             "eng.backend.get_expectation_value(op, x))\n";
          }
          for (unsigned id = 0; id < num_qubits; ++id) {
               outputfile << "assert eng.backend.get_probability('"
                          << sim.measure(id) << "', [x[" << id << "]])"
                          << " > 0.4\n";
               outputfile << "eng.backend.collapse_wavefunction([x[" << id
                          << "]], [" << sim.measure(id) << "])\n";
          }
          outputfile << "\n";
     }
     outputfile.close();
     BOOST_TEST(
         boost::process::system(PYTHON_EXECUTABLE, "random_measurement.py")
         == 0);
}

#endif  // HAS_PYTHON
