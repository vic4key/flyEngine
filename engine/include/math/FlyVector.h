#ifndef FLYVECTOR_H
#define FLYVECTOR_H

#include <glm/glm.hpp>
#include <initializer_list>
#include <cstring>
#include <ostream>

namespace fly
{
  template<unsigned Dim, typename T>
  class Vector
  {
  public:
    /**
    * Constructors
    */
    Vector() = default;
    Vector(const T& value)
    {
      for (unsigned i = 0; i < Dim; i++) {
        _data[i] = value;
      }
    }
    template<typename ...Args>
    Vector(Args... args) : _data{args...}
    {
    }
    /**
    * Copy constructors
    */
    Vector(const Vector& other) = default;
    template<typename T1>
    Vector(const Vector<Dim, T1>& other)
    {
      for (unsigned i = 0; i < Dim; i++) {
        _data[i] = static_cast<T>(other[i]);
      }
    }
    Vector(const Vector<Dim - 1, T>& other, T val)
    {
      for (unsigned i = 0; i < Dim - 1; i++) {
        _data[i] = other[i];
      }
      _data[Dim - 1] = val;
    }
    /**
    * Destructor
    */
    ~Vector() = default;
    /**
    * Member access
    */
    inline T& operator [] (unsigned index)
    {
      return _data[index];
    }
    inline const T& operator [] (unsigned index) const
    {
      return _data[index];
    }
    inline const T* ptr() const
    {
      return _data;
    }
    inline Vector<3, T> xyz() const
    {
      static_assert(Dim >= 3, "Vector dimension too small");
      return Vector<3, T>(_data[0], _data[1], _data[2]);
    }
    /**
    * Vector/vector calculations
    */
    inline Vector operator + (const Vector& b) const
    {
      Vector result;
      for (unsigned i = 0; i < Dim; i++) {
        result[i] = _data[i] + b[i];
      }
      return result;
    }
    inline Vector operator - (const Vector& b) const
    {
      Vector result;
      for (unsigned i = 0; i < Dim; i++) {
        result[i] = _data[i] - b[i];
      }
      return result;
    }
    inline Vector operator * (const Vector& b) const
    {
      Vector result;
      for (unsigned i = 0; i < Dim; i++) {
        result[i] = _data[i] * b[i];
      }
      return result;
    }
    inline Vector operator / (const Vector& b) const
    {
      Vector result;
      for (unsigned i = 0; i < Dim; i++) {
        result[i] = _data[i] / b[i];
      }
      return result;
    }
    inline Vector& operator += (const Vector& b)
    {
      for (unsigned i = 0; i < Dim; i++) {
        _data[i] += b[i];
      }
      return *this;
    }
    inline Vector& operator -= (const Vector& b)
    {
      for (unsigned i = 0; i < Dim; i++) {
        _data[i] -= b[i];
      }
      return *this;
    }
    inline Vector& operator *= (const Vector& b)
    {
      for (unsigned i = 0; i < Dim; i++) {
        _data[i] *= b[i];
      }
      return *this;
    }
    inline Vector& operator /= (const Vector& b)
    {
      for (unsigned i = 0; i < Dim; i++) {
        _data[i] /= b[i];
      }
      return *this;
    }
    /**
    * Comparison operators
    */
    inline bool operator < (const Vector& b) 
    {
      for (unsigned i = 0; i < Dim; i++) {
        if (!(_data[i] < b[i])) {
          return false;
        }
      }
      return true;
    }
    inline bool operator > (const Vector& b)
    {
      for (unsigned i = 0; i < Dim; i++) {
        if (!(_data[i] > b[i])) {
          return false;
        }
      }
      return true;
    }
    inline bool operator <= (const Vector& b)
    {
      for (unsigned i = 0; i < Dim; i++) {
        if (!(_data[i] <= b[i])) {
          return false;
        }
      }
      return true;
    }
    inline bool operator >= (const Vector& b)
    {
      for (unsigned i = 0; i < Dim; i++) {
        if (!(_data[i] >= b[i])) {
          return false;
        }
      }
      return true;
    }
    /**
    * Vector length
    */
    inline T length() const
    {
      return std::sqrt(dot(*this, *this));
    }
    /**
    * Conversion from/to glm
    */
    inline Vector(const glm::vec<Dim, T>& vec)
    {
      std::memcpy(_data, &vec[0], sizeof vec);
    }
    inline operator glm::vec<Dim, T>() const
    {
      glm::vec<Dim, T> vec;
      std::memcpy(&vec[0], _data, sizeof vec);
      return vec;
    }
    /**
    * Change in dimension
    */
    template<unsigned N>
    inline operator Vector<N, T>() const
    {
      Vector<N, T> ret;
      std::memcpy(&ret[0], _data, std::min(sizeof ret, sizeof *this));
      return ret;
    }
    /**
    * Debug output
    */
    friend std::ostream& operator << (std::ostream& os, const Vector& vec)
    {
      os << "[";
      for (unsigned i = 0; i < Dim; i++) {
        os << vec._data[i];
        if (i != Dim - 1) {
          os << " ";
        }
      }
      os << "]";
      return os;
    }
  private:
   T _data [Dim];
  };
}

#endif
