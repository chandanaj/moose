/****************************************************************/
/*               DO NOT MODIFY THIS HEADER                      */
/* MOOSE - Multiphysics Object Oriented Simulation Environment  */
/*                                                              */
/*           (c) 2010 Battelle Energy Alliance, LLC             */
/*                   ALL RIGHTS RESERVED                        */
/*                                                              */
/*          Prepared by Battelle Energy Alliance, LLC           */
/*            Under Contract No. DE-AC07-05ID14517              */
/*            With the U. S. Department of Energy               */
/*                                                              */
/*            See COPYRIGHT for full restrictions               */
/****************************************************************/

// MOOSE includes
#include "VariableResidualNormsDebugOutput.h"
#include "FEProblem.h"
#include "MooseApp.h"
#include "Material.h"
#include "Console.h"

// libMesh includesx
#include "libmesh/transient_system.h"

template<>
InputParameters validParams<VariableResidualNormsDebugOutput>()
{
  InputParameters params = validParams<PetscOutput>();
  params += Output::disableOutputTypes();
  return params;
}

VariableResidualNormsDebugOutput::VariableResidualNormsDebugOutput(const std::string & name, InputParameters & parameters) :
    PetscOutput(name, parameters),
    _sys(_problem_ptr->getNonlinearSystem().sys())
{
  // Force this outputter to output on nonlinear residuals
  _output_nonlinear = true;
}

VariableResidualNormsDebugOutput::~VariableResidualNormsDebugOutput()
{
}

void
VariableResidualNormsDebugOutput::output()
{
  // Only show variable residual norms on Nonlinear iterations
  if (!onNonlinearResidual())
    return;

  // Stream for outputting
  std::ostringstream oss;

  // Determine the maximum variable name size
  unsigned int max_name_size = 0;
  for (unsigned int var_num = 0; var_num < _sys.n_vars(); var_num++)
  {
    unsigned int var_name_size = _sys.variable_name(var_num).size();
    if (var_name_size > max_name_size)
      max_name_size = var_name_size;
  }

  // Perform the output of the variable residuals
  oss << "    |residual|_2 of individual variables:\n";
  for (unsigned int var_num = 0; var_num < _sys.n_vars(); var_num++)
  {
    Real var_res_id = _sys.calculate_norm(*_sys.rhs,var_num,DISCRETE_L2);
    oss << std::setw(27-max_name_size) << " " << std::setw(max_name_size+2) //match position of overall NL residual
        << std::left << _sys.variable_name(var_num) + ":" << var_res_id << "\n";
  }

  _console << oss.str() << std::flush;
}
