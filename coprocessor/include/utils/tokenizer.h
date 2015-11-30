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
        tokenizer.h
        This file is part of riss.

        10.08.2010
        Copyright 2009 Norbert Manthey
*/

#ifndef _TOKENIZER_H
#define _TOKENIZER_H

// for int types
#include <inttypes.h>
#include <vector>
#include <set>
#include <string>
#include <cstring>

#include <iostream>

#define SEPSIZE 4

using namespace std;

/** Tokenizes strings into its part using specified separators
*/
class Tokenizer {

  // struct to store small strings as token separator
  struct sstr {
    char c[SEPSIZE];
    sstr() {/*for(uint32_t i = 0 ; i < SEPSIZE; ++i ) c[i] = 0;*/
    }
  };

  // data
  bool init;           // already did initial setup for position
  const string& data;  // string to tokenize
  uint32_t b;          // current begin of next token
  uint32_t e;          // current end of next token
  // TODO: sort words and separators by first character -> find binary!
  vector<sstr> separators;  // list of separator
  vector<sstr> words;       // list of single words that has to be separated

 public:
  Tokenizer(const string& str) : init(false), data(str), b(0), e(0) {}

  // words, that also separate tokens and should be recognized
  bool addWord(const string& c) { return addWord(c.c_str()); }
  bool addWord(const char* c) {
    sstr s;
    if (strlen(c) >= SEPSIZE) return false;  // handle only small strings
    strcpy(s.c, c);
    words.push_back(s);
    return true;
  }

  // white spaces and separator for words
  bool addSeparator(const string& c) { return addSeparator(c.c_str()); }
  bool addSeparator(const char* c) {
    sstr s;
    if (strlen(c) >= SEPSIZE) return false;  // handle only small strings
    strcpy(s.c, c);
    separators.push_back(s);
    return true;
  }

  bool hasNext() {
    if (!init) {
      findNext();
      init = true;
    }
    return (b < data.size());
  }

  string nextToken() {
    uint32_t tb = b;
    uint32_t te = e;
    b = e + 1;
    findNext();
    //		cerr << "t: next in [" << b << "," << e << "]" << endl;
    return data.substr(tb, te - tb + 1);
  }

 private:
  void findNext() {
    // setup begin b
    for (; b < data.size(); ++b) {
      uint32_t l = inSeparators(b);
      if (l == 0) {  // not in separators
        l = inWords(
            b);  // check if position is a word, then end can be set accordingly
        if (l > 0) {
          e = b + l - 1;
          return;
        }
        e = b;
        break;
      }
    }
    // setup end
    e = b + 1;
    for (; e < data.size(); ++e) {
      if (inWords(e) > 0 || inSeparators(e) > 0) break;
    }
    e--;
    // no the next token should be between b and e!
  }

  // return number of characters of the word!
  uint32_t inWords(uint32_t pos) {
    for (uint32_t i = 0; i < words.size(); ++i) {
      const uint32_t l = strlen(words[i].c);
      if (pos + l > data.size()) continue;
      if (memcmp(words[i].c, data.c_str() + pos, l) == 0) return l;
    }
    return 0;
  }

  // return number of characters of the separator!
  uint32_t inSeparators(uint32_t pos) {
    for (uint32_t i = 0; i < separators.size(); ++i) {
      const uint32_t l = strlen(separators[i].c);
      if (pos + l > data.size()) continue;
      if (memcmp(separators[i].c, data.c_str() + pos, l) == 0) return l;
    }
    return 0;
  }
};

#endif
