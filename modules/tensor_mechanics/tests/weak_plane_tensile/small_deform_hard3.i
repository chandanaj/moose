# Checking evolution tensile strength
# A single element is stretched by 1E-6*t in z direction, and
# the yield-surface evolution is mapped out

[Mesh]
  type = GeneratedMesh
  dim = 3
  nx = 1
  ny = 1
  nz = 1
  xmin = -0.5
  xmax = 0.5
  ymin = -0.5
  ymax = 0.5
  zmin = -0.5
  zmax = 0.5
[]


[Variables]
  [./x_disp]
  [../]
  [./y_disp]
  [../]
  [./z_disp]
  [../]
[]

[TensorMechanics]
  [./solid]
    disp_x = x_disp
    disp_y = y_disp
    disp_z = z_disp
  [../]
[]


[BCs]
  [./bottomx]
    type = PresetBC
    variable = x_disp
    boundary = back
    value = 0.0
  [../]
  [./bottomy]
    type = PresetBC
    variable = y_disp
    boundary = back
    value = 0.0
  [../]
  [./bottomz]
    type = PresetBC
    variable = z_disp
    boundary = back
    value = 0.0
  [../]

  [./topx]
    type = PresetBC
    variable = x_disp
    boundary = front
    value = 0
  [../]
  [./topy]
    type = PresetBC
    variable = y_disp
    boundary = front
    value = 0
  [../]
  [./topz]
    type = FunctionPresetBC
    variable = z_disp
    boundary = front
    function = 1E-6*t
  [../]
[]

[AuxVariables]
  [./wpt_internal]
    order = CONSTANT
    family = MONOMIAL
  [../]
  [./stress_zz]
    order = CONSTANT
    family = MONOMIAL
  [../]
  [./yield_fcn]
    order = CONSTANT
    family = MONOMIAL
  [../]
[]

[AuxKernels]
  [./wpt_internal]
    type = MaterialRealAux
    property = weak_plane_tensile_internal
    variable = wpt_internal
  [../]
  [./stress_zz]
    type = RankTwoAux
    rank_two_tensor = stress
    variable = stress_zz
    index_i = 2
    index_j = 2
  [../]
  [./yield_fcn_auxk]
    type = MaterialRealAux
    property = weak_plane_tensile_yield_function
    variable = yield_fcn
  [../]
[]

[Postprocessors]
  [./wpt_internal]
    type = PointValue
    point = '0 0 0'
    variable = wpt_internal
  [../]
  [./s_zz]
    type = PointValue
    point = '0 0 0'
    variable = stress_zz
  [../]
  [./f]
    type = PointValue
    point = '0 0 0'
    variable = yield_fcn
  [../]
[]

[Materials]
  [./mc]
    type = FiniteStrainWeakPlaneTensile
    block = 0
    disp_x = x_disp
    disp_y = y_disp
    disp_z = z_disp
    wpt_tensile_strength = 10
    wpt_tensile_strength_residual = 4
    wpt_tensile_strength_rate = 1E6
    yield_function_tolerance = 1E-6
    fill_method = symmetric_isotropic
    C_ijkl = '0 1E7'
    wpt_normal_vector = '0 0 1'
    wpt_normal_rotates = false
    ep_plastic_tolerance = 1E-11
    internal_constraint_tolerance = 1E-11
  [../]
[]


[Executioner]
  end_time = 4
  dt = 0.5
  type = Transient
[]


[Outputs]
  file_base = small_deform_hard3
  output_initial = true
  exodus = false
  [./console]
    type = Console
    perf_log = true
    linear_residuals = false
  [../]
  [./csv]
    type = CSV
    interval = 1
  [../]
[]
