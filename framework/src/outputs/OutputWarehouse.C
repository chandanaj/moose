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
#include "OutputWarehouse.h"
#include "Output.h"
#include "Console.h"
#include "FileOutput.h"
#include "Checkpoint.h"
#include "FEProblem.h"

#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

OutputWarehouse::OutputWarehouse() :
    _multiapp_level(0)
{
  // Set the reserved names
  _reserved.insert("none");                  // allows 'none' to be used as a keyword in 'outputs' parameter
  _reserved.insert("all");                   // allows 'all' to be used as a keyword in 'outputs' parameter
  _reserved.insert("_moose_debug_output");   // the [Debug] block creates this object
}

OutputWarehouse::~OutputWarehouse()
{
  // If the output buffer is not empty, it needs to be written
  if (_console_buffer.str().length())
    mooseConsole();
}

void
OutputWarehouse::initialSetup()
{
  for (std::vector<Output *>::const_iterator it = _object_ptrs.begin(); it != _object_ptrs.end(); ++it)
    (*it)->initialSetup();
}


void
OutputWarehouse::init()
{
  for (std::vector<Output *>::const_iterator it = _object_ptrs.begin(); it != _object_ptrs.end(); ++it)
    (*it)->init();
}

void
OutputWarehouse::timestepSetup()
{
  for (std::vector<Output *>::const_iterator it = _object_ptrs.begin(); it != _object_ptrs.end(); ++it)
  {
    (*it)->timestepSetupInternal();
    (*it)->timestepSetup();
  }
}

void
OutputWarehouse::addOutput(MooseSharedPointer<Output> & output)
{
  _all_ptrs.push_back(output);

  // Add the object to the warehouse storage, Checkpoint placed at end so they are called last
  Checkpoint * cp = dynamic_cast<Checkpoint *>(output.get());
  if (cp != NULL)
    _object_ptrs.push_back(output.get());
  else
    _object_ptrs.insert(_object_ptrs.begin(), output.get());

  // Store the name and pointer
  _object_map[output->name()] = output.get();
  _object_names.insert(output->name());

  // If the output object is a FileOutput then store the output filename
  FileOutput * ptr = dynamic_cast<FileOutput *>(output.get());
  if (ptr != NULL)
    addOutputFilename(ptr->filename());

  // Insert object sync times to the global set
  if (output->parameters().isParamValid("sync_times"))
  {
    std::vector<Real> sync_times = output->parameters().get<std::vector<Real> >("sync_times");
    _sync_times.insert(sync_times.begin(), sync_times.end());
  }
}

bool
OutputWarehouse::hasOutput(const std::string & name) const
{
  return _object_map.find(name) != _object_map.end();
}

const std::vector<Output *> &
OutputWarehouse::getOutputs() const
{
  return _object_ptrs;
}


const std::set<OutputName> &
OutputWarehouse::getOutputNames() const
{
  return _object_names;
}

void
OutputWarehouse::addOutputFilename(const OutFileBase & filename)
{
  if (_file_base_set.find(filename) != _file_base_set.end())
    mooseError("An output file with the name, " << filename << ", already exists.");
  _file_base_set.insert(filename);
}

void
OutputWarehouse::outputInitial()
{
  for (std::vector<Output *>::const_iterator it = _object_ptrs.begin(); it != _object_ptrs.end(); ++it)
    (*it)->outputInitial();
}

void
OutputWarehouse::outputFailedStep()
{
  for (std::vector<Output *>::const_iterator it = _object_ptrs.begin(); it != _object_ptrs.end(); ++it)
    (*it)->outputFailedStep();
}

void
OutputWarehouse::outputStep()
{
  for (std::vector<Output *>::const_iterator it = _object_ptrs.begin(); it != _object_ptrs.end(); ++it)
    (*it)->outputStep();
}

void
OutputWarehouse::outputFinal()
{
  for (std::vector<Output *>::const_iterator it = _object_ptrs.begin(); it != _object_ptrs.end(); ++it)
    (*it)->outputFinal();
}

void
OutputWarehouse::meshChanged()
{
  for (std::vector<Output *>::const_iterator it = _object_ptrs.begin(); it != _object_ptrs.end(); ++it)
    (*it)->meshChanged();
}

void
OutputWarehouse::allowOutput(bool state)
{
  for (std::vector<Output *>::const_iterator it = _object_ptrs.begin(); it != _object_ptrs.end(); ++it)
    (*it)->allowOutput(state);
}

void
OutputWarehouse::forceOutput()
{
  for (std::vector<Output *>::const_iterator it = _object_ptrs.begin(); it != _object_ptrs.end(); ++it)
    (*it)->forceOutput();
}

void
OutputWarehouse::mooseConsole()
{
  // Loop through all Console Output objects and pass the current output buffer
  std::vector<Console *> objects = getOutputs<Console>();
  if (!objects.empty())
  {
    for (std::vector<Console *>::iterator it = objects.begin(); it != objects.end(); ++it)
      (*it)->mooseConsole(_console_buffer.str());

    // Reset
    _console_buffer.clear();
    _console_buffer.str("");
  }
}

void
OutputWarehouse::setFileNumbers(std::map<std::string, unsigned int> input, unsigned int offset)
{
  for (std::vector<Output *>::const_iterator it = _object_ptrs.begin(); it != _object_ptrs.end(); ++it)
  {
    FileOutput * ptr = dynamic_cast<FileOutput *>(*it);
    if (ptr != NULL)
    {
      std::map<std::string, unsigned int>::const_iterator it = input.find(ptr->name());
      if (it != input.end())
      {
        int value = it->second + offset;
        if (value < 0)
          ptr->setFileNumber(0);
        else
          ptr->setFileNumber(it->second + offset);
      }
    }
  }
}

std::map<std::string, unsigned int>
OutputWarehouse::getFileNumbers()
{
  std::map<std::string, unsigned int> output;
  for (std::vector<Output *>::const_iterator it = _object_ptrs.begin(); it != _object_ptrs.end(); ++it)
  {
    FileOutput * ptr = dynamic_cast<FileOutput *>(*it);
    if (ptr != NULL)
      output[ptr->name()] = ptr->getFileNumber();
  }
  return output;
}

void
OutputWarehouse::setCommonParameters(InputParameters * params_ptr)
{
  _common_params_ptr = params_ptr;
}

InputParameters *
OutputWarehouse::getCommonParameters()
{
  return _common_params_ptr;
}

std::set<Real> &
OutputWarehouse::getSyncTimes()
{
  return _sync_times;
}

void
OutputWarehouse::addInterfaceHideVariables(const std::string & output_name, const std::set<std::string> & variable_names)
{
  _interface_map[output_name].insert(variable_names.begin(), variable_names.end());
}

void
OutputWarehouse::buildInterfaceHideVariables(const std::string & output_name, std::set<std::string> & hide)
{
  std::map<std::string, std::set<std::string> >::const_iterator it = _interface_map.find(output_name);
  if (it != _interface_map.end())
    hide = it->second;
}

void
OutputWarehouse::checkOutputs(const std::set<OutputName> & names)
{
  for (std::set<OutputName>::const_iterator it = names.begin(); it != names.end(); ++it)
    if (!isReservedName(*it) && !hasOutput(*it))
      mooseError("The output object '" << *it << "' is not a defined output object");
}

const std::set<std::string> &
OutputWarehouse::getReservedNames() const
{
  return _reserved;
}

bool
OutputWarehouse::isReservedName(const std::string & name)
{
  return _reserved.find(name) != _reserved.end();
}
