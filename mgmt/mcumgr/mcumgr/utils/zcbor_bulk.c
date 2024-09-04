/****************************************************************************
 * apps/mgmt/mcumgr/zcbor_bulk.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <string.h>
#include <errno.h>

#include <zcbor_common.h>
#include <zcbor_decode.h>

#include "zcbor_bulk.h"

/****************************************************************************
 * Public Function
 ****************************************************************************/

/****************************************************************************
 * Name: zcbor_map_decode_bulk
 ****************************************************************************/

int zcbor_map_decode_bulk(FAR zcbor_state_t *zsd,
                          FAR struct zcbor_map_decode_key_val *map,
                          size_t map_size, FAR size_t *matched)
{
	FAR struct zcbor_map_decode_key_val *dptr = map;
	bool ok;

	if (!zcbor_map_start_decode(zsd))
    {
      return -EBADMSG;
    }

	*matched = 0;
	ok = true;

	do
    {
      struct zcbor_string key;
      bool found = false;
      size_t map_count = 0;

      ok = zcbor_tstr_decode(zsd, &key);

      while (ok && map_count < map_size)
        {
          if (dptr >= (map + map_size))
            {
              dptr = map;
            }

          if (key.len == dptr->key.len &&
              memcmp(key.value, dptr->key.value, key.len) == 0)
            {
              if (dptr->found)
                {
                  return -EADDRINUSE;
                }

              if (!dptr->decoder(zsd, dptr->value_ptr))
                {
                  /* Failure to decode value matched to key
                   * means that either decoder has been
                   * incorrectly assigned or SMP payload
                   * is broken anyway.
                   */

                  return -ENOMSG;
                }

              dptr->found = true;
              found = true;
              ++dptr;
              ++(*matched);

              break;
            }

          ++dptr;
          ++map_count;
        }

      if (!found && ok)
        {
          ok = zcbor_any_skip(zsd, NULL);
        }

    } while (ok);

	return zcbor_map_end_decode(zsd) ? 0 : -EBADMSG;
}

/****************************************************************************
 * Name: zcbor_map_decode_bulk_key_found
 ****************************************************************************/

bool zcbor_map_decode_bulk_key_found(FAR struct zcbor_map_decode_key_val *map,
                                     size_t map_size, FAR const char *key)
{
  FAR struct zcbor_map_decode_key_val *dptr = map;
	size_t key_len;

	/* Lazy run, comparing pointers only assuming that compiler will be able
   * to store read-only string of the same value as single instance.
	 */

	while (dptr < (map + map_size))
    {
      if (dptr->key.value == (FAR const uint8_t *)key)
        {
          return dptr->found;
        }

      ++dptr;
    }

	/* Lazy run failed so need to do real comprison */

	key_len = strlen(key);
	dptr = map;

	while (dptr < (map + map_size))
    {
      if (dptr->key.len == key_len &&
          memcmp(key, dptr->key.value, key_len) == 0)
        {
          return dptr->found;
        }

      ++dptr;
    }

	return false;
}

/****************************************************************************
 * Name: zcbor_map_decode_bulk_reset
 ****************************************************************************/

void zcbor_map_decode_bulk_reset(FAR struct zcbor_map_decode_key_val *map,
                                 size_t map_size)
{
  size_t map_index;

	for (map_index = 0; map_index < map_size; ++map_index)
    {
      map[map_index].found = false;
    }
}
