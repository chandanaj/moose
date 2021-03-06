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

#include "SolutionTimeAdaptiveDT.h"
#include "FEProblem.h"
#include "Transient.h"

template<>
InputParameters validParams<SolutionTimeAdaptiveDT>()
{
  InputParameters params = validParams<TimeStepper>();
  params.addParam<Real>("percent_change", 0.1, "Percentage to change the timestep by.  Should be between 0 and 1");
  params.addParam<int>("initial_direction", 1, "Direction for the first step.  1 for up... -1 for down. ");
  params.addParam<bool>("adapt_log", false,    "Output adaptive time step log");
  params.addRequiredParam<Real>("dt", "The timestep size between solves");

  return params;
}

SolutionTimeAdaptiveDT::SolutionTimeAdaptiveDT(const std::string & name, InputParameters parameters) :
    TimeStepper(name, parameters),
    _direction(getParam<int>("initial_direction")),
    _percent_change(getParam<Real>("percent_change")),
    _older_sol_time_vs_dt(std::numeric_limits<Real>::max()),
    _old_sol_time_vs_dt(std::numeric_limits<Real>::max()),
    _sol_time_vs_dt(std::numeric_limits<Real>::max()),
    _adapt_log(getParam<bool>("adapt_log"))
{
  if ((_adapt_log) && (processor_id() == 0))
  {
    _adaptive_log.open("adaptive_log");
    _adaptive_log<<"Adaptive Times Step Log"<<std::endl;
  }
}

SolutionTimeAdaptiveDT::~SolutionTimeAdaptiveDT()
{
  _adaptive_log.close();
}

void
SolutionTimeAdaptiveDT::preSolve()
{
  gettimeofday(&_solve_start, NULL);
}

void
SolutionTimeAdaptiveDT::postSolve()
{
  if (converged())
  {
    gettimeofday (&_solve_end, NULL);
    double elapsed_time = (static_cast<double>(_solve_end.tv_sec  - _solve_start.tv_sec) +
                           static_cast<double>(_solve_end.tv_usec - _solve_start.tv_usec)*1.e-6);

    _older_sol_time_vs_dt = _old_sol_time_vs_dt;
    _old_sol_time_vs_dt = _sol_time_vs_dt;
    _sol_time_vs_dt = elapsed_time / _dt;
  }
}

Real
SolutionTimeAdaptiveDT::computeInitialDT()
{
  return getParam<Real>("dt");
}

Real
SolutionTimeAdaptiveDT::computeDT()
{
  //Ratio grew so switch direction
  if (_sol_time_vs_dt > _old_sol_time_vs_dt && _sol_time_vs_dt > _older_sol_time_vs_dt)
  {
    _direction *= -1;

    // Make sure we take at least two steps in this new direction
    _old_sol_time_vs_dt = std::numeric_limits<Real>::max();
    _older_sol_time_vs_dt = std::numeric_limits<Real>::max();
  }

//  if (_t_step > 1)
  Real local_dt =  _dt + _dt * _percent_change*_direction;

  if ((_adapt_log) && (processor_id() == 0))
  {
    Real out_dt = getCurrentDT();
    if (out_dt > _dt_max)
      out_dt = _dt_max;

    _adaptive_log<<"***Time step: "<<_t_step<<", time = "<<_time+out_dt<<std::endl;
    _adaptive_log<<"Cur DT: "<<out_dt<<std::endl;
    _adaptive_log<<"Older Ratio: "<<_older_sol_time_vs_dt<<std::endl;
    _adaptive_log<<"Old Ratio: "<<_old_sol_time_vs_dt<<std::endl;
    _adaptive_log<<"New Ratio: "<<_sol_time_vs_dt<<std::endl;
  }

  return local_dt;
}

void
SolutionTimeAdaptiveDT::rejectStep()
{
  _console << "Solve failed... cutting timestep" << std::endl;
  if (_adapt_log)
    _adaptive_log << "Solve failed... cutting timestep" << std::endl;

  TimeStepper::rejectStep();
}
