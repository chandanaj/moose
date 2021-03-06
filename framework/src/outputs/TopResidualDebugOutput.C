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
#include "TopResidualDebugOutput.h"
#include "FEProblem.h"
#include "MooseApp.h"
#include "Material.h"
#include "Console.h"

// libMesh includesx
#include "libmesh/transient_system.h"

template<>
InputParameters validParams<TopResidualDebugOutput>()
{
  InputParameters params = validParams<PetscOutput>();
  params += Output::disableOutputTypes();

  // Create parameters for allowing debug outputter to be defined within the [Outputs] block
  params.addParam<unsigned int>("num_residuals", 0, "The number of top residuals to print out (0 = no output)");

  // By default operate on both nonlinear and linear residuals
  params.set<bool>("nonlinear_residuals") = true;
  params.set<bool>("linear_residuals") = true;

  return params;
}

TopResidualDebugOutput::TopResidualDebugOutput(const std::string & name, InputParameters & parameters) :
    PetscOutput(name, parameters),
    _num_residuals(getParam<unsigned int>("num_residuals")),
    _sys(_problem_ptr->getNonlinearSystem().sys())
{
}

TopResidualDebugOutput::~TopResidualDebugOutput()
{
}

void
TopResidualDebugOutput::output()
{
  // Display the top residuals
  if (_num_residuals > 0)
    printTopResiduals(*(_sys.rhs), _num_residuals);
}

void
TopResidualDebugOutput::printTopResiduals(const NumericVector<Number> & residual, unsigned int n)
{
  // Need a reference to the libMesh mesh object
  MeshBase & mesh = _problem_ptr->mesh().getMesh();

  std::vector<TopResidualDebugOutputTopResidualData> vec;
  vec.resize(residual.local_size());

  unsigned int j = 0;
  MeshBase::node_iterator it = mesh.local_nodes_begin();
  const MeshBase::node_iterator end = mesh.local_nodes_end();
  for (; it != end; ++it)
  {
    Node & node = *(*it);
    dof_id_type nd = node.id();

    for (unsigned int var = 0; var < node.n_vars(_sys.number()); ++var)
      if (node.n_dofs(_sys.number(), var) > 0)
      {
        dof_id_type dof_idx = node.dof_number(_sys.number(), var, 0);
        vec[j] = TopResidualDebugOutputTopResidualData(var, nd, residual(dof_idx));
        j++;
      }
  }

  // Sort vec by residuals
  std::sort(vec.begin(), vec.end(), sortTopResidualData);

  // Display the residuals
  Moose::err << "[DBG][" << processor_id() << "] Max " << n << " residuals";
  if (j < n)
  {
    n = j;
    Moose::err << " (Only " << n << " available)";
  }
  Moose::err << std::endl;

  for (unsigned int i = 0; i < n; ++i)
    Moose::err << "[DBG][" << processor_id() << "] " << std::setprecision(15) << vec[i]._residual << " '"
               << _sys.variable_name(vec[i]._var).c_str() << "' at node " << vec[i]._nd << '\n';
}
