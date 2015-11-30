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
        stringmap.h
        This file is part of riss.

        16.10.2009
        Copyright 2009 Norbert Manthey
*/

#ifndef _STRINGMAP_H
#define _STRINGMAP_H

#include <iostream>
#include <vector>
#include <cstdlib>
#include <assert.h>
#include <string.h>
#include <string>
#include <sstream>

// for int types
#include <inttypes.h>

using namespace std;

/** Stores pairs of key string and value string
*
* Provides some basic operations on these pairs
*
*/
class StringMap {
 public:
  struct stringmapentry {
    string key;
    string value;

    stringmapentry() : key(""), value("") {}
  };

 private:
  vector<stringmapentry> stringmap;

 public:
  StringMap();
  ~StringMap();

  stringmapentry operator[](uint32_t ind) const { return stringmap[ind]; }

  void insert(const char* key, const char* value);
  bool contains(const char* key) const;
  void clear();
  string get(const char* key) const;
  string get_key(uint32_t ind) const;
  string get_value(uint32_t ind) const;
  // removes entry of key. indicate success by return value.
  bool remove(const char* key);

  uint32_t size() const;
  string toString() const;

  // to write less code for setting up parameters, shows also information about
  // parameters
  /// value of data is only changed, if there is something in the parameters
  void updateParameter(const string& name, int32_t& data,
                       const string& description, int32_t lRange,
                       int32_t hRange, bool show = true) const;
  void updateParameter(const string& name, uint32_t& data,
                       const string& description, uint32_t lRange,
                       uint32_t hRange, bool show = true) const;
  /// value of data is only changed, if there is something in the parameters
  void updateParameter(const string& name, float& data,
                       const string& description, float lRange, float hRange,
                       bool show = true) const;
  void updateParameter(const string& name, double& data,
                       const string& description, double lRange, double hRange,
                       bool show = true) const;
  /// value of data is only changed, if there is something in the parameters
  void updateParameter(const string& name, bool& data,
                       const string& description, bool show = true) const;
  /// value of data is only changed, if there is something in the parameters
  void updateParameter(const string& name, string& data,
                       const string& description, bool show = true) const;

  /// show parameter instead of also returning value
  void showParameter(const string& name, const string& description,
                     double lRange, double hRange) const;
  void showParameter(const string& name, const string& description) const;
};

/*****

        developer section

****/

inline StringMap::StringMap() {}

inline StringMap::~StringMap() {}

// copies the memory of key and value string
inline void StringMap::insert(const char* key, const char* value) {
  const uint32_t mapsize = stringmap.size();
  for (uint32_t i = 0; i < mapsize; i++) {
    if (stringmap[i].key.compare(key) == 0)  // found key -> overwrite value
    {
      if (value != 0)  // if there is a value needed
      {
        stringmap[i].value = value;
      } else {
        stringmap[i].value = "";
      }
      return;
    }
  }
  // not found -> create new entry
  stringmapentry entry;
  entry.key = string(key);
  if (value != 0)  // if there is a value needed
  {
    entry.value = value;
  } else {
    entry.value = "";
  }
  // add to list
  stringmap.push_back(entry);
}

// returns, whether there is an entry with the given key
inline bool StringMap::contains(const char* key) const {
  const uint32_t mapsize = stringmap.size();
  for (uint32_t i = 0; i < mapsize; i++) {
    if (stringmap[i].key.compare(key) == 0) return true;
  }
  return false;
}

// returns the value to the given key
inline string StringMap::get(const char* key) const {
  const uint32_t size = stringmap.size();
  for (uint32_t i = 0; i < size; i++) {
    if (stringmap[i].key.compare(key) == 0) {
      if (stringmap[i].value.size() == 0) return string();
      return stringmap[i].value;
    }
  }
  return string();
}

inline string StringMap::get_key(uint32_t ind) const {
  assert(size() > ind);
  return stringmap[ind].key;
}

inline string StringMap::get_value(uint32_t ind) const {
  assert(size() > ind);
  return stringmap[ind].value;
}

inline void StringMap::clear() { stringmap.clear(); }

inline bool StringMap::remove(const char* key) {
  for (uint32_t i = 0; i < stringmap.size(); ++i) {
    if (stringmap[i].key.compare(key) == 0) {
      stringmap.erase(stringmap.begin() + i);
      return true;
    }
  }
  return false;
}

inline uint32_t StringMap::size() const { return stringmap.size(); }

inline string StringMap::toString() const {
  stringstream stream;
  const uint32_t size = stringmap.size();
  for (uint32_t i = 0; i < size; i++) {
    if (stringmap[i].key.compare("file") == 0) continue;
    stream << stringmap[i].key;
    if (stringmap[i].value.size() == 0) {
      stream << "|";
    } else {
      stream << " " << stringmap[i].value << "|";
    }
  }
  // move file key to the end!
  if (contains("file")) {
    stream << "file " << get("file") << "|";
  }
  return stream.str();
}

inline void StringMap::showParameter(const string& name,
                                     const string& description, double lRange,
                                     double hRange) const {

  if (contains("--help") && !contains("--verb-help")) {
    cerr << name << "\t[?]\twith {" << lRange << "-" << hRange << "}" << endl;
  } else if (contains("--verb-help")) {
    cerr << name << "\t[?]\twith {" << lRange << "-" << hRange << "}" << endl;
    cerr << "\t\t" << description << endl;
  }
}

inline void StringMap::showParameter(const string& name,
                                     const string& description) const {

  if (contains("--help") && !contains("--verb-help")) {
    cerr << name << "\t[?]" << endl;
  } else if (contains("--verb-help")) {
    cerr << name << "\t[?]" << endl;
    cerr << "\t\t" << description << endl;
  }
}

inline void StringMap::updateParameter(const string& name, int32_t& data,
                                       const string& description,
                                       int32_t lRange, int32_t hRange,
                                       bool show) const {
  if (contains(name.c_str())) data = atoi(get(name.c_str()).c_str());

  if (show) {
    if (contains("--help") && !contains("--verb-help")) {
      cerr << name << "\t[" << data << "]\twith {" << lRange << "-" << hRange
           << "}" << endl;
    } else if (contains("--verb-help")) {
      cerr << name << "\t[" << data << "]\twith {" << lRange << "-" << hRange
           << "}" << endl;
      cerr << "\t\t" << description << endl;
    }
  }
}

inline void StringMap::updateParameter(const string& name, uint32_t& data,
                                       const string& description,
                                       uint32_t lRange, uint32_t hRange,
                                       bool show) const {
  if (contains(name.c_str())) data = atoi(get(name.c_str()).c_str());

  if (show) {
    if (contains("--help") && !contains("--verb-help")) {
      cerr << name << "\t[" << data << "]\twith {" << lRange << "-" << hRange
           << "}" << endl;
    } else if (contains("--verb-help")) {
      cerr << name << "\t[" << data << "]\twith {" << lRange << "-" << hRange
           << "}" << endl;
      cerr << "\t\t" << description << endl;
    }
  }
}

inline void StringMap::updateParameter(const string& name, float& data,
                                       const string& description, float lRange,
                                       float hRange, bool show) const {
  if (contains(name.c_str())) data = atof(get(name.c_str()).c_str());

  if (show) {
    if (contains("--help") && !contains("--verb-help")) {
      cerr << name << "\t[" << data << "]\twith {" << lRange << "-" << hRange
           << "}" << endl;
    } else if (contains("--verb-help")) {
      cerr << name << "\t[" << data << "]\twith {" << lRange << "-" << hRange
           << "}" << endl;
      cerr << "\t\t" << description << endl;
    }
  }
}

inline void StringMap::updateParameter(const string& name, double& data,
                                       const string& description, double lRange,
                                       double hRange, bool show) const {
  if (contains(name.c_str())) data = atof(get(name.c_str()).c_str());

  if (show) {
    if (contains("--help") && !contains("--verb-help")) {
      cerr << name << "\t[" << data << "]\twith {" << lRange << "-" << hRange
           << "}" << endl;
    } else if (contains("--verb-help")) {
      cerr << name << "\t[" << data << "]\twith {" << lRange << "-" << hRange
           << "}" << endl;
      cerr << "\t\t" << description << endl;
    }
  }
}

inline void StringMap::updateParameter(const string& name, bool& data,
                                       const string& description,
                                       bool show) const {
  if (contains(name.c_str())) data = 0 != atoi(get(name.c_str()).c_str());

  if (show) {
    if (contains("--help") && !contains("--verb-help")) {
      cerr << name << "\t[" << data << "]\twith {0,1}" << endl;
    } else if (contains("--verb-help")) {
      cerr << name << "\t[" << data << "]\twith {0,1}" << endl;
      cerr << "\t\t" << description << endl;
    }
  }
}

inline void StringMap::updateParameter(const string& name, string& data,
                                       const string& description,
                                       bool show) const {
  if (contains(name.c_str())) data = get(name.c_str());

  if (show) {
    if (contains("--help") && !contains("--verb-help")) {
      cerr << name << "\t[" << data << "]\tas string" << endl;
    } else if (contains("--verb-help")) {
      cerr << name << "\t[" << data << "]\tas string" << endl;
      cerr << "\t\t" << description << endl;
    }
  }
}

#endif
