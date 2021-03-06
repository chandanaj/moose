[Mesh]
  type = GeneratedMesh
  dim = 2
  nx = 10
  ny = 10
  xmax = 0.2
  ymax = 0.2
  distribution = serial
[]

[Variables]
  [./sub_u]
  [../]
[]

[Kernels]
  [./diff]
    type = Diffusion
    variable = sub_u
  [../]
[]

[BCs]
  [./left]
    type = DirichletBC
    variable = sub_u
    boundary = left
    value = 1
  [../]
  [./right]
    type = DirichletBC
    variable = sub_u
    boundary = right
    value = 4
  [../]
[]

[Executioner]
  type = Transient
  num_steps = 1
  dt = 1

  # Preconditioned JFNK (default)
  solve_type = 'PJFNK'

  petsc_options_iname = '-pc_type -pc_hypre_type'
  petsc_options_value = 'hypre boomeramg'
[]

[Outputs]
  output_initial = true
  exodus = true
  [./console]
    type = Console
    perf_log = true
    linear_residuals = true
  [../]
[]
