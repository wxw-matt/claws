/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2002 Hiroyuki Yamamoto
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __PREFS_FILTER_H__
#define __PREFS_FILTER_H__

typedef enum
{
	FILTER_BY_NONE,
	FILTER_BY_AUTO,
	FILTER_BY_FROM,
	FILTER_BY_TO,
	FILTER_BY_SUBJECT
} PrefsFilterType;

void prefs_filter_read_config	(void);
void prefs_filter_write_config	(void);
void prefs_filter_open		(const gchar	*header,
				 const gchar	*key);

#endif /* __PREFS_FILTER_H__ */
