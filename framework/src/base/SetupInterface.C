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

#include "SetupInterface.h"
#include "Conversion.h"

template<>
InputParameters validParams<SetupInterface>()
{
  InputParameters params = emptyInputParameters();

  // Get an MooseEnum of the avaible 'execute_on' optoins
  std::vector<MooseEnum> execute_options(SetupInterface::getExecuteOptions());

  // Add the 'execute_on' input parameter for users to set
  params.addParam<std::vector<MooseEnum> >("execute_on", execute_options, "Set to (residual|jacobian|timestep|timestep_begin|custom) to execute only at that moment");

  return params;
}

SetupInterface::SetupInterface(InputParameters & params)
{
  /**
   * While many of the MOOSE systems inherit from this interface, it doesn't make sense for them all to adjust their execution flags.
   * Our way of dealing with this is by not having those particular classes add the this classes valid params to their own.  In
   * those cases it won't exist so we just set it to a default and ignore it.
   */
  if (params.have_parameter<std::vector<MooseEnum> >("execute_on"))
    _exec_flags = Moose::vectorStringsToEnum<ExecFlagType>(params.get<std::vector<MooseEnum> >("execute_on"));
  else
    _exec_flags.push_back(EXEC_RESIDUAL);
}

SetupInterface::~SetupInterface()
{
}

void
SetupInterface::initialSetup() {}

void
SetupInterface::timestepSetup() {}

void
SetupInterface::jacobianSetup() {}

void
SetupInterface::residualSetup() {}

void
SetupInterface::subdomainSetup() {}

const std::vector<ExecFlagType> &
SetupInterface::execFlags() const
{
  return _exec_flags;
}

std::vector<MooseEnum>
SetupInterface::getExecuteOptions()
{
  return std::vector<MooseEnum>(1, MooseEnum("initial residual jacobian timestep timestep_begin custom", "residual"));
}
