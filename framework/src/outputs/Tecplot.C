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

// Moose includes
#include "Tecplot.h"
#include "MooseApp.h"
#include "FEProblem.h"

// libMesh includes
#include "libmesh/tecplot_io.h"

template<>
InputParameters validParams<Tecplot>()
{
  // Get the base class parameters
  InputParameters params = validParams<OversampleOutput>();
  params += Output::disableOutputTypes();

  // Add binary toggle
  params.addParam<bool>("binary", false, "Set Tecplot files to output in binary format");
  params.addParamNamesToGroup("binary", "Advanced");

  // Add optional parameter to turn on appending to ASCII files
  params.addParam<bool>("ascii_append", false, "If true, append to an existing ASCII file rather than creating a new file each time");

  // Add description for the Tecplot class
  params.addClassDescription("Object for outputting data in the Tecplot format");

  // Return the InputParameters
  return params;
}

Tecplot::Tecplot(const std::string & name, InputParameters parameters) :
    OversampleOutput(name, parameters),
    _binary(getParam<bool>("binary")),
    _ascii_append(getParam<bool>("ascii_append")),
    _first_time(declareRestartableData<bool>("first_time", true))
{
#ifndef LIBMESH_HAVE_TECPLOT_API
  if (_binary)
  {
    mooseWarning("Teclplot binary output requested but not available, outputting ASCII format instead.");
    _binary = false;
  }
#endif

  // Force sequence output Note: This does not change the behavior for
  // this object b/c outputSetup() is empty, but it is placed here for
  // consistency.
  sequence(true);
}



void
Tecplot::output()
{
  TecplotIO out(*_mesh_ptr, _binary, time() + _app.getGlobalTimeOffset());

  // Only set the append flag on the TecplotIO object if the user has
  // asked for it, and this is not the first time we called output().
  if (_ascii_append && !_first_time)
    out.ascii_append() = true;

  out.write_equation_systems(filename(), *_es_ptr);

  // If we're not appending, increment the file number.  If we are appending,
  // we'll use the same filename each time.
  if (_binary || !_ascii_append)
    _file_num++;

  // If this was the first time we called output(), the next time will not be
  // the first time.
  if (_first_time)
    _first_time = false;
}

std::string
Tecplot::filename()
{
  std::ostringstream output;
  output << _file_base;

  // If not appending, put the padded time step in the filename.
  if (_binary || !_ascii_append)
    output << "_"
           << std::setw(_padding)
           << std::setprecision(0)
           << std::setfill('0')
           << std::right
           << _file_num;

  // .plt extension is for binary files
  // .dat extension is for ASCII files
  if (_binary)
    return output.str() + ".plt";
  else
    return output.str() + ".dat";
}
