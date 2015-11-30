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
        spinlock.h
        This file is part of riss.

        24.06.2011
        Copyright 2011 Norbert Manthey
*/

#ifndef __SPINLOCK_H
#define __SPINLOCK_H

#include <queue>

#include <pthread.h>
#include <semaphore.h>

#include <iostream>

/**
 * 0 == pthread_spin_init(pthread_spinlock_t *lock, PTHREAD_PROCESS_PRIVATE);
 * 0 == pthread_spin_destroy(pthread_spinlock_t *lock);
 *
 * 0 == spin_trylock(spinlock_t *lock) -> success
 * void spin_lock(spinlock_t *lock);
 * void spin_unlock(spinlock_t *lock);
 * */

class Spinlock {

 private:
#ifdef USE_SPINLOCKS
  pthread_spinlock_t _lock;  // actual semaphore
#else
  pthread_mutex_t _lock;
#endif

  int _max;  // users for the semaphore
 public:
  /** create an unlocked lock
  * @param max specify number of maximal threads that have entered the semaphore
  */
  Spinlock() : _max(1) {
// create semaphore with no space in it
#ifdef USE_SPINLOCKS
    pthread_spin_init(&_lock, PTHREAD_PROCESS_PRIVATE);
#else
    pthread_mutex_init(&_lock, NULL);
#endif
  }

  /** release all used resources
  */
  ~Spinlock() {
#ifdef USE_SPINLOCK
    pthread_spin_destroy(&_lock);
#else
    pthread_mutex_destroy(&_lock);
#endif
  }

  /** tries to lock
  * @return true, if locking was successful
  */
  bool lock() {
#ifdef USE_SPINLOCKS
    int err = pthread_spin_trylock(&_lock);
#else
    int err = pthread_mutex_trylock(&_lock);
#endif
    return err == 0;
  }
  //@param transitive allow multiple locking of the same thread ?

  /** releases the lock again
  *
  * should only be called by the thread that is currently owns the lock
  */
  void unlock() {
#ifdef USE_SPINLOCKS
    pthread_spin_unlock(&_lock);
#else
    pthread_mutex_unlock(&_lock);
#endif
  }

  /** waits(busy) until the lock is given to the calling thread
  */
  void wait() {
    // perform busy waiting!!
    while (!lock()) {
    };
  }
};

#endif
