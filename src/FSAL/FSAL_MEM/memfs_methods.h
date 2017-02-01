/*
 * vim:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) Panasas Inc., 2011
 * Author: Jim Lieb jlieb@panasas.com
 *
 * contributeur : Philippe DENIEL   philippe.deniel@cea.fr
 *                Thomas LEIBOVICI  thomas.leibovici@cea.fr
 *
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * -------------
 */

/* MEMFS methods for handles
*/

#include "avltree.h"
#include "gsh_list.h"

struct mem_fsal_obj_handle;

/*
 * MEMFS internal export
 */
struct memfs_fsal_export {
	struct fsal_export export;
	char *export_path;
	struct mem_fsal_obj_handle *m_root_handle;
};

fsal_status_t memfs_lookup_path(struct fsal_export *exp_hdl,
				const char *path,
				struct fsal_obj_handle **handle,
				struct attrlist *attrs_out);

fsal_status_t memfs_create_handle(struct fsal_export *exp_hdl,
				  struct gsh_buffdesc *hdl_desc,
				  struct fsal_obj_handle **handle,
				  struct attrlist *attrs_out);

struct mem_fd {
	/** The open and share mode etc. */
	fsal_openflags_t openflags;
	/** Current file offset location */
	off_t offset;
};

/*
 * MEMFS internal object handle
 */

struct mem_fsal_obj_handle {
	struct fsal_obj_handle obj_handle;
	struct attrlist attrs;
	char *handle;
	struct mem_fsal_obj_handle *parent;
	union {
		struct {
			struct avltree avl_name;
			struct avltree avl_index;
			uint32_t numlinks;
		} mh_dir;
		struct {
			struct fsal_share share;
			struct mem_fd fd;
			off_t length;
		} mh_file;
		struct {
			object_file_type_t nodetype;
			fsal_dev_t dev;
		} mh_node;
		struct {
			char *link_contents;
		} mh_symlink;
	};
	struct avltree_node avl_n;
	struct avltree_node avl_i;
	uint32_t index; /* index in parent */
	uint32_t next_i; /* next child index */
	char *m_name;
	bool inavl;
};

static inline bool memfs_unopenable_type(object_file_type_t type)
{
	if ((type == SOCKET_FILE) || (type == CHARACTER_FILE)
	    || (type == BLOCK_FILE)) {
		return true;
	} else {
		return false;
	}
}

void memfs_handle_ops_init(struct fsal_obj_ops *ops);

/* Internal MEMFS method linkage to export object
*/

fsal_status_t memfs_create_export(struct fsal_module *fsal_hdl,
				  void *parse_node,
				  struct config_error_type *err_type,
				  const struct fsal_up_vector *up_ops);
