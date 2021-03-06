/*****************************************/
/* Written by andrew.wilkins@csiro.au    */
/* Please contact me if you make changes */
/*****************************************/

#ifndef RICHARDSRELPERMAUX_H
#define RICHARDSRELPERMAUX_H

#include "AuxKernel.h"

#include "RichardsRelPerm.h"

//Forward Declarations
class RichardsRelPermAux;

template<>
InputParameters validParams<RichardsRelPermAux>();

/**
 * Relative Permeability as a function of effective saturation
 */
class RichardsRelPermAux: public AuxKernel
{
public:
  RichardsRelPermAux(const std::string & name, InputParameters parameters);

protected:
  virtual Real computeValue();

  /// effective saturation
  VariableValue & _seff_var;

  /// userobject that defines relative permeability function
  const RichardsRelPerm & _relperm_UO;
};

#endif // RICHARDSRELPERMAUX_H
