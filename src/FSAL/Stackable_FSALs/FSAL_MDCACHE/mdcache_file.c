/*
 * vim:noexpandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) CohortFS LLC, 2015
 * Author: Daniel Gryniewicz <dang@cohortfs.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* file.c
 * File I/O methods for NULL module
 */

#include "config.h"

#include <assert.h>
#include "fsal.h"
#include "FSAL/access_check.h"
#include "fsal_convert.h"
#include <unistd.h>
#include <fcntl.h>
#include "FSAL/fsal_commonlib.h"
#include "mdcache_int.h"
#include "mdcache_lru.h"

/**
 *
 * @brief Set a timestamp to the current time
 *
 * @param[out] time Pointer to time to be set
 *
 * @return true on success, false on failure
 *
 */
bool
mdc_set_time_current(struct timespec *time)
{
	struct timeval t;

	if (time == NULL)
		return false;

	if (gettimeofday(&t, NULL) != 0)
		return false;

	time->tv_sec = t.tv_sec;
	time->tv_nsec = 1000 * t.tv_usec;

	return true;
}

/**
 * @brief Open a file
 *
 * Delegate to sub-FSAL, subject to hard limits on the number of open FDs
 *
 * @param[in] obj_hdl	File to open
 * @param[in] openflags	Type of open to do
 * @return FSAL status
 */
fsal_status_t mdcache_open(struct fsal_obj_handle *obj_hdl,
			   fsal_openflags_t openflags)
{
	mdcache_entry_t *entry =
		container_of(obj_hdl, mdcache_entry_t, obj_handle);
	fsal_status_t status;

	if (!mdcache_lru_fds_available()) {
		/* This seems the best idea, let the client try again later
		   after the reap. */
		return fsalstat(ERR_FSAL_DELAY, 0);
	}

	subcall(
		status = entry->sub_handle->obj_ops.open(
			entry->sub_handle, openflags)
	       );

	if (FSAL_IS_ERROR(status) && (status.major == ERR_FSAL_STALE))
		mdcache_kill_entry(entry);

	return status;
}

/**
 * @brief Re-open a file with different flags
 *
 * Delegate to sub-FSAL.  This should not be called unless the sub-FSAL supports
 * reopen.
 *
 * @param[in] obj_hdl	File to re-open
 * @param[in] openflags	New open flags
 * @return FSAL status
 */
fsal_status_t mdcache_reopen(struct fsal_obj_handle *obj_hdl,
			   fsal_openflags_t openflags)
{
	mdcache_entry_t *entry =
		container_of(obj_hdl, mdcache_entry_t, obj_handle);
	fsal_status_t status;

	subcall(
		status = entry->sub_handle->obj_ops.open(
			entry->sub_handle, openflags)
	       );

	if (FSAL_IS_ERROR(status) && (status.major == ERR_FSAL_STALE))
		mdcache_kill_entry(entry);

	return status;
}

/**
 * @brief Get the open status of a file
 *
 * Delegate to sub-FSAL, since this isn't cached metadata currently
 *
 * @param[in] obj_hdl	Object to check
 * @return Open flags indicating state
 */
fsal_openflags_t mdcache_status(struct fsal_obj_handle *obj_hdl)
{
	mdcache_entry_t *entry =
		container_of(obj_hdl, mdcache_entry_t, obj_handle);
	fsal_openflags_t status;

	subcall(
		status = entry->sub_handle->obj_ops.status(
			entry->sub_handle)
	       );

	return status;
}

/**
 * @brief Read from a file
 *
 * Delegate to sub-FSAL
 *
 * @param[in] obj_hdl	Object to read from
 * @param[in] offset	Offset into file
 * @param[in] buf_size	Size of read buffer
 * @param[out] buffer	Buffer to read into
 * @param[out] read_amount	Amount read in bytes
 * @param[out] eof	true if End of File was hit
 * @return FSAL status
 */
fsal_status_t mdcache_read(struct fsal_obj_handle *obj_hdl, uint64_t offset,
			   size_t buf_size, void *buffer,
			   size_t *read_amount, bool *eof)
{
	mdcache_entry_t *entry =
		container_of(obj_hdl, mdcache_entry_t, obj_handle);
	fsal_status_t status;

	subcall(
		status = entry->sub_handle->obj_ops.read(
			entry->sub_handle, offset, buf_size, buffer,
			read_amount, eof)
	       );

	if (!FSAL_IS_ERROR(status)) {
		mdc_set_time_current(&obj_hdl->attrs->atime);
	} else if (status.major == ERR_FSAL_DELAY) {
		mdcache_kill_entry(entry);
	}

	return status;
}

/**
 * @brief Read from a file w/ extra info
 *
 * Delegate to sub-FSAL
 *
 * @param[in] obj_hdl	Object to read from
 * @param[in] offset	Offset into file
 * @param[in] buf_size	Size of read buffer
 * @param[out] buffer	Buffer to read into
 * @param[out] read_amount	Amount read in bytes
 * @param[out] eof	true if End of File was hit
 * @param[in,out] info	Extra info about read data
 * @return FSAL status
 */
fsal_status_t mdcache_read_plus(struct fsal_obj_handle *obj_hdl,
				uint64_t offset, size_t buf_size,
				void *buffer, size_t *read_amount,
				bool *eof, struct io_info *info)
{
	mdcache_entry_t *entry =
		container_of(obj_hdl, mdcache_entry_t, obj_handle);
	fsal_status_t status;

	subcall(
		status = entry->sub_handle->obj_ops.read_plus(
			entry->sub_handle, offset, buf_size, buffer,
			read_amount, eof, info)
	       );

	if (!FSAL_IS_ERROR(status)) {
		mdc_set_time_current(&obj_hdl->attrs->atime);
	} else if (status.major == ERR_FSAL_DELAY) {
		mdcache_kill_entry(entry);
	}

	return status;
}

/**
 * @brief Write to a file
 *
 * Delegate to sub-FSAL
 *
 * @param[in] obj_hdl	Object to write to
 * @param[in] offset	Offset into file
 * @param[in] buf_size	Size of write buffer
 * @param[in] buffer	Buffer to write from
 * @param[out] write_amount	Amount written in bytes
 * @param[out] fsal_stable	true if write was to stable storage
 * @return FSAL status
 */
fsal_status_t mdcache_write(struct fsal_obj_handle *obj_hdl, uint64_t offset,
			    size_t buf_size, void *buffer,
			    size_t *write_amount, bool *fsal_stable)
{
	mdcache_entry_t *entry =
		container_of(obj_hdl, mdcache_entry_t, obj_handle);
	fsal_status_t status;

	subcall(
		status = entry->sub_handle->obj_ops.write(
			entry->sub_handle, offset, buf_size, buffer,
			write_amount, fsal_stable)
	       );

	if (status.major == ERR_FSAL_DELAY) {
		mdcache_kill_entry(entry);
	}

	return status;
}

/**
 * @brief Write to a file w/ extra info
 *
 * Delegate to sub-FSAL
 *
 * @param[in] obj_hdl	Object to read from
 * @param[in] offset	Offset into file
 * @param[in] buf_size	Size of read buffer
 * @param[out] buffer	Buffer to read into
 * @param[out] read_amount	Amount read in bytes
 * @param[out] eof	true if End of File was hit
 * @param[in,out] info	Extra info about write data
 * @return FSAL status
 */
fsal_status_t mdcache_write_plus(struct fsal_obj_handle *obj_hdl,
				 uint64_t offset, size_t buf_size,
				 void *buffer, size_t *write_amount,
				 bool *fsal_stable, struct io_info *info)
{
	mdcache_entry_t *entry =
		container_of(obj_hdl, mdcache_entry_t, obj_handle);
	fsal_status_t status;

	subcall(
		status = entry->sub_handle->obj_ops.write_plus(
			entry->sub_handle, offset, buf_size, buffer,
			write_amount, fsal_stable, info)
	       );

	if (status.major == ERR_FSAL_DELAY) {
		mdcache_kill_entry(entry);
	}

	return status;
}

/**
 * @brief Commit to a file
 *
 * Delegate to sub-FSAL
 *
 * @param[in] obj_hdl	Object to commit
 * @param[in] offset	Offset into file
 * @param[in] len	Length of commit
 * @return FSAL status
 */
fsal_status_t mdcache_commit(struct fsal_obj_handle *obj_hdl, off_t offset,
			     size_t len)
{
	mdcache_entry_t *entry =
		container_of(obj_hdl, mdcache_entry_t, obj_handle);
	fsal_status_t status;

	subcall(
		status = entry->sub_handle->obj_ops.commit(
			entry->sub_handle, offset, len)
	       );

	if (status.major == ERR_FSAL_STALE)
		mdcache_kill_entry(entry);

	return status;
}

/**
 * @brief Lock/unlock a range in a file
 *
 * Delegate to sub-FSAL
 *
 * @param[in] obj_hdl	File to lock
 * @param[in] p_owner	Private data for lock
 * @param[in] lock_op	Lock operation
 * @param[in] req_lock	Parameters for requested lock
 * @param[in] conflicting_lock	Description of existing conflicting lock
 * @return FSAL status
 */
fsal_status_t mdcache_lock_op(struct fsal_obj_handle *obj_hdl,
			      void *p_owner, fsal_lock_op_t lock_op,
			      fsal_lock_param_t *req_lock,
			      fsal_lock_param_t *conflicting_lock)
{
	mdcache_entry_t *entry =
		container_of(obj_hdl, mdcache_entry_t, obj_handle);
	fsal_status_t status;

	subcall(
		status = entry->sub_handle->obj_ops.lock_op(
			entry->sub_handle, p_owner, lock_op, req_lock,
			conflicting_lock)
	       );

	return status;
}

/**
 * @brief Close a file
 *
 * @param[in] obj_hdl	File to close
 * @return FSAL status
 */
fsal_status_t mdcache_close(struct fsal_obj_handle *obj_hdl)
{
	mdcache_entry_t *entry =
		container_of(obj_hdl, mdcache_entry_t, obj_handle);
	fsal_status_t status;

	// XXX dang caching FDs?  How does it interact with multi-FD
	subcall(
		status = entry->sub_handle->obj_ops.close(entry->sub_handle)
	       );

	return status;
}
