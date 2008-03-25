/*
 * Copyright (C) 2002-2006 Intel Corporation
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 */

#ifndef _BITS_H_
#define _BITS_H_

#include <stdio.h>

// ****************************** Concatination *******************************
class Cat;

class CatRef {
private:
  int    size;
  UINT64 *ref;
public:
  CatRef(UINT64 *r, int sz)  { ref = r; size = sz; }

  friend class Cat;
};

class Cat {
private:
  CatRef a;
  CatRef b;
public:
  Cat(CatRef aa, CatRef bb) : a(aa), b(bb) {}

  inline operator INT64()          { return ((*a.ref<<b.size) | (*b.ref));}
  inline INT64 operator=(INT64 v)  {
    UINT64 mask = (UINT64_MAX>>(64-b.size));
    *b.ref = v&mask;
    mask = (UINT64_MAX>>(64-a.size));
    *a.ref = (v>>b.size)&mask;
    return ((*a.ref<<b.size) | (*b.ref));
  }
};
  
// *************************** Unsigned Data Types ****************************
template<int n> class Bits;

template<int n> class BitsRef {
private:
  int    lsb;
  UINT64 mask;
  UINT64 *ref;
  BitsRef(Bits<n> *b, int m, int l) { if ((m>=n) || (l<0))
					printf("Bounds error calling Bits<%d>\n", n);
                                      ref  = &b->val;
				      mask = (UINT64_MAX>>(63-m+l));
				      lsb  = l;}
public: 
  inline operator INT64()              { return (*ref>>lsb)&mask;}
  inline void operator=(BitsRef<n> r)  {
    *ref = ( (*ref & (~(mask<<lsb))) | (((INT64)r&mask)<<lsb) );
  }
  void operator=(INT64 v)       {
    *ref = ( (*ref & (~(mask<<lsb))) | ((      v&mask)<<lsb) );
  }

  friend class Bits<n>;
};

template<int n> class Bits {
private:
  UINT64 val;
public:
  Bits<n>(INT64 v=0)                         {val = ((UINT64)(v)) << (64-n) >> (64-n);}
  inline int Size(void)                      {return n;}
  inline operator INT64()                    {return val;}
  inline operator CatRef()                   {return CatRef(&val, n);}
  inline BitsRef<n> operator[](int i)        {return BitsRef<n>(this, i, i);}
  inline BitsRef<n> operator()(int m, int l) {return BitsRef<n>(this, m, l);}
  inline INT64 operator=(INT64 v)            {return val = ((UINT64) (v)     ) << (64-n) >> (64-n);}
  inline INT64 operator|=(INT64 v)           {return val = ((UINT64) (val|v) ) << (64-n) >> (64-n);}
  inline INT64 operator&=(INT64 v)           {return val = ((UINT64) (val&v) ) << (64-n) >> (64-n);}
  inline INT64 operator^=(INT64 v)           {return val = ((UINT64) (val^v) ) << (64-n) >> (64-n);}
  inline INT64 operator%=(INT64 v)           {return val = ((UINT64) (val%v) ) << (64-n) >> (64-n);}
  inline INT64 operator*=(INT64 v)           {return val = ((UINT64) (val*v) ) << (64-n) >> (64-n);}
  inline INT64 operator/=(INT64 v)           {return val = ((UINT64) (val/v) ) << (64-n) >> (64-n);}
  inline INT64 operator+=(INT64 v)           {return val = ((UINT64) (val+v) ) << (64-n) >> (64-n);}
  inline INT64 operator-=(INT64 v)           {return val = ((UINT64) (val-v) ) << (64-n) >> (64-n);}
  inline INT64 operator<<=(INT64 v)          {return val = ((UINT64) (val<<v)) << (64-n) >> (64-n);}
  inline INT64 operator>>=(INT64 v)          {return val = ((UINT64) (val>>v)) << (64-n) >> (64-n);}
  inline INT64 operator++()                  {return val = ((UINT64) (val+1) ) << (64-n) >> (64-n);}
  inline INT64 operator++(int)               {return val = ((UINT64) (val+1) ) << (64-n) >> (64-n);}
  inline INT64 operator--()                  {return val = ((UINT64) (val-1) ) << (64-n) >> (64-n);}
  inline INT64 operator--(int)               {return val = ((UINT64) (val-1) ) << (64-n) >> (64-n);}

  friend class BitsRef<n>;
};

#endif
