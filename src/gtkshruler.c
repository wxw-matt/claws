/* GtkSHRuler
 *
 *  Copyright (C) 2000 Alfons Hoogervorst
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
/* I derived this class from hruler. S in HRuler could be read as
 * Sylpheed (sylpheed.good-day.net), but also [S]ettable HRuler.
 * I basically ripped apart the draw_ticks member of HRuler; it
 * now draws the ticks at ruler->max_size. so gtk_ruler_set_range's
 * last parameter has the distance between two ticks (which is
 * the width of the fixed font character!
 * 
 * -- Alfons (alfons@proteus.demon.nl)
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtkhruler.h>
#include "gtkshruler.h"

#define RULER_HEIGHT          14
#define MINIMUM_INCR          5
#define MAXIMUM_SUBDIVIDE     5
#define MAXIMUM_SCALES        10

#define ROUND(x) ((int) ((x) + 0.5))

static void gtk_shruler_class_init   	(GtkSHRulerClass *klass);
static void gtk_shruler_init         	(GtkSHRuler      *hruler);
static void gtk_shruler_draw_ticks 	(GtkRuler        *ruler);
#if 0
static void gtk_shruler_draw_pos      	(GtkRuler        *ruler);
#endif

guint
gtk_shruler_get_type(void)
{
	static guint shruler_type = 0;

  	if ( !shruler_type ) {
   		static const GtkTypeInfo shruler_info = {
			"GtkSHRuler",
			sizeof (GtkSHRuler),
			sizeof (GtkSHRulerClass),
			(GtkClassInitFunc) gtk_shruler_class_init,
			(GtkObjectInitFunc) gtk_shruler_init,
			/* reserved_1 */ NULL,
        	/* reserved_2 */ NULL,
        	(GtkClassInitFunc) NULL,
     	 };
		/* inherit from GtkHRuler */
      	shruler_type = gtk_type_unique( gtk_hruler_get_type (), &shruler_info );
	}
	return shruler_type;
}

static void
gtk_shruler_class_init(GtkSHRulerClass * klass)
{
 	GtkWidgetClass * widget_class;
  	GtkRulerClass * hruler_class;

  	widget_class = (GtkWidgetClass*) klass;
  	hruler_class = (GtkRulerClass*) klass;

	/* just neglect motion notify events */
  	widget_class->motion_notify_event = NULL /* gtk_shruler_motion_notify */;

  	/* we want the old ruler draw ticks... */
  	/* ruler_class->draw_ticks = gtk_hruler_draw_ticks; */
	hruler_class->draw_ticks = gtk_shruler_draw_ticks;
	
	/* unimplemented draw pos */
	hruler_class->draw_pos = NULL;
/*
  	hruler_class->draw_pos = gtk_shruler_draw_pos;
*/
}

static void
gtk_shruler_init (GtkSHRuler * shruler)
{
	GtkWidget * widget;
	
	widget = GTK_WIDGET (shruler);
	widget->requisition.width = widget->style->klass->xthickness * 2 + 1;
	widget->requisition.height = widget->style->klass->ythickness * 2 + RULER_HEIGHT;
}


GtkWidget*
gtk_shruler_new(void)
{
	return GTK_WIDGET( gtk_type_new( gtk_shruler_get_type() ) );
}

static void
gtk_shruler_draw_ticks(GtkRuler *ruler)
{
	GtkWidget *widget;
	GdkGC *gc, *bg_gc;
	GdkFont *font;
	gint i;
	gint width, height;
	gint xthickness;
	gint ythickness;
	gint pos;

	g_return_if_fail (ruler != NULL);
	g_return_if_fail (GTK_IS_HRULER (ruler));

	if (!GTK_WIDGET_DRAWABLE (ruler)) 
		return;

	widget = GTK_WIDGET (ruler);
	
	gc = widget->style->fg_gc[GTK_STATE_NORMAL];
	bg_gc = widget->style->bg_gc[GTK_STATE_NORMAL];
	font = widget->style->font;

	xthickness = widget->style->klass->xthickness;
	ythickness = widget->style->klass->ythickness;

	width = widget->allocation.width;
	height = widget->allocation.height - ythickness * 2;
  
	gtk_paint_box (widget->style, ruler->backing_store,
		       GTK_STATE_NORMAL, GTK_SHADOW_OUT, 
		       NULL, widget, "hruler",
		       0, 0, 
		       widget->allocation.width, widget->allocation.height);

#if 0
	gdk_draw_line (ruler->backing_store, gc,
		       xthickness,
		       height + ythickness,
		       widget->allocation.width - xthickness,
		       height + ythickness);
#endif

	/* assume ruler->max_size has the char width */    
	/* i is increment of char_width,  pos is label number */
	for ( i = 0, pos = 0; i < widget->allocation.width - xthickness; i += ruler->max_size, pos++ ) {	
		int length = ythickness / 2;
	
		if ( pos % 10 == 0 ) length = ( 4 * ythickness );
		else if (pos % 5 == 0 ) length = ( 2 * ythickness );
		
		gdk_draw_line(ruler->backing_store, gc,
			      i, height + ythickness, 
			      i, height - length);			
		
		if ( pos % 10 == 0 ) {
			char buf[8];
			/* draw label */
			sprintf(buf, "%d", (int) pos);
			gdk_draw_string(ruler->backing_store, font, gc,
					i + 2, ythickness + font->ascent - 1,
					buf);
	    }
	}
}

/* gtk_ruler_set_pos() - does not work yet, need to reimplement 
 * gtk_ruler_draw_pos(). */
void
gtk_shruler_set_pos(GtkSHRuler * ruler, gfloat pos)
{
	GtkRuler * ruler_;
	g_return_if_fail( ruler != NULL );
	
	ruler_ = GTK_RULER(ruler);
	
	if ( pos < ruler_->lower ) 
		pos = ruler_->lower;
	if ( pos > ruler_->upper )
		pos = ruler_->upper;
	
	ruler_->position = pos;	
	
	/*  Make sure the ruler has been allocated already  */
	if ( ruler_->backing_store != NULL )
		gtk_ruler_draw_pos(ruler_);
}
