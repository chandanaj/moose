#ifndef DERIVATIVEMATERIALINTERFACE_H
#define DERIVATIVEMATERIALINTERFACE_H

#include "MaterialProperty.h"

/**
 * Interface class ("Veneer") to provide generator methods for derivative
 * material property names, and guarded getMaterialPropertyPointer calls
 */
template<class T>
class DerivativeMaterialInterface : public T
{
public:
  DerivativeMaterialInterface(const std::string & name, InputParameters parameters);

  /**
   * Helper functions to generate the material property names for the
   * first derivatives.
   */
  const std::string propertyNameFirst(const std::string &base,
    const std::string &c1) const;

  /**
   * Helper functions to generate the material property names for the
   * second derivatives.
   */
  const std::string propertyNameSecond(const std::string &base,
    const std::string &c1, const std::string &c2) const;

  /**
   * Helper functions to generate the material property names for the
   * third derivatives.
   */
  const std::string propertyNameThird(const std::string &base,
    const std::string &c1, const std::string &c2, const std::string &c3) const;

  // Interface style (1)
  // return null pointers for non-existing material properties

  /**
   * Fetch a pointer to a material property if it exists, otherwise return null
   */
  template<typename U>
  MaterialProperty<U> * getMaterialPropertyPointer(const std::string & name);

  // Interface style (2)
  // return references to a zero material property for non-existing material properties

  /**
   * Fetch a material property if it exists, otherwise return the 'constant_zero' property
   */
  template<typename U>
  MaterialProperty<U> & getDefaultMaterialProperty(const std::string & name);

  template<typename U>
  MaterialProperty<U> & getDerivative(const std::string &base, const std::string &c1);

  template<typename U>
  MaterialProperty<U> & getDerivative(const std::string &base, const std::string &c1, const std::string &c2);

  template<typename U>
  MaterialProperty<U> & getDerivative(const std::string &base, const std::string &c1, const std::string &c2, const std::string &c3);

private:
  const std::string _constant_zero;
};


template<class T>
DerivativeMaterialInterface<T>::DerivativeMaterialInterface(const std::string & name, InputParameters parameters) :
    T(name, parameters),
    _constant_zero("constant_zero")
{
}

template<class T>
const std::string
DerivativeMaterialInterface<T>::propertyNameFirst(const std::string &base, const std::string &c1) const
{
  return "d" + base + "/d" + c1;
}

template<class T>
const std::string
DerivativeMaterialInterface<T>::propertyNameSecond(const std::string &base, const std::string &c1, const std::string &c2) const
{
  if (c1 == c2)
    return "d^2" + base + "/d" + c1 + "^2";
  else if (c1 < c2)
    return "d^2" + base + "/d" + c1 + "d" + c2;
  else
    return "d^2" + base + "/d" + c2 + "d" + c1;
}

template<class T>
const std::string
DerivativeMaterialInterface<T>::propertyNameThird(const std::string &base, const std::string &c1, const std::string &c2, const std::string &c3) const
{
  // to obtain well defined names we sort alphabetically
  std::vector<std::string> c(3);
  c[0] = c1;
  c[1] = c2;
  c[2] = c3;
  std::sort(c.begin(), c.end());

  std::string ret = "d^3" + base + "/d" + c[0];

  // this generates 'pretty' names with exponents rather than repeat multiplication
  if (c[0] == c[1] && c[1] == c[2])
    return ret + "^3";
  else if (c[0] == c[1])
    return ret + "^2d" + c[2];
  else if (c[1] == c[2])
    return ret + "d" + c[1] + "^2";
  else
    return ret + "d" + c[1] + "d" + c[2];
}

template<class T>
template<typename U>
MaterialProperty<U> *
DerivativeMaterialInterface<T>::getMaterialPropertyPointer(const std::string & name)
{
  return this->template hasMaterialProperty<U>(name) ? &(this->template getMaterialProperty<U>(name)) : NULL;
}

template<class T>
template<typename U>
MaterialProperty<U> &
DerivativeMaterialInterface<T>::getDefaultMaterialProperty(const std::string & name)
{
  if (this->template hasMaterialProperty<U>(name))
    return this->template getMaterialProperty<U>(name);
  else
  {
    if (this->template hasMaterialProperty<U>(_constant_zero))
      return this->template getMaterialProperty<U>(_constant_zero);
    else
      mooseError("In your input file declare a 'GenericConstantMaterial' with property named '" + _constant_zero + "' set to 0.");
  }
}


template<class T>
template<typename U>
MaterialProperty<U> &
DerivativeMaterialInterface<T>::getDerivative(const std::string &base, const std::string &c1)
{
  return getDefaultMaterialProperty<U>(propertyNameFirst(base, c1));
}

template<class T>
template<typename U>
MaterialProperty<U> &
DerivativeMaterialInterface<T>::getDerivative(const std::string &base, const std::string &c1, const std::string &c2)
{
  return getDefaultMaterialProperty<U>(propertyNameSecond(base, c1, c2));
}

template<class T>
template<typename U>
MaterialProperty<U> &
DerivativeMaterialInterface<T>::getDerivative(const std::string &base, const std::string &c1, const std::string &c2, const std::string &c3)
{
  return getDefaultMaterialProperty<U>(propertyNameThird(base, c1, c2, c3));
}

#endif //DERIVATIVEMATERIALINTERFACE_H
