// ---------------------------------------------------------------------------
// Array and tuple classes (bare bones version).
//
// MASC uses the C++11 array and tuple classes. Not all commercial EDA tools
// support C++11, unfortunately. Synopsys Hector does not, nor does Cadence
// C2S, and I suspect most other high level synthesis tools don't either.
//
// The classes defined below are sufficient for MASC and also work with
// Hector and C2S. They are intended to be compatible with C++11.
// ---------------------------------------------------------------------------

#ifndef MASC_H
#define MASC_H

typedef unsigned int uint;

#if !defined(__CPROVER__)
#include <iostream>
#endif

// ---------------------------------------------------------------------------
// Templates for passing and returning arrays
// ---------------------------------------------------------------------------

template <class T, uint m>
class array {

  T elt[m];

public:

  T& operator[] (int idx) {
    return elt[idx];
  }

#if !defined(__CPROVER__)
  std::ostream&
  dump (std::ostream& os) {
    os << "{";
    for (int i = 0; i < m; i++) {
      if (0 < i)
        os << ", ";
      os << elt[i];
    }
    os << "}";
    return os;
  }
#endif

};

template<class T, uint m>
  std::ostream& operator<<(std::ostream& os, array<T,m> src)
{
  return src.dump(os);
}

// ---------------------------------------------------------------------------
// Templates for passing & returning tuples
// ---------------------------------------------------------------------------

// null type
struct null_type
{
  bool operator==(const null_type &other) const { return true; }
};

// a const value of null_type
inline const null_type cnull() {return null_type();}

#if !defined(__CPROVER__)
std::ostream& operator<<(std::ostream& os, const null_type dummy)
{
  os << "-";
  return os;
}
#endif

// a global to absorb "writes" to unused tuple fields.
// would be good to get rid of this.
null_type dummy;

template 
   <class T0 = null_type, 
    class T1 = null_type,
    class T2 = null_type,
    class T3 = null_type>
class ltuple;

template 
   <class T0 = null_type,
    class T1 = null_type,
    class T2 = null_type,
    class T3 = null_type>
class tuple
{

  T0 el0;
  T1 el1;
  T2 el2;
  T3 el3;

public:

  friend tuple<T0,T1,T2,T3>
  ltuple<T0,T1,T2,T3>::operator= (tuple<T0,T1,T2,T3>);

#if !defined(__CPROVER__)
  std::ostream&
  dump (std::ostream& os) {
    os << "(" << el0 << "," << el1 << "," << el2 << "," << el3 << ")";
    return os;
  }
#endif

  tuple() {}

  tuple(T0 t0): el0(t0), el1(cnull()), el2(cnull()), el3(cnull()) {}

  tuple(T0 t0, T1 t1): el0(t0), el1(t1), el2(cnull()), el3(cnull()) {}

  tuple(T0 t0, T1 t1, T2 t2): el0(t0), el1(t1), el2(t2), el3(cnull()) {}

  tuple(T0 t0, T1 t1, T2 t2, T3 t3): el0(t0), el1(t1), el2(t2), el3(t3) {}

  bool operator==(const tuple<T0,T1,T2,T3> &other) const
  {
    return
      el0 == other.el0 &&
      el1 == other.el1 &&
      el2 == other.el2 &&
      el3 == other.el3;           
  }

  
};

#if !defined(__CPROVER__)
template<class T0, class T1, class T2, class T3>
  std::ostream& operator<<(std::ostream& os, tuple<T0,T1,T2,T3> src)
{
  return src.dump(os);
}
#endif

template 
   <class T0, 
    class T1,
    class T2,
    class T3>
class ltuple
{

private:
  T0 &el0;
  T1 &el1;
  T2 &el2;
  T3 &el3;

public:

  ltuple(T0 &t0, T1 &t1, T2 &t2, T3 &t3 )
    : el0(t0), el1(t1), el2(t2), el3(t3)
  {}

  tuple<T0,T1,T2,T3>
  operator= (tuple<T0,T1,T2,T3>);
};

template <class T0, class T1, class T2, class T3>
tuple<T0,T1,T2,T3>
ltuple<T0,T1,T2,T3>::operator= (tuple<T0,T1,T2,T3> src) {
  el0 = src.el0;
  el1 = src.el1;
  el2 = src.el2;
  el3 = src.el3;
  return src;
}


template <class T0>
ltuple<T0>
tie(T0 &t0)
{
  return ltuple<T0>(t0, dummy, dummy, dummy);
}

template <class T0, class T1>
ltuple<T0,T1>
tie(T0 &t0, T1 &t1)
{
  return ltuple<T0,T1>(t0,t1,dummy,dummy);
}

template <class T0, class T1, class T2>
ltuple<T0,T1,T2>
tie(T0 &t0, T1 &t1, T2 &t2)
{
  return ltuple<T0,T1,T2>(t0,t1,t2,dummy);
}

template <class T0, class T1, class T2, class T3>
ltuple<T0,T1,T2,T3>
tie(T0 &t0, T1 &t1, T2 &t2, T3 &t3)
{
  return ltuple<T0,T1,T2,T3>(t0,t1,t2,t3);
}
 
#endif

