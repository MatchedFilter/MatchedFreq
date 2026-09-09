#include "Solver/TransientSolver.h"
#include "Solver/MnaMatrix.h"

#include <Eigen/Dense>
#include <algorithm>
#include <cmath>

namespace MFreq::Solver
{

namespace
{

constexpr double TIME_EPSILON_FACTOR = 0.1;

} // namespace

auto TransientSolver::Solve(const Netlist::CircuitNetlist &netlist,
                            const Netlist::TransientAnalysis &config)
    -> std::expected<SimulationResult, std::string>
{
  if (config.step <= 0.0 || config.stopTime <= config.step)
  {
    return std::unexpected("Invalid transient analysis time parameters");
  }

  MnaMatrixManager matrixManager;
  matrixManager.BuildNodeMap(netlist.components);

  const auto systemSize =
      static_cast<Eigen::Index>(matrixManager.GetMatrixSize());
  if (systemSize == 0)
  {
    return std::unexpected("Circuit system size is zero");
  }

  SimulationResult result;

  // Track non-ground nodes in mapping order
  for (const auto &comp : netlist.components)
  {
    for (const auto &node : {comp.node1, comp.node2})
    {
      const int32_t nodeIndex = matrixManager.GetNodeIndex(node);
      if (nodeIndex >= 0 &&
          std::ranges::find(result.nodeNames, node) == result.nodeNames.end())
      {
        result.nodeNames.push_back(node);
      }
    }
  }

  const double timeStep = config.step;
  const double maxSimulationTime =
      config.stopTime + (timeStep * TIME_EPSILON_FACTOR);

  // Allocate MNA Matrices
  Eigen::MatrixXd mnaConductanceMatrix =
      Eigen::MatrixXd::Zero(systemSize, systemSize);
  Eigen::MatrixXd mnaCapacitanceMatrix =
      Eigen::MatrixXd::Zero(systemSize, systemSize);
  Eigen::VectorXd mnaSourceVector = Eigen::VectorXd::Zero(systemSize);

  size_t voltageSourceIndex = matrixManager.GetNodeCount();

  // Stamp components into static G, C, and b matrices
  for (const auto &comp : netlist.components)
  {
    const int32_t n1 = matrixManager.GetNodeIndex(comp.node1);
    const int32_t n2 = matrixManager.GetNodeIndex(comp.node2);

    switch (comp.type)
    {
    case Netlist::ComponentType::Resistor:
    {
      const double conductance = 1.0 / comp.value;
      if (n1 >= 0)
      {
        mnaConductanceMatrix(n1, n1) += conductance;
      }
      if (n2 >= 0)
      {
        mnaConductanceMatrix(n2, n2) += conductance;
      }
      if (n1 >= 0 && n2 >= 0)
      {
        mnaConductanceMatrix(n1, n2) -= conductance;
        mnaConductanceMatrix(n2, n1) -= conductance;
      }
      break;
    }
    case Netlist::ComponentType::Capacitor:
    {
      const double capacitance = comp.value;
      if (n1 >= 0)
      {
        mnaCapacitanceMatrix(n1, n1) += capacitance;
      }
      if (n2 >= 0)
      {
        mnaCapacitanceMatrix(n2, n2) += capacitance;
      }
      if (n1 >= 0 && n2 >= 0)
      {
        mnaCapacitanceMatrix(n1, n2) -= capacitance;
        mnaCapacitanceMatrix(n2, n1) -= capacitance;
      }
      break;
    }
    case Netlist::ComponentType::VoltageSource:
    {
      const auto vsRow = static_cast<Eigen::Index>(voltageSourceIndex++);
      if (n1 >= 0)
      {
        mnaConductanceMatrix(n1, vsRow) += 1.0;
        mnaConductanceMatrix(vsRow, n1) += 1.0;
      }
      if (n2 >= 0)
      {
        mnaConductanceMatrix(n2, vsRow) -= 1.0;
        mnaConductanceMatrix(vsRow, n2) -= 1.0;
      }
      mnaSourceVector(vsRow) = comp.value;
      break;
    }
    default:
      break;
    }
  }

  // Construct Transient Coefficient Matrix: A = G + (C / dt)
  const Eigen::MatrixXd coefficientMatrixA =
      mnaConductanceMatrix + (mnaCapacitanceMatrix / timeStep);
  const Eigen::PartialPivLU<Eigen::MatrixXd> luSolver(coefficientMatrixA);

  Eigen::VectorXd stateVectorX = Eigen::VectorXd::Zero(systemSize);
  double currentTime = 0.0;

  // Time-stepping loop using Implicit Euler integration
  while (currentTime <= maxSimulationTime)
  {
    TimeStepData step;
    step.time = currentTime;

    // Save node voltages (exclude branch currents from output vector)
    step.nodeVoltages.reserve(result.nodeNames.size());
    for (const auto &nodeName : result.nodeNames)
    {
      const int32_t idx = matrixManager.GetNodeIndex(nodeName);
      if (idx >= 0)
      {
        step.nodeVoltages.push_back(stateVectorX(idx));
      }
      else
      {
        step.nodeVoltages.push_back(0.0);
      }
    }

    result.steps.push_back(step);

    // Compute RHS for next time step: b_next = b + (C / dt) * x_previous
    const Eigen::VectorXd effectiveRhsB =
        mnaSourceVector + (mnaCapacitanceMatrix / timeStep) * stateVectorX;
    stateVectorX = luSolver.solve(effectiveRhsB);

    currentTime += timeStep;
  }

  return result;
}

} // namespace MFreq::Solver
