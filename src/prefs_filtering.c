/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2003 Hiroyuki Yamamoto and the Sylpheed-Claws team
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
#include <gtk/gtk.h>
#include <gtk/gtkoptionmenu.h>
#include <gdk/gdkkeysyms.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "intl.h"
#include "main.h"
#include "prefs_gtk.h"
#include "prefs_matcher.h"
#include "prefs_filtering.h"
#include "prefs_common.h"
#include "mainwindow.h"
#include "foldersel.h"
#include "manage_window.h"
#include "inc.h"
#include "utils.h"
#include "gtkutils.h"
#include "alertpanel.h"
#include "folder.h"
#include "filtering.h"
#include "addr_compl.h"
#include "colorlabel.h"

#include "matcher_parser.h"
#include "matcher.h"

static struct Filtering {
	GtkWidget *window;

	GtkWidget *ok_btn;
	GtkWidget *cond_entry;
	GtkWidget *action_list;
	GtkWidget *action_combo;
	GtkWidget *account_label;
	GtkWidget *account_list;
	GtkWidget *account_combo;
	GtkWidget *dest_entry;
	GtkWidget *dest_btn;
	GtkWidget *dest_label;
	GtkWidget *recip_label;
	GtkWidget *exec_label;
	GtkWidget *exec_btn;

	GtkWidget *color_label;
	GtkWidget *color_optmenu;

	GtkWidget *cond_clist;

	/* need this to make address completion entry work */
	gint current_action;
} filtering;

/* widget creating functions */
static void prefs_filtering_create		(void);

static void prefs_filtering_set_dialog	(const gchar *header,
					 const gchar *key);
static void prefs_filtering_set_list	(void);

/* callback functions */
static void prefs_filtering_register_cb	(void);
static void prefs_filtering_substitute_cb	(void);
static void prefs_filtering_delete_cb	(void);
static void prefs_filtering_up		(void);
static void prefs_filtering_down	(void);
static void prefs_filtering_select	(GtkCList	*clist,
					 gint		 row,
					 gint		 column,
					 GdkEvent	*event);

static gint prefs_filtering_deleted	(GtkWidget	*widget,
					 GdkEventAny	*event,
					 gpointer	 data);
static void prefs_filtering_key_pressed	(GtkWidget	*widget,
					 GdkEventKey	*event,
					 gpointer	 data);
static void prefs_filtering_cancel	(void);
static void prefs_filtering_ok		(void);

static void prefs_filtering_condition_define	(void);
static gint prefs_filtering_clist_set_row	(gint row, FilteringProp * prop);
static void prefs_filtering_select_dest		(void);
static void prefs_filtering_action_select	(GtkList *list,
						 GtkWidget *widget, 
						 gpointer user_data);

static void prefs_filtering_action_selection_changed(GtkList *list,
						     gpointer user_data);
					  
static void prefs_filtering_reset_dialog	(void);
static gboolean prefs_filtering_rename_path_func(GNode *node, gpointer data);
static gboolean prefs_filtering_delete_path_func(GNode *node, gpointer data);

static FolderItem * cur_item = NULL; /* folder (if dialog opened for processing) */

typedef enum Action_ {
	ACTION_MOVE,
	ACTION_COPY,
	ACTION_DELETE,
	ACTION_MARK,
	ACTION_UNMARK,
	ACTION_LOCK,
	ACTION_UNLOCK,
	ACTION_MARK_AS_READ,
	ACTION_MARK_AS_UNREAD,
	ACTION_FORWARD,
	ACTION_FORWARD_AS_ATTACHMENT,
	ACTION_REDIRECT,
	ACTION_EXECUTE,
	ACTION_COLOR,
	/* add other action constants */
} Action;

static struct {
	gchar *text;
	Action action;
} action_text [] = {
	{ N_("Move"),			ACTION_MOVE	}, 	
	{ N_("Copy"),			ACTION_COPY	},
	{ N_("Delete"),			ACTION_DELETE	},
	{ N_("Mark"),			ACTION_MARK	},
	{ N_("Unmark"),			ACTION_UNMARK   },
	{ N_("Lock"),			ACTION_LOCK	},
	{ N_("Unlock"),			ACTION_UNLOCK	},
	{ N_("Mark as read"),		ACTION_MARK_AS_READ },
	{ N_("Mark as unread"),		ACTION_MARK_AS_UNREAD },
	{ N_("Forward"),		ACTION_FORWARD  },
	{ N_("Forward as attachment"),	ACTION_FORWARD_AS_ATTACHMENT },
	{ N_("Redirect"),		ACTION_REDIRECT },
	{ N_("Execute"),		ACTION_EXECUTE	},
	{ N_("Color"),			ACTION_COLOR	}
};

static const gchar *get_action_text(Action action)
{
	int n;
	for (n = 0; n < sizeof action_text / sizeof action_text[0]; n++)
		if (action_text[n].action == action)
			return action_text[n].text;
	return "";			
}

static gint get_sel_from_list(GtkList * list)
{
	gint row = 0;
	void * sel;
	GList * child;

	if (list->selection == NULL)
		return -1;

	sel = list->selection->data;
	for(child = list->children ; child != NULL ;
	    child = g_list_next(child)) {
		if (child->data == sel)
			return row;
		row ++;
	}
	
	return row;
}

static gint get_account_id_from_list_id(gint list_id)
{
	GList * accounts;

	for (accounts = account_get_list() ; accounts != NULL;
	     accounts = accounts->next) {
		PrefsAccount *ac = (PrefsAccount *)accounts->data;

		if (list_id == 0)
			return ac->account_id;
		list_id--;
	}
	return 0;
}

static gint get_list_id_from_account_id(gint account_id)
{
	GList * accounts;
	gint list_id = 0;

	for (accounts = account_get_list() ; accounts != NULL;
	     accounts = accounts->next) {
		PrefsAccount *ac = (PrefsAccount *)accounts->data;

		if (account_id == ac->account_id)
			return list_id;
		list_id++;
	}
	return 0;
}

static gint prefs_filtering_get_matching_from_action(Action action_id)
{
	switch (action_id) {
	case ACTION_MOVE:
		return MATCHACTION_MOVE;
	case ACTION_COPY:
		return MATCHACTION_COPY;
	case ACTION_DELETE:
		return MATCHACTION_DELETE;
	case ACTION_MARK:
		return MATCHACTION_MARK;
	case ACTION_UNMARK:
		return MATCHACTION_UNMARK;
	case ACTION_LOCK:
		return MATCHACTION_LOCK;
	case ACTION_UNLOCK:
		return MATCHACTION_UNLOCK;
	case ACTION_MARK_AS_READ:
		return MATCHACTION_MARK_AS_READ;
	case ACTION_MARK_AS_UNREAD:
		return MATCHACTION_MARK_AS_UNREAD;
	case ACTION_FORWARD:
		return MATCHACTION_FORWARD;
	case ACTION_FORWARD_AS_ATTACHMENT:
		return MATCHACTION_FORWARD_AS_ATTACHMENT;
	case ACTION_REDIRECT:
		return MATCHACTION_REDIRECT;
	case ACTION_EXECUTE:
		return MATCHACTION_EXECUTE;
	case ACTION_COLOR:
		return MATCHACTION_COLOR;
	default:
		return -1;
	}
}

void prefs_filtering_open(FolderItem * item,
			  const gchar *header,
			  const gchar *key)
{
	gchar *esckey;

	if (prefs_rc_is_readonly(FILTERING_RC))
		return;

	inc_lock();

	if (!filtering.window) {
		prefs_filtering_create();
	}

	manage_window_set_transient(GTK_WINDOW(filtering.window));
	gtk_widget_grab_focus(filtering.ok_btn);

	cur_item = item;

	esckey = matcher_escape_str(key);
	prefs_filtering_set_dialog(header, esckey);
	g_free(esckey);

	gtk_widget_show(filtering.window);

	start_address_completion();
}

/* prefs_filtering_close() - just to have one common exit point */
static void prefs_filtering_close(void)
{
	end_address_completion();
	
	gtk_widget_hide(filtering.window);
	inc_unlock();
}

static void prefs_filtering_create(void)
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *ok_btn;
	GtkWidget *cancel_btn;
	GtkWidget *confirm_area;

	GtkWidget *vbox1;
	GtkWidget *hbox1;
	GtkWidget *reg_hbox;
	GtkWidget *arrow;
	GtkWidget *btn_hbox;

	GtkWidget *cond_label;
	GtkWidget *cond_entry;
	GtkWidget *cond_btn;
	GtkWidget *action_label;
	GtkWidget *action_list;
	GtkWidget *action_combo;
	GtkWidget *account_label;
	GtkWidget *account_list;
	GtkWidget *account_combo;
	GtkWidget *dest_label;
	GtkWidget *recip_label;
	GtkWidget *exec_label;
	GtkWidget *dest_entry;
	GtkWidget *dest_btn;
	GtkWidget *exec_btn;
	GtkWidget *color_label;
	GtkWidget *color_optmenu;

	GtkWidget *reg_btn;
	GtkWidget *subst_btn;
	GtkWidget *del_btn;

	GtkWidget *cond_hbox;
	GtkWidget *cond_scrolledwin;
	GtkWidget *cond_clist;

	GtkWidget *btn_vbox;
	GtkWidget *up_btn;
	GtkWidget *down_btn;


	GList *combo_items;
	gint i;

	GList *accounts;
	GList * cur;

	gchar *title[1];

	debug_print("Creating filtering configuration window...\n");

	window = gtk_window_new (GTK_WINDOW_DIALOG);
	gtk_container_set_border_width (GTK_CONTAINER (window), 8);
	gtk_window_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
	gtk_window_set_modal (GTK_WINDOW (window), TRUE);
	gtk_window_set_policy (GTK_WINDOW (window), FALSE, TRUE, FALSE);

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox);
	gtk_container_add (GTK_CONTAINER (window), vbox);

	gtkut_button_set_create(&confirm_area, &ok_btn, _("OK"),
				&cancel_btn, _("Cancel"), NULL, NULL);
	gtk_widget_show (confirm_area);
	gtk_box_pack_end (GTK_BOX(vbox), confirm_area, FALSE, FALSE, 0);
	gtk_widget_grab_default (ok_btn);

	gtk_window_set_title (GTK_WINDOW(window),
			      	    _("Filtering/Processing configuration"));

	gtk_signal_connect (GTK_OBJECT(window), "delete_event",
			    GTK_SIGNAL_FUNC(prefs_filtering_deleted), NULL);
	gtk_signal_connect (GTK_OBJECT(window), "key_press_event",
			    GTK_SIGNAL_FUNC(prefs_filtering_key_pressed), NULL);
	MANAGE_WINDOW_SIGNALS_CONNECT (window);
	gtk_signal_connect (GTK_OBJECT(ok_btn), "clicked",
			    GTK_SIGNAL_FUNC(prefs_filtering_ok), NULL);
	gtk_signal_connect (GTK_OBJECT(cancel_btn), "clicked",
			    GTK_SIGNAL_FUNC(prefs_filtering_cancel), NULL);

	vbox1 = gtk_vbox_new (FALSE, VSPACING);
	gtk_widget_show (vbox1);
	gtk_box_pack_start (GTK_BOX (vbox), vbox1, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), 2);

	cond_label = gtk_label_new (_("Condition"));
	gtk_widget_show (cond_label);
	gtk_misc_set_alignment (GTK_MISC (cond_label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox1), cond_label, FALSE, FALSE, 0);

	hbox1 = gtk_hbox_new (FALSE, VSPACING);
	gtk_widget_show (hbox1);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), 2);

	cond_entry = gtk_entry_new ();
	gtk_widget_show (cond_entry);
	gtk_box_pack_start (GTK_BOX (hbox1), cond_entry, TRUE, TRUE, 0);

	cond_btn = gtk_button_new_with_label (_("Define ..."));
	gtk_widget_show (cond_btn);
	gtk_box_pack_start (GTK_BOX (hbox1), cond_btn, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (cond_btn), "clicked",
			    GTK_SIGNAL_FUNC (prefs_filtering_condition_define),
			    NULL);

	hbox1 = gtk_hbox_new (FALSE, VSPACING);
	gtk_widget_show (hbox1);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), 2);

	action_label = gtk_label_new (_("Action"));
	gtk_widget_show (action_label);
	gtk_misc_set_alignment (GTK_MISC (action_label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox1), action_label, FALSE, FALSE, 0);

	action_combo = gtk_combo_new ();
	gtk_widget_show (action_combo);
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(action_combo)->entry),
			       FALSE);

	combo_items = NULL;
	for (i = 0; i < sizeof action_text / sizeof action_text[0]; i++)
		combo_items = g_list_append
			(combo_items, (gpointer) _(action_text[i].text));
	gtk_combo_set_popdown_strings(GTK_COMBO(action_combo), combo_items);

	g_list_free(combo_items);

	gtk_box_pack_start (GTK_BOX (hbox1), action_combo,
			    TRUE, TRUE, 0);
	action_list = GTK_COMBO(action_combo)->list;
	gtk_signal_connect (GTK_OBJECT (action_list), "select-child",
			    GTK_SIGNAL_FUNC (prefs_filtering_action_select),
			    NULL);

	gtk_signal_connect(GTK_OBJECT(action_list), "selection-changed",
			   GTK_SIGNAL_FUNC(prefs_filtering_action_selection_changed),
			   NULL);

	/* accounts */

	hbox1 = gtk_hbox_new (FALSE, VSPACING);
	gtk_widget_show (hbox1);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), 2);

	account_label = gtk_label_new (_("Account"));
	gtk_widget_show (account_label);
	gtk_misc_set_alignment (GTK_MISC (account_label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox1), account_label, FALSE, FALSE, 0);

	account_combo = gtk_combo_new ();
	gtk_widget_show (account_combo);

	combo_items = NULL;
	for (accounts = account_get_list() ; accounts != NULL;
	     accounts = accounts->next) {
		PrefsAccount *ac = (PrefsAccount *)accounts->data;
		gchar *name;

		name = g_strdup_printf("%s <%s> (%s)",
				       ac->name, ac->address,
				       ac->account_name);
		combo_items = g_list_append(combo_items, (gpointer) name);
	}

	gtk_combo_set_popdown_strings(GTK_COMBO(account_combo), combo_items);

	for(cur = g_list_first(combo_items) ; cur != NULL ;
	    cur = g_list_next(cur))
		g_free(cur->data);
	g_list_free(combo_items);

	gtk_box_pack_start (GTK_BOX (hbox1), account_combo,
			    TRUE, TRUE, 0);
	account_list = GTK_COMBO(account_combo)->list;
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(account_combo)->entry),
			       FALSE);

	/* destination */

	hbox1 = gtk_hbox_new (FALSE, VSPACING);
	gtk_widget_show (hbox1);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), 2);

	dest_label = gtk_label_new (_("Destination"));
	gtk_widget_show (dest_label);
	gtk_misc_set_alignment (GTK_MISC (dest_label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox1), dest_label, FALSE, FALSE, 0);

	recip_label = gtk_label_new (_("Recipient"));
	gtk_widget_show (recip_label);
	gtk_misc_set_alignment (GTK_MISC (recip_label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox1), recip_label, FALSE, FALSE, 0);

	exec_label = gtk_label_new (_("Execute"));
	gtk_widget_show (exec_label);
	gtk_misc_set_alignment (GTK_MISC (exec_label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox1), exec_label, FALSE, FALSE, 0);
	
	color_label = gtk_label_new (_("Color"));
	gtk_widget_show(color_label);
	gtk_misc_set_alignment(GTK_MISC(color_label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox1), color_label, FALSE, FALSE, 0);

	dest_entry = gtk_entry_new ();
	gtk_widget_show (dest_entry);
	gtk_box_pack_start (GTK_BOX (hbox1), dest_entry, TRUE, TRUE, 0);
	
	color_optmenu = gtk_option_menu_new();
	gtk_option_menu_set_menu(GTK_OPTION_MENU(color_optmenu),
				 colorlabel_create_color_menu());
	gtk_box_pack_start(GTK_BOX(hbox1), color_optmenu, TRUE, TRUE, 0);

	dest_btn = gtk_button_new_with_label (_("Select ..."));
	gtk_widget_show (dest_btn);
	gtk_box_pack_start (GTK_BOX (hbox1), dest_btn, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (dest_btn), "clicked",
			    GTK_SIGNAL_FUNC (prefs_filtering_select_dest),
			    NULL);

	exec_btn = gtk_button_new_with_label (_("Info ..."));
	gtk_widget_show (exec_btn);
	gtk_box_pack_start (GTK_BOX (hbox1), exec_btn, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (exec_btn), "clicked",
			    GTK_SIGNAL_FUNC (prefs_matcher_exec_info),
			    NULL);

	/* register / substitute / delete */

	reg_hbox = gtk_hbox_new (FALSE, 4);
	gtk_widget_show (reg_hbox);
	gtk_box_pack_start (GTK_BOX (vbox1), reg_hbox, FALSE, FALSE, 0);

	arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT);
	gtk_widget_show (arrow);
	gtk_box_pack_start (GTK_BOX (reg_hbox), arrow, FALSE, FALSE, 0);
	gtk_widget_set_usize (arrow, -1, 16);

	btn_hbox = gtk_hbox_new (TRUE, 4);
	gtk_widget_show (btn_hbox);
	gtk_box_pack_start (GTK_BOX (reg_hbox), btn_hbox, FALSE, FALSE, 0);

	reg_btn = gtk_button_new_with_label (_("Add"));
	gtk_widget_show (reg_btn);
	gtk_box_pack_start (GTK_BOX (btn_hbox), reg_btn, FALSE, TRUE, 0);
	gtk_signal_connect (GTK_OBJECT (reg_btn), "clicked",
			    GTK_SIGNAL_FUNC (prefs_filtering_register_cb), NULL);

	subst_btn = gtk_button_new_with_label (_("  Replace  "));
	gtk_widget_show (subst_btn);
	gtk_box_pack_start (GTK_BOX (btn_hbox), subst_btn, FALSE, TRUE, 0);
	gtk_signal_connect (GTK_OBJECT (subst_btn), "clicked",
			    GTK_SIGNAL_FUNC (prefs_filtering_substitute_cb),
			    NULL);

	del_btn = gtk_button_new_with_label (_("Delete"));
	gtk_widget_show (del_btn);
	gtk_box_pack_start (GTK_BOX (btn_hbox), del_btn, FALSE, TRUE, 0);
	gtk_signal_connect (GTK_OBJECT (del_btn), "clicked",
			    GTK_SIGNAL_FUNC (prefs_filtering_delete_cb), NULL);

	cond_hbox = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (cond_hbox);
	gtk_box_pack_start (GTK_BOX (vbox1), cond_hbox, TRUE, TRUE, 0);

	cond_scrolledwin = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (cond_scrolledwin);
	gtk_widget_set_usize (cond_scrolledwin, -1, 150);
	gtk_box_pack_start (GTK_BOX (cond_hbox), cond_scrolledwin,
			    TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (cond_scrolledwin),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	title[0] = _("Current filtering/processing rules");
	cond_clist = gtk_clist_new_with_titles(1, title);
	gtk_widget_show (cond_clist);
	gtk_container_add (GTK_CONTAINER (cond_scrolledwin), cond_clist);
	gtk_clist_set_column_width (GTK_CLIST (cond_clist), 0, 80);
	gtk_clist_set_selection_mode (GTK_CLIST (cond_clist),
				      GTK_SELECTION_BROWSE);
	GTK_WIDGET_UNSET_FLAGS (GTK_CLIST (cond_clist)->column[0].button,
				GTK_CAN_FOCUS);
	gtk_signal_connect (GTK_OBJECT (cond_clist), "select_row",
			    GTK_SIGNAL_FUNC (prefs_filtering_select), NULL);

	btn_vbox = gtk_vbox_new (FALSE, 8);
	gtk_widget_show (btn_vbox);
	gtk_box_pack_start (GTK_BOX (cond_hbox), btn_vbox, FALSE, FALSE, 0);

	up_btn = gtk_button_new_with_label (_("Up"));
	gtk_widget_show (up_btn);
	gtk_box_pack_start (GTK_BOX (btn_vbox), up_btn, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (up_btn), "clicked",
			    GTK_SIGNAL_FUNC (prefs_filtering_up), NULL);

	down_btn = gtk_button_new_with_label (_("Down"));
	gtk_widget_show (down_btn);
	gtk_box_pack_start (GTK_BOX (btn_vbox), down_btn, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (down_btn), "clicked",
			    GTK_SIGNAL_FUNC (prefs_filtering_down), NULL);

	gtk_widget_set_usize(window, 500, -1);

	gtk_widget_show_all(window);

	filtering.window    = window;
	filtering.ok_btn = ok_btn;

	filtering.cond_entry = cond_entry;
	filtering.action_list = action_list;
	filtering.action_combo = action_combo;
	filtering.account_label = account_label;
	filtering.account_list = account_list;
	filtering.account_combo = account_combo;
	filtering.dest_entry = dest_entry;
	filtering.dest_btn = dest_btn;
	filtering.dest_label = dest_label;
	filtering.recip_label = recip_label;
	filtering.exec_label = exec_label;
	filtering.exec_btn = exec_btn;

	filtering.cond_clist   = cond_clist;

	filtering.color_label   = color_label;
	filtering.color_optmenu = color_optmenu;
}

static void prefs_filtering_update_hscrollbar(void)
{
	gint optwidth = gtk_clist_optimal_column_width(GTK_CLIST(filtering.cond_clist), 0);
	gtk_clist_set_column_width(GTK_CLIST(filtering.cond_clist), 0, optwidth);
}

void prefs_filtering_rename_path(const gchar *old_path, const gchar *new_path)
{
	GList * cur;
	gchar *paths[2] = {NULL, NULL};
	paths[0] = (gchar*)old_path;
	paths[1] = (gchar*)new_path;
	for (cur = folder_get_list() ; cur != NULL ; cur = g_list_next(cur)) {
		Folder *folder;
		folder = (Folder *) cur->data;
		g_node_traverse(folder->node, G_PRE_ORDER, G_TRAVERSE_ALL, -1,
				prefs_filtering_rename_path_func, paths);
	}
	prefs_filtering_rename_path_func(NULL, paths);
}

static gboolean prefs_filtering_rename_path_func(GNode *node, gpointer data)
{
	GSList *cur;
	gchar *old_path, *new_path;
	gchar *base;
	gchar *prefix;
	gchar *suffix;
	gchar *dest_path;
	gchar *old_path_with_sep;
	gint destlen;
	gint prefixlen;
	gint oldpathlen;
	FolderItem *item;

	old_path = ((gchar **)data)[0];
	new_path = ((gchar **)data)[1];

	g_return_val_if_fail(old_path != NULL, FALSE);
	g_return_val_if_fail(new_path != NULL, FALSE);
#ifdef WIN32
	old_path = g_strdup(old_path);
	new_path = g_strdup(new_path);
	locale_from_utf8(&old_path);
	locale_from_utf8(&new_path);
#endif

	oldpathlen = strlen(old_path);
	old_path_with_sep = g_strconcat(old_path,G_DIR_SEPARATOR_S,NULL);
	if (node == NULL)
		cur = global_processing;
	else {
		item = node->data;
		if (!item || !item->prefs) 
			return FALSE;
		cur = item->prefs->processing;
	}

	for (; cur != NULL; cur = cur->next) {
		FilteringProp   *filtering = (FilteringProp *)cur->data;
		FilteringAction *action = filtering->action;

		if (!action->destination) continue;

		destlen = strlen(action->destination);

		if (destlen > oldpathlen) {
			prefixlen = destlen - oldpathlen;
			suffix = action->destination + prefixlen;

			if (!strncmp(old_path, suffix, oldpathlen)) {
				prefix = g_malloc0(prefixlen + 1);
				strncpy2(prefix, action->destination, prefixlen);

				base = suffix + oldpathlen;
				while (*base == G_DIR_SEPARATOR) base++;
				if (*base == '\0')
					dest_path = g_strconcat(prefix,
								G_DIR_SEPARATOR_S,
								new_path, NULL);
				else
					dest_path = g_strconcat(prefix,
								G_DIR_SEPARATOR_S,
								new_path,
								G_DIR_SEPARATOR_S,
								base, NULL);

				g_free(prefix);
				g_free(action->destination);
				action->destination = dest_path;
			} else { /* for non-leaf folders */
				/* compare with trailing slash */
				if (!strncmp(old_path_with_sep, action->destination, oldpathlen+1)) {
					
					suffix = action->destination + oldpathlen + 1;
					dest_path = g_strconcat(new_path,
								G_DIR_SEPARATOR_S,
								suffix, NULL);
					g_free(action->destination);
					action->destination = dest_path;
				}
			}
		} else {
			/* folder-moving a leaf */
			if (!strcmp(old_path, action->destination)) {		
				dest_path = g_strdup(new_path);
				g_free(action->destination);
				action->destination = dest_path;
			}
		}
	}
#ifdef WIN32
	g_free(old_path);
	g_free(new_path);
#endif
	g_free(old_path_with_sep);
	prefs_matcher_write_config();

	return FALSE;
}

void prefs_filtering_delete_path(const gchar *path)
{
	GList * cur;
	for (cur = folder_get_list() ; cur != NULL ; cur = g_list_next(cur)) {
		Folder *folder;
		folder = (Folder *) cur->data;
		g_node_traverse(folder->node, G_PRE_ORDER, G_TRAVERSE_ALL, -1,
				prefs_filtering_delete_path_func, (gchar *)path);
	}
	prefs_filtering_delete_path_func(NULL, (gchar *)path);
}

static gboolean prefs_filtering_delete_path_func(GNode *node, gpointer data)
{
	GSList *cur, *orig;
	GSList *next;
	gchar *path = (gchar *)data;
	gchar *suffix;
	gint destlen;
	gint prefixlen;
	gint pathlen;
	FolderItem *item;
	
	g_return_val_if_fail(path != NULL, FALSE);

	pathlen = strlen(path);
	if (node == NULL)
		cur = global_processing;
	else {
		item = node->data;
		if (!item || !item->prefs)
			return FALSE;
		cur = item->prefs->processing;
	}
	orig = cur;
	
	for (; cur != NULL; cur = cur->next) {
		FilteringProp *filtering = (FilteringProp *)cur->data;
		FilteringAction *action;
		if (!cur->data)
			break;
		
		action = filtering->action;
		next = cur->next;

		if (!action->destination) continue;

		destlen = strlen(action->destination);

		if (destlen > pathlen) {
			prefixlen = destlen - pathlen;
			suffix = action->destination + prefixlen;

			if (suffix && !strncmp(path, suffix, pathlen)) {
				filteringprop_free(filtering);
				orig = g_slist_remove(orig, filtering);
			}
		} else if (strcmp(action->destination, path) == 0) {
			filteringprop_free(filtering);
			orig = g_slist_remove(orig, filtering);

		}
	}

	if (node == NULL)
		global_processing = orig;
	else {
		item = node->data;
		if (!item || !item->prefs)
			return FALSE;
		item->prefs->processing = orig;
	}

	prefs_matcher_write_config();

	return FALSE;
}

static void prefs_filtering_set_dialog(const gchar *header, const gchar *key)
{
	GtkCList *clist = GTK_CLIST(filtering.cond_clist);
	GSList *cur;
	GSList * prefs_filtering;
	gchar *cond_str[1];
	gint row;
	
	gtk_clist_freeze(clist);
	gtk_clist_clear(clist);

	cond_str[0] = _("(New)");
	row = gtk_clist_append(clist, cond_str);
	gtk_clist_set_row_data(clist, row, NULL);

	if (cur_item == NULL)
		prefs_filtering = global_processing;
	else
		prefs_filtering = cur_item->prefs->processing;

	for(cur = prefs_filtering ; cur != NULL ; cur = g_slist_next(cur)) {
		FilteringProp * prop = (FilteringProp *) cur->data;

		cond_str[0] = filteringprop_to_string(prop);
		subst_char(cond_str[0], '\t', ':');
#ifdef WIN32
		locale_to_utf8(&cond_str[0]);
#endif
		row = gtk_clist_append(clist, cond_str);
		gtk_clist_set_row_data(clist, row, prop);

		g_free(cond_str[0]);
	}

	prefs_filtering_update_hscrollbar();
	gtk_clist_thaw(clist);

	prefs_filtering_reset_dialog();

	if (header && key) {
		gchar *match_str = g_strconcat(header, " ",
			get_matchparser_tab_str(MATCHTYPE_MATCHCASE),
			" \"", key, "\"", NULL);
		gtk_entry_set_text(GTK_ENTRY(filtering.cond_entry), match_str);
		g_free(match_str);
	}
}

static void prefs_filtering_reset_dialog(void)
{
	gtk_list_select_item(GTK_LIST(filtering.action_list), 0);
	gtk_list_select_item(GTK_LIST(filtering.account_list), 0);
	gtk_entry_set_text(GTK_ENTRY(filtering.dest_entry), "");
	gtk_entry_set_text(GTK_ENTRY(filtering.cond_entry), "");
}

static void prefs_filtering_set_list(void)
{
	gint row = 1;
	FilteringProp *prop;
	GSList * cur;
	gchar * filtering_str;
	GSList * prefs_filtering;

	if (cur_item == NULL)
		prefs_filtering = global_processing;
	else
		prefs_filtering = cur_item->prefs->processing;

	for(cur = prefs_filtering ; cur != NULL ; cur = g_slist_next(cur))
		filteringprop_free((FilteringProp *) cur->data);
	g_slist_free(prefs_filtering);
	prefs_filtering = NULL;

	while (gtk_clist_get_text(GTK_CLIST(filtering.cond_clist),
				  row, 0, &filtering_str)) {
#ifdef WIN32
		filtering_str = g_strdup(filtering_str);
		locale_from_utf8(&filtering_str);
#endif
		if (strcmp(filtering_str, _("(New)")) != 0) {
			/* tmp = filtering_str; */
			prop = matcher_parser_get_filtering(filtering_str);
			if (prop != NULL)
				prefs_filtering =
					g_slist_append(prefs_filtering, prop);
		}
		row++;
#ifdef WIN32
		g_free(filtering_str);
#endif
	}

	if (cur_item == NULL)
		global_processing = prefs_filtering;
	else
		cur_item->prefs->processing = prefs_filtering;
}

static gint prefs_filtering_clist_set_row(gint row, FilteringProp * prop)
{
	GtkCList *clist = GTK_CLIST(filtering.cond_clist);
	gchar * str;
	gchar *cond_str[1];

	if (prop == NULL) {
		cond_str[0] = _("(New)");
		return gtk_clist_append(clist, cond_str);
	}

	str = filteringprop_to_string(prop);
	if (str == NULL) {
		return -1;
	}
#ifdef WIN32
	locale_to_utf8(&str);
#endif
	cond_str[0] = str;

	if (row < 0)
		row = gtk_clist_append(clist, cond_str);
	else
		gtk_clist_set_text(clist, row, 0, cond_str[0]);
	g_free(str);

	return row;
}

static void prefs_filtering_condition_define_done(MatcherList * matchers)
{
	gchar * str;

	if (matchers == NULL)
		return;

	str = matcherlist_to_string(matchers);

	if (str != NULL) {
		gtk_entry_set_text(GTK_ENTRY(filtering.cond_entry), str);
		g_free(str);
	}
}

static void prefs_filtering_condition_define(void)
{
	gchar * cond_str;
	MatcherList * matchers = NULL;

	cond_str = gtk_entry_get_text(GTK_ENTRY(filtering.cond_entry));

	if (*cond_str != '\0') {
		matchers = matcher_parser_get_cond(cond_str);
		if (matchers == NULL)
			alertpanel_error(_("Condition string is not valid."));
	}

	prefs_matcher_open(matchers, prefs_filtering_condition_define_done);

	if (matchers != NULL)
		matcherlist_free(matchers);
}


/* register / substitute delete buttons */


static FilteringProp * prefs_filtering_dialog_to_filtering(gboolean alert)
{
	MatcherList * cond;
	gchar * cond_str;
	FilteringProp * prop;
	FilteringAction * action;
	gint list_id;
	Action action_id;
	gint action_type;
	gint account_id;
	gchar * destination;
	gint labelcolor = 0;
	
	cond_str = gtk_entry_get_text(GTK_ENTRY(filtering.cond_entry));
	if (*cond_str == '\0') {
		if(alert == TRUE) alertpanel_error(_("Condition string is empty."));
		return NULL;
	}

	action_id = get_sel_from_list(GTK_LIST(filtering.action_list));
	action_type = prefs_filtering_get_matching_from_action(action_id);
	list_id = get_sel_from_list(GTK_LIST(filtering.account_list));
	account_id = get_account_id_from_list_id(list_id);

	switch (action_id) {
	case ACTION_MOVE:
	case ACTION_COPY:
	case ACTION_EXECUTE:
		destination = gtk_entry_get_text(GTK_ENTRY(filtering.dest_entry));
		if (*destination == '\0') {
			if(alert == TRUE) alertpanel_error(_("Destination is not set."));
			return NULL;
		}
		break;
	case ACTION_FORWARD:
	case ACTION_FORWARD_AS_ATTACHMENT:
	case ACTION_REDIRECT:
		destination = gtk_entry_get_text(GTK_ENTRY(filtering.dest_entry));
		if (*destination == '\0') {
			if(alert == TRUE) alertpanel_error(_("Recipient is not set."));
			return NULL;
		}
		break;
	case ACTION_COLOR:
		labelcolor = colorlabel_get_color_menu_active_item(
			gtk_option_menu_get_menu(GTK_OPTION_MENU(filtering.color_optmenu)));
		destination = NULL;	
		break;
	default:
		destination = NULL;
		break;
	}
	
#ifdef WIN32
	if (destination) {
		destination = g_strdup(destination);
		locale_from_utf8(&destination);
	}
#endif
	action = filteringaction_new(action_type, account_id, destination, labelcolor);

	cond = matcher_parser_get_cond(cond_str);

	if (cond == NULL) {
		if(alert == TRUE) alertpanel_error(_("Condition string is not valid."));
		filteringaction_free(action);
		return NULL;
	}

	prop = filteringprop_new(cond, action);
#ifdef WIN32
	if (destination) g_free(destination);
#endif

	return prop;
}

static void prefs_filtering_register_cb(void)
{
	FilteringProp * prop;
	
	prop = prefs_filtering_dialog_to_filtering(TRUE);
	if (prop == NULL)
		return;
	prefs_filtering_clist_set_row(-1, prop);

	filteringprop_free(prop);
	
	prefs_filtering_update_hscrollbar();
}

static void prefs_filtering_substitute_cb(void)
{
	GtkCList *clist = GTK_CLIST(filtering.cond_clist);
	gint row;
	FilteringProp * prop;
	
	if (!clist->selection) return;

	row = GPOINTER_TO_INT(clist->selection->data);
	if (row == 0) return;

	prop = prefs_filtering_dialog_to_filtering(TRUE);
	if (prop == NULL)
		return;
	prefs_filtering_clist_set_row(row, prop);

	filteringprop_free(prop);
	
	prefs_filtering_update_hscrollbar();
}

static void prefs_filtering_delete_cb(void)
{
	GtkCList *clist = GTK_CLIST(filtering.cond_clist);
	gint row;

	if (!clist->selection) return;
	row = GPOINTER_TO_INT(clist->selection->data);
	if (row == 0) return;

	if (alertpanel(_("Delete rule"),
		       _("Do you really want to delete this rule?"),
		       _("Yes"), _("No"), NULL) == G_ALERTALTERNATE)
		return;

	gtk_clist_remove(clist, row);

	prefs_filtering_reset_dialog();

	prefs_filtering_update_hscrollbar();
}

static void prefs_filtering_up(void)
{
	GtkCList *clist = GTK_CLIST(filtering.cond_clist);
	gint row;

	if (!clist->selection) return;

	row = GPOINTER_TO_INT(clist->selection->data);
	if (row > 1) {
		gtk_clist_row_move(clist, row, row - 1);
		if(gtk_clist_row_is_visible(clist, row - 1) != GTK_VISIBILITY_FULL) {
			gtk_clist_moveto(clist, row - 1, 0, 0, 0);
		} 
	}
}

static void prefs_filtering_down(void)
{
	GtkCList *clist = GTK_CLIST(filtering.cond_clist);
	gint row;

	if (!clist->selection) return;

	row = GPOINTER_TO_INT(clist->selection->data);
	if (row > 0 && row < clist->rows - 1) {
		gtk_clist_row_move(clist, row, row + 1);
		if(gtk_clist_row_is_visible(clist, row + 1) != GTK_VISIBILITY_FULL) {
			gtk_clist_moveto(clist, row + 1, 0, 1, 0);
		} 
	}
}

static void prefs_filtering_select_set(FilteringProp *prop)
{
	FilteringAction *action;
	gchar *matcher_str;
	gint list_id;

	prefs_filtering_reset_dialog();

	action = prop->action;

	matcher_str = matcherlist_to_string(prop->matchers);
	if (matcher_str == NULL) {
		filteringprop_free(prop);
		return;
	}

	gtk_entry_set_text(GTK_ENTRY(filtering.cond_entry), matcher_str);
	
	if (action->destination)
		gtk_entry_set_text(GTK_ENTRY(filtering.dest_entry), action->destination);
	else
		gtk_entry_set_text(GTK_ENTRY(filtering.dest_entry), "");

	switch(action->type) {
	case MATCHACTION_MOVE:
		gtk_list_select_item(GTK_LIST(filtering.action_list),
				     ACTION_MOVE);
		break;
	case MATCHACTION_COPY:
		gtk_list_select_item(GTK_LIST(filtering.action_list),
				     ACTION_COPY);
		break;
	case MATCHACTION_DELETE:
		gtk_list_select_item(GTK_LIST(filtering.action_list),
				     ACTION_DELETE);
		break;
	case MATCHACTION_MARK:
		gtk_list_select_item(GTK_LIST(filtering.action_list),
				     ACTION_MARK);
		break;
	case MATCHACTION_UNMARK:
		gtk_list_select_item(GTK_LIST(filtering.action_list),
				     ACTION_UNMARK);
		break;
	case MATCHACTION_LOCK:
		gtk_list_select_item(GTK_LIST(filtering.action_list),
				     ACTION_LOCK);
		break;
	case MATCHACTION_UNLOCK:
		gtk_list_select_item(GTK_LIST(filtering.action_list),
				     ACTION_UNLOCK);
		break;
	case MATCHACTION_MARK_AS_READ:
		gtk_list_select_item(GTK_LIST(filtering.action_list),
				     ACTION_MARK_AS_READ);
		break;
	case MATCHACTION_MARK_AS_UNREAD:
		gtk_list_select_item(GTK_LIST(filtering.action_list),
				     ACTION_MARK_AS_UNREAD);
		break;
	case MATCHACTION_FORWARD:
		list_id = get_list_id_from_account_id(action->account_id);
		gtk_list_select_item(GTK_LIST(filtering.action_list),
				     ACTION_FORWARD);
		gtk_list_select_item(GTK_LIST(filtering.account_list),
				     list_id);
		break;
	case MATCHACTION_FORWARD_AS_ATTACHMENT:
		list_id = get_list_id_from_account_id(action->account_id);
		gtk_list_select_item(GTK_LIST(filtering.action_list),
				     ACTION_FORWARD_AS_ATTACHMENT);
		gtk_list_select_item(GTK_LIST(filtering.account_list),
				     list_id);
		break;
	case MATCHACTION_REDIRECT:
		list_id = get_list_id_from_account_id(action->account_id);
		gtk_list_select_item(GTK_LIST(filtering.action_list),
				     ACTION_REDIRECT);
		gtk_list_select_item(GTK_LIST(filtering.account_list),
				     list_id);
		break;
	case MATCHACTION_EXECUTE:
		gtk_list_select_item(GTK_LIST(filtering.action_list),
				     ACTION_EXECUTE);
		break;
	case MATCHACTION_COLOR:
		gtk_list_select_item(GTK_LIST(filtering.action_list),
				     ACTION_COLOR);
		gtk_option_menu_set_history(GTK_OPTION_MENU(filtering.color_optmenu), action->labelcolor);     
		break;
	}

	g_free(matcher_str);
}

static void prefs_filtering_select(GtkCList *clist, gint row, gint column,
				GdkEvent *event)
{
	FilteringProp * prop;
	gchar * filtering_str;

	if (row == 0) {
		prefs_filtering_reset_dialog();
		return;
	}

        if (!gtk_clist_get_text(GTK_CLIST(filtering.cond_clist),
				row, 0, &filtering_str))
		return;
	
	prop = matcher_parser_get_filtering(filtering_str);
	if (prop == NULL)
		return;

	prefs_filtering_select_set(prop);

	filteringprop_free(prop);
}

static void prefs_filtering_select_dest(void)
{
	FolderItem *dest;
	gchar * path;

	dest = foldersel_folder_sel(NULL, FOLDER_SEL_COPY, NULL);
	if (!dest) return;

	path = folder_item_get_identifier(dest);

	gtk_entry_set_text(GTK_ENTRY(filtering.dest_entry), path);
	g_free(path);
}

static void prefs_filtering_action_selection_changed(GtkList *list,
						     gpointer user_data)
{
	gint value;

	value = get_sel_from_list(GTK_LIST(filtering.action_list));

	if (filtering.current_action != value) {
		if (filtering.current_action == ACTION_FORWARD 
		||  filtering.current_action == ACTION_FORWARD_AS_ATTACHMENT
		||  filtering.current_action == ACTION_REDIRECT) {
			debug_print("unregistering address completion entry\n");
			address_completion_unregister_entry(GTK_ENTRY(filtering.dest_entry));
		}
		if (value == ACTION_FORWARD || value == ACTION_FORWARD_AS_ATTACHMENT
		||  value == ACTION_REDIRECT) {
			debug_print("registering address completion entry\n");
			address_completion_register_entry(GTK_ENTRY(filtering.dest_entry));
		}
		filtering.current_action = value;
	}
}

static void prefs_filtering_action_select(GtkList *list,
					  GtkWidget *widget,
					  gpointer user_data)
{
	Action value;

	value = (Action) get_sel_from_list(GTK_LIST(filtering.action_list));

	switch (value) {
	case ACTION_MOVE:
		gtk_widget_show(filtering.account_label);
		gtk_widget_set_sensitive(filtering.account_label, FALSE);
		gtk_widget_set_sensitive(filtering.account_combo, FALSE);
		gtk_widget_show(filtering.dest_entry);
		gtk_widget_set_sensitive(filtering.dest_entry, TRUE);
		gtk_widget_show(filtering.dest_btn);
		gtk_widget_set_sensitive(filtering.dest_btn, TRUE);
		gtk_widget_show(filtering.dest_label);
		gtk_widget_set_sensitive(filtering.dest_label, TRUE);
		gtk_widget_hide(filtering.recip_label);
		gtk_widget_hide(filtering.exec_label);
		gtk_widget_hide(filtering.exec_btn);
		gtk_widget_hide(filtering.color_optmenu);
		gtk_widget_hide(filtering.color_label);
		break;
	case ACTION_COPY:
		gtk_widget_show(filtering.account_label);
		gtk_widget_set_sensitive(filtering.account_label, FALSE);
		gtk_widget_set_sensitive(filtering.account_combo, FALSE);
		gtk_widget_show(filtering.dest_entry);
		gtk_widget_set_sensitive(filtering.dest_entry, TRUE);
		gtk_widget_show(filtering.dest_btn);
		gtk_widget_set_sensitive(filtering.dest_btn, TRUE);
		gtk_widget_show(filtering.dest_label);
		gtk_widget_set_sensitive(filtering.dest_label, TRUE);
		gtk_widget_hide(filtering.recip_label);
		gtk_widget_hide(filtering.exec_label);
		gtk_widget_hide(filtering.exec_btn);
		gtk_widget_hide(filtering.color_optmenu);
		gtk_widget_hide(filtering.color_label);
		break;
	case ACTION_DELETE:
		gtk_widget_show(filtering.account_label);
		gtk_widget_set_sensitive(filtering.account_label, FALSE);
		gtk_widget_set_sensitive(filtering.account_combo, FALSE);
		gtk_widget_show(filtering.dest_entry);
		gtk_widget_set_sensitive(filtering.dest_entry, FALSE);
		gtk_widget_show(filtering.dest_btn);
		gtk_widget_set_sensitive(filtering.dest_btn, FALSE);
		gtk_widget_show(filtering.dest_label);
		gtk_widget_set_sensitive(filtering.dest_label, FALSE);
		gtk_widget_hide(filtering.recip_label);
		gtk_widget_hide(filtering.exec_label);
		gtk_widget_hide(filtering.exec_btn);
		gtk_widget_hide(filtering.color_optmenu);
		gtk_widget_hide(filtering.color_label);
		break;
	case ACTION_MARK:
	case ACTION_UNMARK:
	case ACTION_LOCK:
	case ACTION_UNLOCK:
	case ACTION_MARK_AS_READ:
	case ACTION_MARK_AS_UNREAD:
		gtk_widget_show(filtering.account_label);
		gtk_widget_set_sensitive(filtering.account_label, FALSE);
		gtk_widget_set_sensitive(filtering.account_combo, FALSE);
		gtk_widget_show(filtering.dest_entry);
		gtk_widget_set_sensitive(filtering.dest_entry, FALSE);
		gtk_widget_show(filtering.dest_btn);
		gtk_widget_set_sensitive(filtering.dest_btn, FALSE);
		gtk_widget_show(filtering.dest_label);
		gtk_widget_set_sensitive(filtering.dest_label, FALSE);
		gtk_widget_hide(filtering.recip_label);
		gtk_widget_hide(filtering.exec_label);
		gtk_widget_hide(filtering.exec_btn);
		gtk_widget_hide(filtering.color_optmenu);
		gtk_widget_hide(filtering.color_label);
		break;
	case ACTION_FORWARD:
		gtk_widget_show(filtering.account_label);
		gtk_widget_set_sensitive(filtering.account_label, TRUE);
		gtk_widget_set_sensitive(filtering.account_combo, TRUE);
		gtk_widget_show(filtering.dest_entry);
		gtk_widget_set_sensitive(filtering.dest_entry, TRUE);
		gtk_widget_show(filtering.dest_btn);
		gtk_widget_set_sensitive(filtering.dest_btn, FALSE);
		gtk_widget_hide(filtering.dest_label);
		gtk_widget_show(filtering.recip_label);
		gtk_widget_set_sensitive(filtering.recip_label, TRUE);
		gtk_widget_hide(filtering.exec_label);
		gtk_widget_hide(filtering.exec_btn);
		gtk_widget_hide(filtering.color_optmenu);
		gtk_widget_hide(filtering.color_label);
		break;
	case ACTION_FORWARD_AS_ATTACHMENT:
		gtk_widget_show(filtering.account_label);
		gtk_widget_set_sensitive(filtering.account_label, TRUE);
		gtk_widget_set_sensitive(filtering.account_combo, TRUE);
		gtk_widget_show(filtering.dest_entry);
		gtk_widget_set_sensitive(filtering.dest_entry, TRUE);
		gtk_widget_show(filtering.dest_btn);
		gtk_widget_set_sensitive(filtering.dest_btn, FALSE);
		gtk_widget_hide(filtering.dest_label);
		gtk_widget_show(filtering.recip_label);
		gtk_widget_set_sensitive(filtering.recip_label, TRUE);
		gtk_widget_hide(filtering.exec_label);
		gtk_widget_hide(filtering.exec_btn);
		gtk_widget_hide(filtering.color_optmenu);
		gtk_widget_hide(filtering.color_label);
		break;
	case ACTION_REDIRECT:
		gtk_widget_show(filtering.account_label);
		gtk_widget_set_sensitive(filtering.account_label, TRUE);
		gtk_widget_set_sensitive(filtering.account_combo, TRUE);
		gtk_widget_show(filtering.dest_entry);
		gtk_widget_set_sensitive(filtering.dest_entry, TRUE);
		gtk_widget_show(filtering.dest_btn);
		gtk_widget_set_sensitive(filtering.dest_btn, FALSE);
		gtk_widget_hide(filtering.dest_label);
		gtk_widget_show(filtering.recip_label);
		gtk_widget_set_sensitive(filtering.recip_label, TRUE);
		gtk_widget_hide(filtering.exec_label);
		gtk_widget_hide(filtering.exec_btn);
		gtk_widget_hide(filtering.color_optmenu);
		gtk_widget_hide(filtering.color_label);
		break;
	case ACTION_EXECUTE:
		gtk_widget_show(filtering.account_label);
		gtk_widget_set_sensitive(filtering.account_label, FALSE);
		gtk_widget_set_sensitive(filtering.account_combo, FALSE);
		gtk_widget_show(filtering.dest_entry);
		gtk_widget_set_sensitive(filtering.dest_entry, TRUE);
		gtk_widget_hide(filtering.dest_btn);
		gtk_widget_hide(filtering.dest_label);
		gtk_widget_hide(filtering.recip_label);
		gtk_widget_show(filtering.exec_label);
		gtk_widget_set_sensitive(filtering.exec_btn, TRUE);
		gtk_widget_show(filtering.exec_btn);
		gtk_widget_hide(filtering.color_optmenu);
		gtk_widget_hide(filtering.color_label);
		break;
	case ACTION_COLOR:
		gtk_widget_show(filtering.account_label);
		gtk_widget_set_sensitive(filtering.account_label, FALSE);
		gtk_widget_set_sensitive(filtering.account_combo, FALSE);
		gtk_widget_hide(filtering.dest_entry);
		gtk_widget_hide(filtering.dest_btn);
		gtk_widget_hide(filtering.dest_label);
		gtk_widget_hide(filtering.recip_label);
		gtk_widget_hide(filtering.exec_label);
		gtk_widget_show(filtering.exec_btn);
		gtk_widget_set_sensitive(filtering.exec_btn, FALSE);
		gtk_widget_show(filtering.color_optmenu);
		gtk_widget_show(filtering.color_label);
		break;
	}
}

static gint prefs_filtering_deleted(GtkWidget *widget, GdkEventAny *event,
				 gpointer data)
{
	prefs_filtering_cancel();
	return TRUE;
}

static void prefs_filtering_key_pressed(GtkWidget *widget, GdkEventKey *event,
				     gpointer data)
{
	if (event && event->keyval == GDK_Escape)
		prefs_filtering_cancel();
}

static void prefs_filtering_ok(void)
{
	FilteringProp * prop;
	gchar * str;
	gchar * filtering_str;
	gint row = 1;
        AlertValue val;
	
	prop = prefs_filtering_dialog_to_filtering(FALSE);
	if (prop != NULL) {
		str = filteringprop_to_string(prop);

		while (gtk_clist_get_text(GTK_CLIST(filtering.cond_clist),
					  row, 0, &filtering_str)) {
#ifdef WIN32
			filtering_str = g_strdup(filtering_str);
			locale_from_utf8(&filtering_str);
#endif
			if (strcmp(filtering_str, str) == 0) break;
#ifdef WIN32
			g_free(filtering_str);
#endif
			row++;
		}
		if (strcmp(filtering_str, str) != 0) {
			val = alertpanel(_("Entry not saved"),
				 _("The entry was not saved. Close anyway?"),
				 _("Yes"), _("No"), NULL);
			if (G_ALERTDEFAULT != val) {
				g_free(str);
				return;
			}
		}
#ifdef WIN32
		g_free(filtering_str);
#endif
		g_free(str);
	}
	prefs_filtering_set_list();
	prefs_matcher_write_config();
	prefs_filtering_close();
}

static void prefs_filtering_cancel(void)
{
	prefs_matcher_read_config();
	prefs_filtering_close();
}
