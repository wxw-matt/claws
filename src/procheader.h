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

#ifndef __PROCHEADER_H__
#define __PROCHEADER_H__

#include <glib.h>
#include <stdio.h>
#include <time.h>

#include "procmsg.h"

typedef struct _HeaderEntry	HeaderEntry;
typedef struct _Header		Header;

struct _HeaderEntry
{
	gchar	 *name;
	gchar	 *body;
	gboolean  unfold;
};

struct _Header
{
	gchar *name;
	gchar *body;
};

gint procheader_get_one_field		(gchar		*buf,
					 gint		 len,
					 FILE		*fp,
					 HeaderEntry	 hentry[]);
gchar *procheader_get_unfolded_line	(gchar		*buf,
					 gint		 len,
					 FILE		*fp);

GSList *procheader_get_header_list_from_file	(const gchar	*file);
GSList *procheader_get_header_list		(FILE		*fp);
GPtrArray *procheader_get_header_array		(FILE		*fp);
GPtrArray *procheader_get_header_array_asis	(FILE		*fp);
void procheader_header_list_destroy		(GSList		*hlist);
void procheader_header_array_destroy		(GPtrArray	*harray);
void procheader_header_free			(Header		*header);

const HeaderEntry* procheader_get_headernames	(gboolean	full);

void procheader_get_header_fields	(FILE		*fp,
					 HeaderEntry	 hentry[]);
MsgInfo *procheader_parse_file		(const gchar	*file,
					 MsgFlags	 flags,
					 gboolean	 full,
					 gboolean	 decrypted);
MsgInfo *procheader_parse_str		(const gchar	*str,
					 MsgFlags	 flags,
					 gboolean	 full,
					 gboolean	 decrypted);
MsgInfo *procheader_parse_stream	(FILE		*fp,
					 MsgFlags	 flags,
					 gboolean	 full,
					 gboolean	 decrypted);
MsgInfo *procheader_file_parse		(FILE		*fp,
					 MsgFlags	 flags,
					 gboolean	 full,
					 gboolean	 decrypted);

gchar *procheader_get_fromname		(const gchar	*str);

gboolean procheader_date_parse_to_tm	(const gchar	*str,
					 struct tm	*t,
					 char		*zone);

time_t procheader_date_parse		(gchar		*dest,
					 const gchar	*src,
					 gint		 len);
void procheader_date_get_localtime	(gchar		*dest,
					 gint		 len,
					 const time_t	 timer);
Header * procheader_parse_header        (gchar * buf);

gboolean procheader_headername_equal    (char * hdr1, char * hdr2);
void procheader_header_free             (Header * header);

/* Added by Mel Hadasht on 27 Aug 2001 */
/* Get a header from msginfo */
gint get_header_from_msginfo(MsgInfo *msginfo, gchar *buf, gint len,gchar *header);
#endif /* __PROCHEADER_H__ */
