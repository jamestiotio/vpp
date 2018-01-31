/*
 * Copyright (c) 2017 Cisco and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef SRC_VNET_SESSION_SEGMENT_MANAGER_H_
#define SRC_VNET_SESSION_SEGMENT_MANAGER_H_

#include <vnet/vnet.h>
#include <svm/svm_fifo_segment.h>
#include <svm/queue.h>
#include <vlibmemory/api.h>
#include <vppinfra/lock.h>
#include <vppinfra/valloc.h>

typedef struct _segment_manager_properties
{
  /** Session fifo sizes.  */
  u32 rx_fifo_size;
  u32 tx_fifo_size;

  /** Preallocated pool sizes */
//  u32 preallocated_fifo_pairs;

  /** Configured additional segment size */
  u32 add_segment_size;

  /** Flag that indicates if additional segments should be created */
  u8 add_segment;

  /** Segment type: if set to SSVM_N_TYPES, private segments are used */
  ssvm_segment_type_t segment_type;

  /** Use one or more private mheaps, instead of the global heap */
//  u32 private_segment_count;
} segment_manager_properties_t;

typedef struct _segment_manager
{
  /** Pool of segments allocated by this manager */
  svm_fifo_segment_private_t *segments;

  /** rwlock that protects the segments pool */
  clib_rwlock_t segments_rwlock;

  /** Owner app index */
  u32 app_index;

  /**
   * First segment should not be deleted unless segment manger is deleted.
   * This also indicates that the segment manager is the first to have been
   * allocated for the app.
   */
  u8 first_is_protected;

  /**
   * App event queue allocated in first segment
   */
  svm_queue_t *event_queue;
} segment_manager_t;

#define segment_manager_foreach_segment_w_lock(VAR, SM, BODY)		\
do {									\
    clib_rwlock_reader_lock (&(SM)->segments_rwlock);			\
    pool_foreach((VAR), ((SM)->segments), (BODY));			\
    clib_rwlock_reader_unlock (&(SM)->segments_rwlock);			\
} while (0)

typedef struct segment_manager_main_
{
  /** Pool of segment managers */
  segment_manager_t *segment_managers;

  /** Virtual address allocator */
  clib_valloc_main_t va_allocator;

} segment_manager_main_t;

extern segment_manager_main_t segment_manager_main;

typedef struct segment_manager_main_init_args_
{
  u64 baseva;
  u64 size;
} segment_manager_main_init_args_t;

#define SEGMENT_MANAGER_INVALID_APP_INDEX ((u32) ~0)

/** Pool of segment managers */
extern segment_manager_t *segment_managers;

always_inline segment_manager_t *
segment_manager_get (u32 index)
{
  return pool_elt_at_index (segment_manager_main.segment_managers, index);
}

always_inline segment_manager_t *
segment_manager_get_if_valid (u32 index)
{
  if (pool_is_free_index (segment_manager_main.segment_managers, index))
    return 0;
  return pool_elt_at_index (segment_manager_main.segment_managers, index);
}

always_inline u32
segment_manager_index (segment_manager_t * sm)
{
  return sm - segment_manager_main.segment_managers;
}

always_inline svm_queue_t *
segment_manager_event_queue (segment_manager_t * sm)
{
  return sm->event_queue;
}

segment_manager_t *segment_manager_new ();
int segment_manager_init (segment_manager_t * sm, u32 first_seg_size,
			  u32 evt_q_size, u32 prealloc_fifo_pairs);

svm_fifo_segment_private_t *segment_manager_get_segment (segment_manager_t *
							 sm,
							 u32 segment_index);
svm_fifo_segment_private_t
  * segment_manager_get_segment_w_lock (segment_manager_t * sm,
					u32 segment_index);
void segment_manager_segment_reader_unlock (segment_manager_t * sm);
void segment_manager_segment_writer_unlock (segment_manager_t * sm);

int segment_manager_add_first_segment (segment_manager_t * sm,
				       u32 segment_size);
void segment_manager_del_sessions (segment_manager_t * sm);
void segment_manager_del (segment_manager_t * sm);
void segment_manager_init_del (segment_manager_t * sm);
u8 segment_manager_has_fifos (segment_manager_t * sm);
int
segment_manager_alloc_session_fifos (segment_manager_t * sm,
				     svm_fifo_t ** server_rx_fifo,
				     svm_fifo_t ** server_tx_fifo,
				     u32 * fifo_segment_index);
void
segment_manager_dealloc_fifos (u32 svm_segment_index, svm_fifo_t * rx_fifo,
			       svm_fifo_t * tx_fifo);
svm_queue_t *segment_manager_alloc_queue (segment_manager_t * sm,
					  u32 queue_size);
void segment_manager_dealloc_queue (segment_manager_t * sm, svm_queue_t * q);
void segment_manager_app_detach (segment_manager_t * sm);

void segment_manager_main_init (segment_manager_main_init_args_t * a);
segment_manager_properties_t
  * segment_manager_properties_init (segment_manager_properties_t * sm);

#endif /* SRC_VNET_SESSION_SEGMENT_MANAGER_H_ */
/*
 * fd.io coding-style-patch-verification: ON
 *
 * Local Variables:
 * eval: (c-set-style "gnu")
 * End:
 */
