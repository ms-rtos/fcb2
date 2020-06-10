/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef OS_MUTEX_H
#define OS_MUTEX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "syscfg/syscfg.h"

/**
 * OS mutex structure
 */
struct os_mutex {
    ms_handle_t   id;
};

/**
 * Create a mutex and initialize it.
 *
 * @param mu Pointer to mutex
 *
 * @return os_error_t
 *      OS_INVALID_PARM     Mutex passed in was NULL.
 *      OS_OK               no error.
 */
os_error_t os_mutex_init(struct os_mutex *mu);

/**
 * Release a mutex.
 *
 * @param mu Pointer to the mutex to be released
 *
 * @return os_error_t
 *      OS_INVALID_PARM Mutex passed in was NULL.
 *      OS_BAD_MUTEX    Mutex was not granted to current task (not owner).
 *      OS_OK           No error
 */
os_error_t os_mutex_release(struct os_mutex *mu);

/**
 * Pend (wait) for a mutex.
 *
 * @param mu Pointer to mutex.
 * @param timeout Timeout, in os ticks.
 *                A timeout of 0 means do not wait if not available.
 *                A timeout of OS_TIMEOUT_NEVER means wait forever.
 *
 *
 * @return os_error_t
 *      OS_INVALID_PARM     Mutex passed in was NULL.
 *      OS_TIMEOUT          Mutex was owned by another task and timeout=0
 *      OS_OK               no error.
 */
os_error_t os_mutex_pend(struct os_mutex *mu, os_time_t timeout);

#ifdef __cplusplus
}
#endif

#endif  /* OS_MUTEX_H */
