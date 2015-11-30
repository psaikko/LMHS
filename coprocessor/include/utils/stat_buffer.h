/*************************************************************************************************
Copyright (c) 2012, Norbert Manthey

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**************************************************************************************************/
/*
        stat_buffer.h
        This file is part of riss.

        06.04.2011
        Copyright 2011 Norbert Manthey
*/

#ifndef _STAT_BUFFER_H
#define _STAT_BUFFER_H

#include <cassert>
#include <inttypes.h>
#include <cstring>
#include <cstdlib>

#include <iostream>
using namespace std;

/** Buffer that can store float values in and return first and second derivation
 * of the last length elements
*/
class StatisticBuffer {

  float* data;         // last values
  uint32_t lastEntry;  // position of the last entry
  uint32_t _size;      // already inserted elements#
  uint32_t _entries;   // number of calls to add elements

  void fillOrdered(float* dst, const float* src) {
    uint32_t p = (lastEntry + 1 == _size) ? 0 : lastEntry + 1;
    for (uint32_t i = 0; i < _size; i++) {
      dst[i] = src[p];
      p = (p + 1 == _size) ? 0 : p + 1;
    }
  }

 public:
  StatisticBuffer() : data(0), lastEntry(0), _size(0) {}

  StatisticBuffer(uint32_t length)
      : data(new float[length]), lastEntry(0), _size(length) {
    memset(data, 0, sizeof(float) * _size);
    // std::cerr << "c build stat buffer with size " << length << endl;
  }

  bool isInited() const { return data != 0; }

  bool init(uint32_t length) {

    assert(data == 0);
    data = new float[length];
    lastEntry = 0;
    _size = length;
    memset(data, 0, sizeof(float) * _size);

    // 			std::cerr << "c init stat buffer with size " << length <<
    // std::endl;
    return true;
  }

  /** reset the statistic counter
  */
  void reset() {
    lastEntry = 0;
    _entries = 0;
    memset(data, 0, sizeof(float) * _size);
  }

  uint32_t size() const { return _size; }

  uint32_t entries() const { return _entries; }

  void add(const float value) {
    lastEntry = (lastEntry + 1 == _size) ? 0 : lastEntry + 1;
    data[lastEntry] = value;
    _entries++;
  }

  float avg() const {
    float a = 0;
    for (uint32_t i = 0; i < _size; ++i) {
      a += data[i];
    }
    return a / (float)_size;
  }

  float deriv(uint32_t nr) {

    assert(nr < _size);

    float ordered[_size];

    fillOrdered(ordered, data);

    // 			cerr << "c ordered buffer:";
    // 			for( uint32_t i = 0 ; i < _size; ++i ){
    // 				cerr << " " << ordered[i];
    // 			} cerr << endl;
    //
    // 			cerr << "c unordered buffer:";
    // 			for( uint32_t i = 0 ; i < _size; ++i ){
    // 				cerr << " " << data[i];
    // 				if( i == lastEntry ) cerr << "*";
    // 			} cerr << endl;

    for (uint32_t d = 0; d < nr; ++d) {
      for (uint32_t i = _size - 1; i > 0; --i) {
        ordered[i] = ordered[i] - ordered[i - 1];
      }
    }

    // 			cerr << "c buffer for deriv " << nr <<":";
    // 			for( uint32_t i = 0 ; i < _size; ++i ){
    // 				cerr << " " << ordered[i];
    // 			} cerr << endl;

    return ordered[_size - 1];
  }
};

#endif
