#include "DomainIntegralAction.h"

#include "Factory.h"
#include "FEProblem.h"
#include "Parser.h"
#include "libmesh/string_to_enum.h"
#include "CrackFrontDefinition.h"
#include "InteractionIntegralAuxFields.h"

template<>
InputParameters validParams<DomainIntegralAction>()
{
  InputParameters params = validParams<Action>();
  addCrackFrontDefinitionParams(params);
  MultiMooseEnum integral_vec("JIntegral InteractionIntegralKI InteractionIntegralKII InteractionIntegralKIII");
  params.addRequiredParam<MultiMooseEnum>("integrals", integral_vec, "Domain integrals to calculate.  Choices are: " + integral_vec.getRawNames());
  params.addParam<std::vector<BoundaryName> >("boundary", "The list of boundary IDs from the mesh where this boundary condition applies");
  params.addParam<std::string>("order", "FIRST",  "Specifies the order of the FE shape function to use for q AuxVariables");
  params.addParam<std::string>("family", "LAGRANGE", "Specifies the family of FE shape functions to use for q AuxVariables");
  params.addRequiredParam<std::vector<Real> >("radius_inner", "Inner radius for volume integral domain");
  params.addRequiredParam<std::vector<Real> >("radius_outer", "Outer radius for volume integral domain");
  params.addParam<std::vector<VariableName> >("output_variable", "Variable values to be reported along the crack front");
  params.addParam<Real>("poissons_ratio","Poisson's ratio");
  params.addParam<Real>("youngs_modulus","Young's modulus");
  params.addParam<std::vector<SubdomainName> >("block","The block ids where InteractionIntegralAuxFields is defined");
  params.addParam<VariableName>("disp_x", "The x displacement");
  params.addParam<VariableName>("disp_y", "The y displacement");
  params.addParam<VariableName>("disp_z", "The z displacement");
  return params;
}

DomainIntegralAction::DomainIntegralAction(const std::string & name, InputParameters params):
  Action(name, params),
  _boundary_names(getParam<std::vector<BoundaryName> >("boundary")),
  _order(getParam<std::string>("order")),
  _family(getParam<std::string>("family")),
  _direction_method_moose_enum(getParam<MooseEnum>("crack_direction_method")),
  _end_direction_method_moose_enum(getParam<MooseEnum>("crack_end_direction_method")),
  _have_crack_direction_vector(false),
  _have_crack_direction_vector_end_1(false),
  _have_crack_direction_vector_end_2(false),
  _treat_as_2d(getParam<bool>("2d")),
  _axis_2d(getParam<unsigned int>("axis_2d")),
  _radius_inner(getParam<std::vector<Real> >("radius_inner")),
  _radius_outer(getParam<std::vector<Real> >("radius_outer")),
  _use_displaced_mesh(false)
{
  if (isParamValid("crack_direction_vector"))
  {
    _crack_direction_vector = getParam<RealVectorValue>("crack_direction_vector");
    _have_crack_direction_vector = true;
  }
  if (isParamValid("crack_direction_vector_end_1"))
  {
    _crack_direction_vector_end_1 = getParam<RealVectorValue>("crack_direction_vector_end_1");
    _have_crack_direction_vector_end_1 = true;
  }
  if (isParamValid("crack_direction_vector_end_2"))
  {
    _crack_direction_vector_end_2 = getParam<RealVectorValue>("crack_direction_vector_end_2");
    _have_crack_direction_vector_end_2 = true;
  }
  if (isParamValid("crack_mouth_boundary"))
  {
    _crack_mouth_boundary_names = getParam<std::vector<BoundaryName> >("crack_mouth_boundary");
  }
  if (_radius_inner.size() != _radius_outer.size())
  {
    mooseError("Number of entries in 'radius_inner' and 'radius_outer' must match.");
  }

  MultiMooseEnum integral_moose_enums = getParam<MultiMooseEnum>("integrals");
  if (integral_moose_enums.size() == 0)
    mooseError("Must specify at least one domain integral to perform.");
  for (unsigned int i=0; i<integral_moose_enums.size(); ++i)
  {
    if (integral_moose_enums[i] != "JIntegral")
    {
      //Check that parameters required for interaction integrals are defined
      if (!(isParamValid("disp_x")) || !(isParamValid("disp_y")))
        mooseError("DomainIntegral error: must set displacements for integral: "<<integral_moose_enums[i]);

      if (!(isParamValid("poissons_ratio")) || !(isParamValid("youngs_modulus")))
        mooseError("DomainIntegral error: must set Poisson's ratio and Young's modulus for integral: "<<integral_moose_enums[i]);

      if (!(isParamValid("block")))
        mooseError("DomainIntegral error: must set block ID or name for integral: "<<integral_moose_enums[i]);

      _poissons_ratio = getParam<Real>("poissons_ratio");
      _youngs_modulus = getParam<Real>("youngs_modulus");
      _blocks = getParam<std::vector<SubdomainName> >("block");
      _disp_x = getParam<VariableName>("disp_x");
      _disp_y = getParam<VariableName>("disp_y");
      _disp_z = getParam<VariableName>("disp_z");

      //mooseError("Domain integral type not yet implemented: "<<integral_moose_enums[i]);
    }

    _integrals.insert(INTEGRAL(int(integral_moose_enums.get(i))));
  }

  if (isParamValid("output_variable"))
    _output_variables = getParam<std::vector<VariableName> >("output_variable");
}

DomainIntegralAction::~DomainIntegralAction()
{
}

void
DomainIntegralAction::act()
{
  const std::string uo_name("crackFrontDefinition");
  const std::string ak_base_name("q");
  const std::string av_base_name("q");
  const std::string pp_base_name("J");
  const unsigned int num_crack_front_nodes = calcNumCrackFrontNodes();

  if (_current_task == "add_user_object")
  {
    const std::string uo_type_name("CrackFrontDefinition");

    InputParameters params = _factory.getValidParams(uo_type_name);
    params.set<std::vector<MooseEnum> >("execute_on")[0] = "initial";
    params.set<MooseEnum>("crack_direction_method") = _direction_method_moose_enum;
    params.set<MooseEnum>("crack_end_direction_method") = _end_direction_method_moose_enum;
    if (_have_crack_direction_vector)
    {
      params.set<RealVectorValue>("crack_direction_vector") = _crack_direction_vector;
    }
    if (_have_crack_direction_vector_end_1)
    {
      params.set<RealVectorValue>("crack_direction_vector_end_1") = _crack_direction_vector_end_1;
    }
    if (_have_crack_direction_vector_end_2)
    {
      params.set<RealVectorValue>("crack_direction_vector_end_2") = _crack_direction_vector_end_2;
    }
    if (_crack_mouth_boundary_names.size() != 0)
    {
      params.set<std::vector<BoundaryName> >("crack_mouth_boundary") = _crack_mouth_boundary_names;
    }
    params.set<bool>("2d") = _treat_as_2d;
    params.set<unsigned int>("axis_2d") = _axis_2d;
    params.set<std::vector<BoundaryName> >("boundary") = _boundary_names;
    params.set<bool>("use_displaced_mesh") = _use_displaced_mesh;

    _problem->addUserObject(uo_type_name, uo_name, params);
  }
  else if (_current_task == "add_aux_variable")
  {
    for (unsigned int ring_index=0; ring_index<_radius_inner.size(); ++ring_index)
    {
      if (_treat_as_2d)
      {
        std::ostringstream av_name_stream;
        av_name_stream<<av_base_name<<"_"<<ring_index+1;
        _problem->addAuxVariable(av_name_stream.str(),
                                 FEType(Utility::string_to_enum<Order>(_order),
                                        Utility::string_to_enum<FEFamily>(_family)));
      }
      else
      {
        for (unsigned int cfn_index=0; cfn_index<num_crack_front_nodes; ++cfn_index)
        {
          std::ostringstream av_name_stream;
          av_name_stream<<av_base_name<<"_"<<cfn_index+1<<"_"<<ring_index+1;
          _problem->addAuxVariable(av_name_stream.str(),
                                   FEType(Utility::string_to_enum<Order>(_order),
                                          Utility::string_to_enum<FEFamily>(_family)));
        }
      }
    }
  }
  else if (_current_task == "add_aux_kernel")
  {
    const std::string ak_type_name("DomainIntegralQFunction");
    InputParameters params = _factory.getValidParams(ak_type_name);
    params.set<std::vector<MooseEnum> >("execute_on")[0] = "initial";
    params.set<UserObjectName>("crack_front_definition") = uo_name;
    params.set<bool>("use_displaced_mesh") = _use_displaced_mesh;

    for (unsigned int ring_index=0; ring_index<_radius_inner.size(); ++ring_index)
    {
      params.set<Real>("j_integral_radius_inner") = _radius_inner[ring_index];
      params.set<Real>("j_integral_radius_outer") = _radius_outer[ring_index];
      if (_treat_as_2d)
      {
        std::ostringstream ak_name_stream;
        ak_name_stream<<ak_base_name<<"_"<<ring_index+1;
        std::ostringstream av_name_stream;
        av_name_stream<<av_base_name<<"_"<<ring_index+1;
        params.set<AuxVariableName>("variable") = av_name_stream.str();
        _problem->addAuxKernel(ak_type_name, ak_name_stream.str(), params);
      }
      else
      {
        for (unsigned int cfn_index=0; cfn_index<num_crack_front_nodes; ++cfn_index)
        {
          std::ostringstream ak_name_stream;
          ak_name_stream<<ak_base_name<<"_"<<cfn_index+1<<"_"<<ring_index+1;
          std::ostringstream av_name_stream;
          av_name_stream<<av_base_name<<"_"<<cfn_index+1<<"_"<<ring_index+1;
          params.set<AuxVariableName>("variable") = av_name_stream.str();
          params.set<unsigned int>("crack_front_node_index") = cfn_index;
          _problem->addAuxKernel(ak_type_name, ak_name_stream.str(), params);
        }
      }
    }
  }
  else if (_current_task == "add_postprocessor")
  {
    if (_integrals.count(J_INTEGRAL) != 0)
    {
      const std::string pp_type_name("JIntegral");
      InputParameters params = _factory.getValidParams(pp_type_name);
      params.set<std::vector<MooseEnum> >("execute_on")[0] = "timestep";
      params.set<UserObjectName>("crack_front_definition") = uo_name;
      params.set<bool>("use_displaced_mesh") = _use_displaced_mesh;
      for (unsigned int ring_index=0; ring_index<_radius_inner.size(); ++ring_index)
      {
        if (_treat_as_2d)
        {
          std::ostringstream av_name_stream;
          av_name_stream<<av_base_name<<"_"<<ring_index+1;
          std::ostringstream pp_name_stream;
          pp_name_stream<<pp_base_name<<"_"<<ring_index+1;
          std::vector<VariableName> qvars;
          qvars.push_back(av_name_stream.str());
          params.set<std::vector<VariableName> >("q") = qvars;
          _problem->addPostprocessor(pp_type_name,pp_name_stream.str(),params);
        }
        else
        {
          for (unsigned int cfn_index=0; cfn_index<num_crack_front_nodes; ++cfn_index)
          {
            std::ostringstream av_name_stream;
            av_name_stream<<av_base_name<<"_"<<cfn_index+1<<"_"<<ring_index+1;
            std::ostringstream pp_name_stream;
            pp_name_stream<<pp_base_name<<"_"<<cfn_index+1<<"_"<<ring_index+1;
            std::vector<VariableName> qvars;
            qvars.push_back(av_name_stream.str());
            params.set<std::vector<VariableName> >("q") = qvars;
            params.set<unsigned int>("crack_front_node_index") = cfn_index;
            _problem->addPostprocessor(pp_type_name,pp_name_stream.str(),params);
          }
        }
      }
    }
    if (_integrals.count(INTERACTION_INTEGRAL_KI) != 0)
    {
      const std::string pp_base_name("II");
      const std::string pp_type_name("InteractionIntegral");
      InputParameters params = _factory.getValidParams(pp_type_name);
      params.set<std::vector<MooseEnum> >("execute_on")[0] = "timestep";
      params.set<UserObjectName>("crack_front_definition") = uo_name;
      params.set<bool>("use_displaced_mesh") = _use_displaced_mesh;
      params.set<Real>("poissons_ratio") = _poissons_ratio;
      params.set<Real>("youngs_modulus") = _youngs_modulus;
      params.set<std::vector<VariableName> >("disp_x") = std::vector<VariableName>(1,_disp_x);
      params.set<std::vector<VariableName> >("disp_y") = std::vector<VariableName>(1,_disp_y);
      if (_disp_z !="")
        params.set<std::vector<VariableName> >("disp_z") = std::vector<VariableName>(1,_disp_z);
      params.set<std::string>("aux_stress") = "aux_stress_I";
      params.set<std::string>("aux_disp") = "aux_disp_I";
      params.set<std::string>("aux_grad_disp") = "aux_grad_disp_I";
      params.set<std::string>("aux_strain") = "aux_strain_I";
      params.set<Real>("K_factor") = 0.5 * _youngs_modulus / (1 - std::pow(_poissons_ratio,2));
      for (unsigned int ring_index=0; ring_index<_radius_inner.size(); ++ring_index)
      {
        if (_treat_as_2d)
        {
          std::ostringstream av_name_stream;
          av_name_stream<<av_base_name<<"_"<<ring_index+1;
          std::ostringstream pp_name_stream;
          pp_name_stream<<pp_base_name<<"_"<<"KI"<<"_"<<ring_index+1;
          std::vector<VariableName> qvars;
          qvars.push_back(av_name_stream.str());
          params.set<std::vector<VariableName> >("q") = qvars;
          _problem->addPostprocessor(pp_type_name,pp_name_stream.str(),params);
        }
        else
        {
          for (unsigned int cfn_index=0; cfn_index<num_crack_front_nodes; ++cfn_index)
          {
            std::ostringstream av_name_stream;
            av_name_stream<<av_base_name<<"_"<<cfn_index+1<<"_"<<ring_index+1;
            std::ostringstream pp_name_stream;
            pp_name_stream<<pp_base_name<<"_"<<"KI"<<"_"<<cfn_index+1<<"_"<<ring_index+1;
            std::vector<VariableName> qvars;
            qvars.push_back(av_name_stream.str());
            params.set<std::vector<VariableName> >("q") = qvars;
            params.set<unsigned int>("crack_front_node_index") = cfn_index;
            _problem->addPostprocessor(pp_type_name,pp_name_stream.str(),params);
          }
        }
      }
    }
    if (_integrals.count(INTERACTION_INTEGRAL_KII) != 0)
    {
      const std::string pp_base_name("II");
      const std::string pp_type_name("InteractionIntegral");
      InputParameters params = _factory.getValidParams(pp_type_name);
      params.set<std::vector<MooseEnum> >("execute_on")[0] = "timestep";
      params.set<UserObjectName>("crack_front_definition") = uo_name;
      params.set<bool>("use_displaced_mesh") = _use_displaced_mesh;
      params.set<Real>("poissons_ratio") = _poissons_ratio;
      params.set<Real>("youngs_modulus") = _youngs_modulus;
      params.set<std::vector<VariableName> >("disp_x") = std::vector<VariableName>(1,_disp_x);
      params.set<std::vector<VariableName> >("disp_y") = std::vector<VariableName>(1,_disp_y);
      if (_disp_z !="")
        params.set<std::vector<VariableName> >("disp_z") = std::vector<VariableName>(1,_disp_z);
      params.set<std::string>("aux_stress") = "aux_stress_II";
      params.set<std::string>("aux_disp") = "aux_disp_II";
      params.set<std::string>("aux_grad_disp") = "aux_grad_disp_II";
      params.set<std::string>("aux_strain") = "aux_strain_II";
      params.set<Real>("K_factor") = 0.5 * _youngs_modulus / (1 - std::pow(_poissons_ratio,2));
      for (unsigned int ring_index=0; ring_index<_radius_inner.size(); ++ring_index)
      {
        if (_treat_as_2d)
        {
          std::ostringstream av_name_stream;
          av_name_stream<<av_base_name<<"_"<<ring_index+1;
          std::ostringstream pp_name_stream;
          pp_name_stream<<pp_base_name<<"_"<<"KII"<<"_"<<ring_index+1;
          std::vector<VariableName> qvars;
          qvars.push_back(av_name_stream.str());
          params.set<std::vector<VariableName> >("q") = qvars;
          _problem->addPostprocessor(pp_type_name,pp_name_stream.str(),params);
        }
        else
        {
          for (unsigned int cfn_index=0; cfn_index<num_crack_front_nodes; ++cfn_index)
          {
            std::ostringstream av_name_stream;
            av_name_stream<<av_base_name<<"_"<<cfn_index+1<<"_"<<ring_index+1;
            std::ostringstream pp_name_stream;
            pp_name_stream<<pp_base_name<<"_"<<"KII"<<"_"<<cfn_index+1<<"_"<<ring_index+1;
            std::vector<VariableName> qvars;
            qvars.push_back(av_name_stream.str());
            params.set<std::vector<VariableName> >("q") = qvars;
            params.set<unsigned int>("crack_front_node_index") = cfn_index;
            _problem->addPostprocessor(pp_type_name,pp_name_stream.str(),params);
          }
        }
      }
    }
    if (_integrals.count(INTERACTION_INTEGRAL_KIII) != 0)
    {
      const std::string pp_base_name("II");
      const std::string pp_type_name("InteractionIntegral");
      InputParameters params = _factory.getValidParams(pp_type_name);
      params.set<std::vector<MooseEnum> >("execute_on")[0] = "timestep";
      params.set<UserObjectName>("crack_front_definition") = uo_name;
      params.set<bool>("use_displaced_mesh") = _use_displaced_mesh;
      params.set<Real>("poissons_ratio") = _poissons_ratio;
      params.set<Real>("youngs_modulus") = _youngs_modulus;
      params.set<std::vector<VariableName> >("disp_x") = std::vector<VariableName>(1,_disp_x);
      params.set<std::vector<VariableName> >("disp_y") = std::vector<VariableName>(1,_disp_y);
      if (_disp_z !="")
        params.set<std::vector<VariableName> >("disp_z") = std::vector<VariableName>(1,_disp_z);
      params.set<std::string>("aux_stress") = "aux_stress_III";
      params.set<std::string>("aux_disp") = "aux_disp_III";
      params.set<std::string>("aux_grad_disp") = "aux_grad_disp_III";
      params.set<std::string>("aux_strain") = "aux_strain_III";
      params.set<Real>("K_factor") = 0.5 * _youngs_modulus / (2 * (1 + _poissons_ratio));
      for (unsigned int ring_index=0; ring_index<_radius_inner.size(); ++ring_index)
      {
        if (_treat_as_2d)
        {
          std::ostringstream av_name_stream;
          av_name_stream<<av_base_name<<"_"<<ring_index+1;
          std::ostringstream pp_name_stream;
          pp_name_stream<<pp_base_name<<"_"<<"KIII"<<"_"<<ring_index+1;
          std::vector<VariableName> qvars;
          qvars.push_back(av_name_stream.str());
          params.set<std::vector<VariableName> >("q") = qvars;
          _problem->addPostprocessor(pp_type_name,pp_name_stream.str(),params);
        }
        else
        {
          for (unsigned int cfn_index=0; cfn_index<num_crack_front_nodes; ++cfn_index)
          {
            std::ostringstream av_name_stream;
            av_name_stream<<av_base_name<<"_"<<cfn_index+1<<"_"<<ring_index+1;
            std::ostringstream pp_name_stream;
            pp_name_stream<<pp_base_name<<"_"<<"KIII"<<"_"<<cfn_index+1<<"_"<<ring_index+1;
            std::vector<VariableName> qvars;
            qvars.push_back(av_name_stream.str());
            params.set<std::vector<VariableName> >("q") = qvars;
            params.set<unsigned int>("crack_front_node_index") = cfn_index;
            _problem->addPostprocessor(pp_type_name,pp_name_stream.str(),params);
          }
        }
      }
    }
    for (unsigned int i=0; i<_output_variables.size(); ++i)
    {
      const std::string ov_base_name(_output_variables[i]);
      const std::string pp_type_name("CrackFrontData");
      InputParameters params = _factory.getValidParams(pp_type_name);
      params.set<std::vector<MooseEnum> >("execute_on")[0] = "timestep";
      params.set<UserObjectName>("crack_front_definition") = uo_name;
      if (_treat_as_2d)
      {
        std::ostringstream pp_name_stream;
        pp_name_stream<<ov_base_name<<"_crack";
        params.set<VariableName>("variable") = _output_variables[i];
        _problem->addPostprocessor(pp_type_name,pp_name_stream.str(),params);
      }
      else
      {
        for (unsigned int cfn_index=0; cfn_index<num_crack_front_nodes; ++cfn_index)
        {
          std::ostringstream pp_name_stream;
          pp_name_stream<<ov_base_name<<"_crack_"<<cfn_index+1;
          params.set<VariableName>("variable") = _output_variables[i];
          params.set<unsigned int>("crack_front_node_index") = cfn_index;
          _problem->addPostprocessor(pp_type_name,pp_name_stream.str(),params);
        }
      }
    }
  }
  else if (_current_task == "add_material")
  {

    int n_int_integrals(0);
    int i_ki;
    int i_kii;
    int i_kiii;

    if (_integrals.count(INTERACTION_INTEGRAL_KI)  != 0)
    {
      i_ki = n_int_integrals;
      n_int_integrals++;
    }
    if (_integrals.count(INTERACTION_INTEGRAL_KII)  != 0)
    {
      i_kii = n_int_integrals;
      n_int_integrals++;
    }
    if (_integrals.count(INTERACTION_INTEGRAL_KIII)  != 0)
    {
      i_kiii = n_int_integrals;
      n_int_integrals++;
    }

    std::vector<MooseEnum> sif_mode_enum_vec(InteractionIntegralAuxFields::getSIFModesVec(n_int_integrals));

    if (_integrals.count(INTERACTION_INTEGRAL_KI)  != 0)
      sif_mode_enum_vec[i_ki] = "KI";
    if (_integrals.count(INTERACTION_INTEGRAL_KII)  != 0)
      sif_mode_enum_vec[i_kii] = "KII";
    if (_integrals.count(INTERACTION_INTEGRAL_KIII)  != 0)
      sif_mode_enum_vec[i_kiii] = "KIII";

    if (sif_mode_enum_vec.size() > 0)
    {
      const std::string mater_name("interactionIntegralAuxFields");
      const std::string mater_type_name("InteractionIntegralAuxFields");

      InputParameters params = _factory.getValidParams(mater_type_name);
      params.set<UserObjectName>("crack_front_definition") = uo_name;
      params.set<bool>("use_displaced_mesh") = _use_displaced_mesh;
      params.set<std::vector<MooseEnum> >("sif_modes") = sif_mode_enum_vec;
      params.set<Real>("poissons_ratio") = _poissons_ratio;
      params.set<Real>("youngs_modulus") = _youngs_modulus;
      params.set<std::vector<SubdomainName> >("block") = _blocks;

      _problem->addMaterial(mater_type_name,mater_name,params);
    }
  }
}


unsigned int
DomainIntegralAction::calcNumCrackFrontNodes()
{
  std::vector<BoundaryID> bids = _mesh->getBoundaryIDs(_boundary_names,true);
  std::set<unsigned int> nodes;

  ConstBndNodeRange & bnd_nodes = *_mesh->getBoundaryNodeRange();
  for (ConstBndNodeRange::const_iterator nd = bnd_nodes.begin() ; nd != bnd_nodes.end(); ++nd)
  {
    const BndNode * bnode = *nd;
    BoundaryID boundary_id = bnode->_bnd_id;

    for (unsigned int ibid=0; ibid<bids.size(); ++ibid)
    {
      if (boundary_id == bids[ibid])
      {
        nodes.insert(bnode->_node->id());
        break;
      }
    }
  }
  return nodes.size();
}
