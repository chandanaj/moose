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

#ifndef CONSTANTDT_H
#define CONSTANTDT_H

#include "TimeStepper.h"

class ConstantDT;

template<>
InputParameters validParams<ConstantDT>();

class ConstantDT : public TimeStepper
{
public:
  ConstantDT(const std::string & name, InputParameters parameters);

protected:
  virtual Real computeInitialDT();
  virtual Real computeDT();

private:
  const Real _constant_dt;
  const Real _growth_factor;
};

#endif /* CONSTANTDT_H */
