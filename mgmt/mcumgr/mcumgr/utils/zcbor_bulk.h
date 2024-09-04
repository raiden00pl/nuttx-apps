/****************************************************************************
 * apps/mgmt/mcumgr/mcumgr/zcbor_bulk.h
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

#ifndef __MGMT_MCUMGR_MCUMGR_ZCBOR_BULK_H
#define __MGMT_MCUMGR_MCUMGR_ZCBOR_BULK_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Name: ZCBOR_MAP_DECODE_KEY_VAL
 *
 * Description:
 *   Define single key-value decode mapping
 *
 * ZCBOR_MAP_DECODE_KEY_DECODER should be used instead of this macro as,
 * this macro does not allow keys with whitespaces embeeded, which CBOR
 * does allow.
 *
 * The macro creates a single zcbor_map_decode_key_val type object.
 *
 * Input Parameters:
 *   k   - key; the @p k will be stringified so should be given without "";
 *   dec - decoder function; this should be zcbor_decoder_t
 *         type function from zcbor or a user provided implementation
 *         compatible with the type.
 *   vp - non-NULL pointer for result of decoding; should correspond
 *        to type served by decoder function for the mapping.
 *
 ****************************************************************************/

#define ZCBOR_MAP_DECODE_KEY_VAL(k, dec, vp)                                \
  ZCBOR_MAP_DECODE_KEY_DECODER(STRINGIFY(k), dec, vp)

/****************************************************************************
 * Name: ZCBOR_MAP_DECODE_KEY_DECODER
 *
 * Description:
 *   Define single key-decoder mapping
 *
 *   The macro creates a single zcbor_map_decode_key_val type object.
 *
 * Input Parameters:
 *   k   - key is "" enclosed string representing key;
 *   dec - decoder function; this should be zcbor_decoder_t
 *         type function from zcbor or a user provided implementation
 *         compatible with the type.
 *   vp  - non-NULL pointer for result of decoding; should correspond
 *         to type served by decoder function for the mapping.
 *
 ****************************************************************************/

#define ZCBOR_MAP_DECODE_KEY_DECODER(k, dec, vp)                            \
  {                                                                         \
    {                                                                       \
      .value = k,                                                           \
      .len   = sizeof(k) - 1,                                               \
    },                                                                      \
        .decoder = (zcbor_decoder_t *)dec, .value_ptr = vp, .found = false, \
  }

/****************************************************************************
 * Public Types
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

struct zcbor_map_decode_key_val
{
  struct zcbor_string  key;     /* Map key string */
  FAR zcbor_decoder_t *decoder; /* Key corresponding decoder */
  FAR void            *value_ptr;
  bool                 found;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: zcbor_map_decode_bulk
 *
 * Description:
 *   Decodes single level map according to a provided key-decode map.
 *
 *   The function takes @p map of key to decoder array defined as:
 *
 *   struct zcbor_map_decode_key_val map[] =
 *    {
 *       ZCBOR_MAP_DECODE_KEY_DECODER("key0", decode_fun0, val_ptr0),
 *       ZCBOR_MAP_DECODE_KEY_DECODER("key1", decode_fun1, val_ptr1),
 *       ...
 *    };
 *
 *   where "key?" is string representing key; the decode_fun? is
 *   zcbor_decoder_t compatible function, either from zcbor or defined by
 *   user; val_ptr? are pointers to variables where decoder function for
 *   a given key will place a decoded value - they have to agree in type
 *   with decoder function.
 *
 *   Failure to decode any of values will cause the function to return
 *   negative error, and leave the map open: map is broken anyway or
 *   key-decoder mapping is broken, and we can not really decode the map.
 *
 *   Note that the function opens map by itself and will fail if map
 *   is already opened.
 *
 * Input Parameters:
 *   zsd      - zcbor decoder state;
 *   map      - key-decoder mapping list;
 *   map_size - size of maps, both maps have to have the same size;
 *   matched  - pointer to the  counter of matched keys, zeroed upon
 *              successful map entry and incremented only for successful
 *              decoded fields.
 *
 * Return Value:
 *   0 when the whole map has been parsed, there have been
 *   no decoding errors, and map has been closed successfully;
 *
 *   -ENOMSG when given decoder function failed to decode value;
 *
 *   -EADDRINUSE when key appears twice within map, map is then parsed up
 *   to they key that has appeared twice;
 *
 *   -EBADMSG when failed to close map.
 *
 ****************************************************************************/

int zcbor_map_decode_bulk(FAR zcbor_state_t *zsd,
                          FAR struct zcbor_map_decode_key_val *map,
                          size_t map_size, FAR size_t *matched);

/****************************************************************************
 * Name: zcbor_map_decode_bulk_key_found
 *
 * Description:
 *   Check whether key has been found by zcbor_map_decode_bulk
 *
 * Input Parameters:
 *   map      - key-decoder mapping list;
 *   map_size - size of maps, both maps have to have the same size;
 *   key      - string representing key to check with map.
 *
 * Return Value:
 *   true if key has been found during decoding, false otherwise.
 *
 ****************************************************************************/

bool zcbor_map_decode_bulk_key_found(
  FAR struct zcbor_map_decode_key_val *map,
  size_t map_size, FAR const char *key);

/****************************************************************************
 * Name: zcbor_map_decode_bulk_reset
 *
 * Description:
 *   Reset decoding state of key-value
 *
 *   The function takes @p map and resets internal fields that mark
 *   decoding state of the map. Function needs to be used on map after
 *   the map has been already used for decoding other buffer, otherwise
 *   decoding may fail.
 *
 * Input Parameters:
 *   map      - key-decoder mapping list;
 *   map_size - size of maps, both maps have to have the same size.
 *
 ****************************************************************************/

void zcbor_map_decode_bulk_reset(FAR struct zcbor_map_decode_key_val *map,
                                 size_t map_size);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __MGMT_MCUMGR_MCUMGR_ZCBOR_BULK_H */
