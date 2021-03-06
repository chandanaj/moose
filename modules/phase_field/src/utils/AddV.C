#include "AddV.h"

InputParameters &
AddV(InputParameters & parameters, const std::string & var_name)
{
  unsigned int op_num = parameters.get<unsigned int>("op_num");
  std::string var_name_base = parameters.get<std::string>("var_name_base");

  //Create variable names
  std::vector<VariableName> v;
  v.resize(op_num);

  if (op_num > 0)
  {
    for (unsigned int op = 0; op < op_num; op++)
    {
      std::string coupled_var_name = var_name_base;
      std::stringstream out;
      out << op;
      coupled_var_name.append(out.str());
      v[op] = coupled_var_name;
    }

    parameters.remove(var_name);
    parameters.set<std::vector<VariableName> >(var_name) = v;
  }

  return parameters;
}
