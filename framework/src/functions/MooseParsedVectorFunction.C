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

#include "MooseParsedVectorFunction.h"

template<>
InputParameters validParams<MooseParsedVectorFunction>()
{
  InputParameters params = validParams<Function>();
  params += validParams<MooseParsedFunctionBase>();
  params.addParam<std::string>("value_x", "0", "x-component of function.");
  params.addParam<std::string>("value_y", "0", "y-component of function.");
  params.addParam<std::string>("value_z", "0", "z-component of function.");
  return params;
}

MooseParsedVectorFunction::MooseParsedVectorFunction(const std::string & name, InputParameters parameters) :
    Function(name, parameters),
    MooseParsedFunctionBase(name, parameters),
    _vector_value(verifyFunction(std::string("{") + getParam<std::string>("value_x") + "}{" +
                                 getParam<std::string>("value_y") + "}{" +
                                 getParam<std::string>("value_z") + "}")),
    _function_ptr(NULL)
{
}

MooseParsedVectorFunction::~MooseParsedVectorFunction()
{
  delete _function_ptr;
}

RealVectorValue
MooseParsedVectorFunction::vectorValue(Real t, const Point & p)
{
  return _function_ptr->evaluate<RealVectorValue>(t, p);
}

RealGradient
MooseParsedVectorFunction::gradient(Real /*t*/, const Point & /*p*/)
{
  mooseError("The gradient method is not defined in MooseParsedVectorFunction");
}

void
MooseParsedVectorFunction::initialSetup()
{
  if (_function_ptr == NULL)
    _function_ptr = new MooseParsedFunctionWrapper(_pfb_feproblem, _vector_value, _vars, _vals);
}
