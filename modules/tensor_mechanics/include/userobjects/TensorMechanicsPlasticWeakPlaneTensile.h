#ifndef TENSORMECHANICSPLASTICWEAKPLANETENSILE_H
#define TENSORMECHANICSPLASTICWEAKPLANETENSILE_H

#include "TensorMechanicsPlasticModel.h"


class TensorMechanicsPlasticWeakPlaneTensile;


template<>
InputParameters validParams<TensorMechanicsPlasticWeakPlaneTensile>();

/**
 * Rate-independent associative weak-plane tensile failure
 * with hardening/softening
 */
class TensorMechanicsPlasticWeakPlaneTensile : public TensorMechanicsPlasticModel
{
 public:
  TensorMechanicsPlasticWeakPlaneTensile(const std::string & name, InputParameters parameters);

  /**
   * The yield function
   * @param stress the stress at which to calculate the yield function
   * @param intnl internal parameter
   * @return the yield function
   */
  Real yieldFunction(const RankTwoTensor & stress, const Real & intnl) const;

  /**
   * The derivative of yield function with respect to stress
   * @param stress the stress at which to calculate the yield function
   * @param intnl internal parameter
   * @return df_dstress(i, j) = dyieldFunction/dstress(i, j)
   */
  RankTwoTensor dyieldFunction_dstress(const RankTwoTensor & stress, const Real & intnl) const;

  /**
   * The derivative of yield function with respect to the internal parameter
   * @param stress the stress at which to calculate the yield function
   * @param intnl internal parameter
   * @return the derivative
   */
  Real dyieldFunction_dintnl(const RankTwoTensor & stress, const Real & intnl) const;

  /**
   * The flow potential
   * @param stress the stress at which to calculate the flow potential
   * @param intnl internal parameter
   * @return the flow potential
   */
  RankTwoTensor flowPotential(const RankTwoTensor & stress, const Real & intnl) const;

  /**
   * The derivative of the flow potential with respect to stress
   * @param stress the stress at which to calculate the flow potential
   * @param intnl internal parameter
   * @return dr_dstress(i, j, k, l) = dr(i, j)/dstress(k, l)
   */
  RankFourTensor dflowPotential_dstress(const RankTwoTensor & stress, const Real & intnl) const;

  /**
   * The derivative of the flow potential with respect to the internal parameter
   * @param stress the stress at which to calculate the flow potential
   * @param intnl internal parameter
   * @return dr_dintnl(i, j) = dr(i, j)/dintnl
   */
  RankTwoTensor dflowPotential_dintnl(const RankTwoTensor & stress, const Real & intnl) const;

 protected:

  /// tension cutoff
  Real _tension_cutoff;

  /// tension cutoff at infinite hardening/softening
  Real _tension_cutoff_residual;

  /// Tensile strength = wpt_tensile_strenght_residual + (wpt_tensile_strength - wpt_tensile_strength_residual)*exp(-wpt_tensile_rate*plasticstrain).
  Real _tension_cutoff_rate;

  /// tensile strength as a function of residual value, rate, and internal_param
  virtual Real tensile_strength(const Real internal_param) const;

  /// d(tensile strength)/d(internal_param) as a function of residual value, rate, and internal_param
  virtual Real dtensile_strength(const Real internal_param) const;
};

#endif // TENSORMECHANICSPLASTICWEAKPLANETENSILE_H
