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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "defs.h"

#include <glib.h>
#include <string.h>
#include <locale.h>

#include "prefs_common.h"
#include "manual.h"
#include "utils.h"

static gchar *sylpheeddoc_manuals[] =
{
    "en",
    "es", 
    "fr", 
    "de",
};

static gchar *get_language()
{
	gchar *language;
	gchar *c;

#ifdef WIN32
	language = g_strdup(gtk_set_locale());
#else
	language = g_strdup(setlocale(LC_ALL, NULL));
#endif
	if((c = strchr(language, ',')) != NULL)
		*c = '\0';
	if((c = strchr(language, '_')) != NULL)
		*c = '\0';

	return language;
}

static gchar *get_local_path_with_locale(gchar *rootpath)
{
	gchar *lang_str, *dir;

	lang_str = get_language();
#ifdef WIN32
	dir = g_strconcat(get_installed_dir(), G_DIR_SEPARATOR_S,
			  rootpath, G_DIR_SEPARATOR_S,
#else
	dir = g_strconcat(rootpath, G_DIR_SEPARATOR_S, 
#endif
			  lang_str, NULL);
	g_free(lang_str);
	if(!is_dir_exist(dir)) {
		g_free(dir);
#ifdef WIN32
		dir = g_strconcat(get_installed_dir(), G_DIR_SEPARATOR_S,
				  rootpath, G_DIR_SEPARATOR_S,
#else
		dir = g_strconcat(rootpath, G_DIR_SEPARATOR_S,
#endif
				  "en", NULL);
		if(!is_dir_exist(dir)) {
			g_free(dir);
			dir = NULL;
		}
	}

	return dir;
}

gboolean manual_available(ManualType type)
{
	gboolean ret = FALSE;
    	gchar *dir = NULL;
	
	switch (type) {
		case MANUAL_MANUAL_LOCAL:
			dir = get_local_path_with_locale(MANUALDIR);
			if (dir != NULL) {
				g_free(dir);
				ret = TRUE;
			}
			break;
		case MANUAL_FAQ_LOCAL:
			dir = get_local_path_with_locale(FAQDIR);
			if (dir != NULL) {
				g_free(dir);
				ret = TRUE;
			}
			break;
		default:
			ret = FALSE;
	}

	return ret;
}

static gchar *get_syldoc_language()
{
	gchar *language;
	int i;
	
	language = get_language();
	for (i = 0; i < sizeof(sylpheeddoc_manuals) / sizeof(sylpheeddoc_manuals[0]); i++) {
		if (strcmp(language, sylpheeddoc_manuals[i]) == 0) {
			return language;
		}
	}
	g_free(language);
	
	return g_strdup("en");
}

void manual_open(ManualType type)
{
	gchar *uri = NULL;
	gchar *dir;
	gchar *lang_str;

	switch (type) {
		case MANUAL_MANUAL_LOCAL:
			dir = get_local_path_with_locale(MANUALDIR);
			if (dir != NULL) {
				uri = g_strconcat("file://", dir, G_DIR_SEPARATOR_S, MANUAL_HTML_INDEX, NULL);
				g_free(dir);
			}
			break;

		case MANUAL_FAQ_LOCAL:
			dir = get_local_path_with_locale(FAQDIR);
			if (dir != NULL) {
				uri = g_strconcat("file://", dir, G_DIR_SEPARATOR_S, FAQ_HTML_INDEX, NULL);
				g_free(dir);
			}
			break;

		case MANUAL_MANUAL_SYLDOC:
			lang_str = get_syldoc_language();
			uri = g_strconcat(SYLDOC_URI, lang_str, SYLDOC_MANUAL_HTML_INDEX, NULL);
			g_free(lang_str);
			break;

		case MANUAL_FAQ_SYLDOC:
			lang_str = get_syldoc_language();
			uri = g_strconcat(SYLDOC_URI, lang_str, SYLDOC_FAQ_HTML_INDEX, NULL);
			g_free(lang_str);
			break;

		case MANUAL_FAQ_CLAWS:
			uri = g_strconcat(CLAWS_FAQ_URI, NULL);
			break;

		default:
			break;
	}
#ifdef WIN32
	translate_strs(uri, G_DIR_SEPARATOR_S, "/");
#endif
	open_uri(uri, prefs_common.uri_cmd);
	g_free(uri);
}
