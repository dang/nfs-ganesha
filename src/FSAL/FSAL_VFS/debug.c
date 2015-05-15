/*
 * vim:noexpandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) CohortFS LLC, 2015
 * Author: Daniel Gryniewicz dang@cohortfs.com
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

/* debug.c
 * VFS debug tracing
 */

#include "fsal_types.h"

static const char *ace_type(fsal_acetype_t type)
{
	switch (type) {
	case FSAL_ACE_TYPE_ALLOW:
		return "allow";
	case FSAL_ACE_TYPE_DENY:
		return "deny ";
	case FSAL_ACE_TYPE_AUDIT:
		return "audit";
	case FSAL_ACE_TYPE_ALARM:
		return "alarm";
	default:
		return "unknown";
	}
}

static const char *ace_perm(fsal_aceperm_t perm)
{
	static char buf[64];
	char *c = buf;

	if (perm & FSAL_ACE_PERM_READ_DATA)
		*c++ = 'r';
	else
		*c++ = '.';
	if (perm & FSAL_ACE_PERM_WRITE_DATA)
		*c++ = 'w';
	else
		*c++ = '.';
	if (perm & FSAL_ACE_PERM_APPEND_DATA)
		*c++ = 'a';
	else
		*c++ = '.';
	if (perm & FSAL_ACE_PERM_READ_NAMED_ATTR)
		*c++ = 'R';
	else
		*c++ = '.';
	if (perm & FSAL_ACE_PERM_WRITE_NAMED_ATTR)
		*c++ = 'W';
	else
		*c++ = '.';
	if (perm & FSAL_ACE_PERM_EXECUTE)
		*c++ = 'x';
	else
		*c++ = '.';
	if (perm & FSAL_ACE_PERM_DELETE_CHILD)
		*c++ = 'c';
	else
		*c++ = '.';
	if (perm & FSAL_ACE_PERM_READ_ATTR)
		*c++ = 'R';
	else
		*c++ = '.';
	if (perm & FSAL_ACE_PERM_WRITE_ATTR)
		*c++ = 'W';
	else
		*c++ = '.';
	if (perm & FSAL_ACE_PERM_DELETE)
		*c++ = 'd';
	else
		*c++ = '.';
	if (perm & FSAL_ACE_PERM_READ_ACL)
		*c++ = 'R';
	else
		*c++ = '.';
	if (perm & FSAL_ACE_PERM_WRITE_ACL)
		*c++ = 'W';
	else
		*c++ = '.';
	if (perm & FSAL_ACE_PERM_WRITE_OWNER)
		*c++ = 'o';
	else
		*c++ = '.';
	if (perm & FSAL_ACE_PERM_SYNCHRONIZE)
		*c++ = 's';
	else
		*c++ = '.';
	*c = '\0';

	return buf;
}

static const char *ace_flag(char *buf, fsal_aceflag_t flag)
{
	char *c = buf;

	if (flag & FSAL_ACE_FLAG_FILE_INHERIT)
		*c++ = 'f';
	else
		*c++ = '.';
	if (flag & FSAL_ACE_FLAG_DIR_INHERIT)
		*c++ = 'd';
	else
		*c++ = '.';
	if (flag & FSAL_ACE_FLAG_NO_PROPAGATE)
		*c++ = 'p';
	else
		*c++ = '.';
	if (flag & FSAL_ACE_FLAG_INHERIT_ONLY)
		*c++ = 'i';
	else
		*c++ = '.';
	if (flag & FSAL_ACE_FLAG_SUCCESSFUL)
		*c++ = 's';
	else
		*c++ = '.';
	if (flag & FSAL_ACE_FLAG_FAILED)
		*c++ = 'f';
	else
		*c++ = '.';
	if (flag & FSAL_ACE_FLAG_GROUP_ID)
		*c++ = 'g';
	else
		*c++ = '.';
	if (flag & FSAL_ACE_FLAG_INHERITED)
		*c++ = 'I';
	else
		*c++ = '.';
	if (flag & FSAL_ACE_IFLAG_EXCLUDE_FILES)
		*c++ = 'x';
	else
		*c++ = '.';
	if (flag & FSAL_ACE_IFLAG_EXCLUDE_DIRS)
		*c++ = 'X';
	else
		*c++ = '.';
	if (flag & FSAL_ACE_IFLAG_SPECIAL_ID)
		*c++ = 'S';
	else
		*c++ = '.';
	if (flag & FSAL_ACE_IFLAG_MODE_GEN)
		*c++ = 'G';
	else
		*c++ = '.';
	*c = '\0';

	return buf;
}

void print_ace(fsal_ace_t *ace, const char *func)
{
	char fbuf[16];
	char ibuf[16];
	LogDebug(COMPONENT_FSAL, "%s: ACE %s:%s-%s(%s)%u", func,
		 ace_type(ace->type), ace_perm(ace->perm),
		 ace_flag(fbuf, ace->flag), ace_flag(ibuf, ace->iflag),
		 ace->who.uid);
}

void print_acl(fsal_acl_t *acl, const char *func)
{
	fsal_ace_t *ace = NULL;

	LogDebug(COMPONENT_FSAL, "%s: %u aces:", func, acl->naces);
	for (ace = acl->aces; ace < acl->aces + acl->naces; ace++)
		print_ace(ace, func);
}
