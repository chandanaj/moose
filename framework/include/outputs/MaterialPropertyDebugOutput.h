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

#ifndef MATERIALPROPERTYDEBUGOUTPUT_H
#define MATERIALPROPERTYDEBUGOUTPUT_H

// MOOSE includes
#include "PetscOutput.h"
#include "FEProblem.h"

// Forward declerations
class MaterialPropertyDebugOutput;

template<>
InputParameters validParams<MaterialPropertyDebugOutput>();

/**
 * A class for producing various debug related outputs
 *
 * This class may be used from inside the [Outputs] block or via the [Debug] block (preferred)
 */
class MaterialPropertyDebugOutput : public Output
{
public:

  /**
   * Class constructor
   * @param name Output object name
   * @param parameters Object input parameters
   */
  MaterialPropertyDebugOutput(const std::string & name, InputParameters & parameters);

  /**
   * Class destructor
   */
  virtual ~MaterialPropertyDebugOutput();

protected:

  /**
   * Perform the debugging output
   * For this object this is empty; the output is preformed in the constructor
   */
  virtual void output();

  /**
   * Prints material property information in a format similar to Moose system information
   */
  void printMaterialMap() const;

  /**
   * Builds a output streams for the properties in each material object
   * @param output The output stream to populate
   * @param materials Vector of pointers to the Material objects of interest
   */
  void printMaterialProperties(std::stringstream & output, const std::vector<Material * > & materials) const;

};

#endif // MATERIALPROPERTYEBUGOUTPUT_H
