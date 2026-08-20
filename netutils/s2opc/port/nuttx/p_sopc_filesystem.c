/****************************************************************************
 * apps/netutils/s2opc/port/nuttx/p_sopc_filesystem.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to Systerel under one or more contributor license agreements.
 * See the NOTICE file distributed with this work for additional information
 * regarding copyright ownership.  Systerel licenses this file to you under
 * the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the
 * License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "sopc_filesystem.h"
#include "sopc_helper_string.h"
#include "sopc_mem_alloc.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: s2opc_file_path
 ****************************************************************************/

static SOPC_ReturnStatus
s2opc_file_path(FAR const char *directory, FAR const char *name,
                FAR char **path)
{
  FAR char *prefix;
  SOPC_ReturnStatus status;

  prefix = NULL;
  status = SOPC_StrConcat(directory, "/", &prefix);
  if (status == SOPC_STATUS_OK)
    {
      status = SOPC_StrConcat(prefix, name, path);
    }

  SOPC_Free(prefix);
  return status;
}

/****************************************************************************
 * Name: s2opc_file_path_free
 ****************************************************************************/

static void s2opc_file_path_free(FAR void *data)
{
  if (data != NULL)
    {
      SOPC_Free(*(FAR char **)data);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: SOPC_FileSystem_mkdir
 ****************************************************************************/

SOPC_FileSystem_CreationResult
SOPC_FileSystem_mkdir(FAR const char *directory_path)
{
  if (mkdir(directory_path,
            S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0)
    {
      return SOPC_FileSystem_Creation_OK;
    }

  switch (errno)
    {
      case ENOENT:
        return SOPC_FileSystem_Creation_Error_PathPrefixInvalid;

      case EEXIST:
        return SOPC_FileSystem_Creation_Error_PathAlreadyExists;

      case EACCES:
        return SOPC_FileSystem_Creation_Error_PathPermisionDenied;

      default:
        return SOPC_FileSystem_Creation_Error_PathResolutionIssue;
    }
}

/****************************************************************************
 * Name: SOPC_FileSystem_rmdir
 ****************************************************************************/

SOPC_FileSystem_RemoveResult
SOPC_FileSystem_rmdir(FAR const char *directory_path)
{
  if (rmdir(directory_path) == 0)
    {
      return SOPC_FileSystem_Remove_OK;
    }

  switch (errno)
    {
      case ENOENT:
        return SOPC_FileSystem_Remove_Error_PathInvalid;

      case ENOTEMPTY:
      case EEXIST:
        return SOPC_FileSystem_Remove_Error_PathNotEmpty;

      case EACCES:
      case EPERM:
      case EBUSY:
        return SOPC_FileSystem_Remove_Error_PathPermisionDenied;

      default:
        return SOPC_FileSystem_Remove_Error_UnknownIssue;
    }
}

/****************************************************************************
 * Name: SOPC_FileSystem_GetDirFilePaths
 ****************************************************************************/

SOPC_FileSystem_GetDirResult
SOPC_FileSystem_GetDirFilePaths(FAR const char *directory_path,
                                FAR SOPC_Array **file_paths)
{
  FAR SOPC_Array *paths;
  FAR struct dirent *entry;
  FAR char *path;
  struct stat statbuf;
  FAR DIR *directory;
  SOPC_ReturnStatus status;

  if (directory_path == NULL || file_paths == NULL)
    {
      return SOPC_FileSystem_GetDir_Error_InvalidParameters;
    }

  *file_paths = NULL;
  directory = opendir(directory_path);
  if (directory == NULL)
    {
      return SOPC_FileSystem_GetDir_Error_PathInvalid;
    }

  paths = SOPC_Array_Create(sizeof(FAR char *), 0, s2opc_file_path_free);
  if (paths == NULL)
    {
      closedir(directory);
      return SOPC_FileSystem_GetDir_Error_UnknownIssue;
    }

  status = SOPC_STATUS_OK;
  errno = 0;
  while (status == SOPC_STATUS_OK && (entry = readdir(directory)) != NULL)
    {
      path = NULL;
      status = s2opc_file_path(directory_path, entry->d_name, &path);
      if (status == SOPC_STATUS_OK && stat(path, &statbuf) < 0)
        {
          status = SOPC_STATUS_NOK;
        }

      if (status == SOPC_STATUS_OK && S_ISREG(statbuf.st_mode))
        {
          if (SOPC_Array_Append(paths, path))
            {
              path = NULL;
            }
          else
            {
              status = SOPC_STATUS_OUT_OF_MEMORY;
            }
        }

      SOPC_Free(path);
      errno = 0;
    }

  if (errno != 0)
    {
      status = SOPC_STATUS_NOK;
    }

  closedir(directory);
  if (status != SOPC_STATUS_OK)
    {
      SOPC_Array_Delete(paths);
      return SOPC_FileSystem_GetDir_Error_UnknownIssue;
    }

  *file_paths = paths;
  return SOPC_FileSystem_GetDir_OK;
}

/****************************************************************************
 * Name: SOPC_FileSystem_fmemopen
 ****************************************************************************/

FAR FILE *SOPC_FileSystem_fmemopen(FAR void *buffer, size_t size,
                                   FAR const char *mode)
{
  return fmemopen(buffer, size, mode);
}
