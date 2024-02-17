/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* Metacity tile-preview marks the area a window will *ehm* snap to */

/*
 * Copyright (C) 2010 Florian Müllner
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <gtk/gtk.h>
#include <cairo.h>

#include "tile-preview.h"
#include "core.h"

#define OUTLINE_WIDTH 4  /* frame width in non-composite case */


struct _MetaTilePreview {
  GtkWidget     *preview_window;

  GdkRGBA       *preview_color;

  MetaRectangle  tile_rect;

  gboolean       has_alpha: 1;
};

static gboolean
meta_tile_preview_draw (GtkWidget *widget,
                        cairo_t   *cr,
                        gpointer   user_data)
{
  MetaTilePreview *preview = user_data;

  cairo_set_line_width (cr, 1.0);

  if (preview->has_alpha)
    {
      GdkRGBA preview_color = *preview->preview_color;

      /* Fill the preview area with a transparent color */
      gdk_cairo_set_source_rgba (cr, &preview_color);

      cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
      cairo_paint (cr);

      /* Use the opaque color for the border */
      preview_color.alpha = 1.0;
      gdk_cairo_set_source_rgba (cr, &preview_color);
    }
  else
    {
      GdkRGBA clr = {0.17255, 0.65490, 0.97255, 1.0};

      gdk_cairo_set_source_rgba (cr, &clr);

      cairo_rectangle (cr,
                       OUTLINE_WIDTH - 0.5, OUTLINE_WIDTH - 0.5,
                       preview->tile_rect.width - 2 * (OUTLINE_WIDTH - 1) - 1,
                       preview->tile_rect.height - 2 * (OUTLINE_WIDTH - 1) - 1);
      cairo_stroke (cr);
    }

  cairo_rectangle (cr,
                   0.5, 0.5,
                   preview->tile_rect.width - 1,
                   preview->tile_rect.height - 1);
  cairo_stroke (cr);

  return FALSE;
}

static void
meta_tile_preview_style_updated (GtkWidget *widget,
                                 gpointer   user_data)
{
  MetaTilePreview *preview = user_data;
  GtkStyleContext *context = gtk_style_context_new ();
  GtkWidgetPath *path = gtk_widget_path_new ();
  guchar alpha = 0xFF;

  gtk_widget_path_append_type (path, GTK_TYPE_ICON_VIEW);
  gtk_style_context_set_path (context, path);

  gtk_style_context_get (context, GTK_STATE_FLAG_SELECTED, "background-color", &preview->preview_color, NULL);
  gtk_style_context_get_style (context, "selection-box-alpha", &alpha, NULL);

  preview->preview_color->alpha = (double)alpha / 0xFF;

  gtk_widget_path_free (path);
  g_object_unref (context);
}

static void
meta_tile_preview_composited_changed (GtkWidget *widget,
                                      gpointer   user_data)
{
  MetaTilePreview *preview = (MetaTilePreview *)user_data;
  GdkScreen *screen = gdk_screen_get_default ();
  GdkVisual *visual = gdk_screen_get_rgba_visual (screen);

  preview->has_alpha = visual && gtk_widget_is_composited (widget);
}

MetaTilePreview *
meta_tile_preview_new (int      screen_number,
                       gboolean composited)
{
  MetaTilePreview *preview;
  GdkVisual *visual;
  GdkScreen *screen;

  screen = gdk_display_get_screen (gdk_display_get_default (), screen_number);
  visual = gdk_screen_get_rgba_visual (screen);

  preview = g_new (MetaTilePreview, 1);

  preview->preview_window = gtk_window_new (GTK_WINDOW_POPUP);

  gtk_window_set_screen (GTK_WINDOW (preview->preview_window), screen);
  gtk_widget_set_app_paintable (preview->preview_window, TRUE);

  preview->preview_color = NULL;

  preview->tile_rect.x = preview->tile_rect.y = 0;
  preview->tile_rect.width = preview->tile_rect.height = 0;

  preview->has_alpha = visual && composited;

  if (preview->has_alpha)
    {
      gtk_widget_set_visual (preview->preview_window, visual);

      g_signal_connect (preview->preview_window, "style-updated",
                        G_CALLBACK (meta_tile_preview_style_updated), preview);
      g_signal_connect (preview->preview_window, "composited-changed",
                        G_CALLBACK (meta_tile_preview_composited_changed), preview);
      meta_tile_preview_style_updated (preview->preview_window, preview);
   }

  gtk_widget_realize (preview->preview_window);

  g_signal_connect (preview->preview_window, "draw",
                    G_CALLBACK (meta_tile_preview_draw), preview);

  return preview;
}

void
meta_tile_preview_free (MetaTilePreview *preview)
{
  gtk_widget_destroy (preview->preview_window);

  if (preview->preview_color)
    gdk_rgba_free (preview->preview_color);

  g_free (preview);
}

void
meta_tile_preview_show (MetaTilePreview *preview,
                        MetaRectangle   *tile_rect)
{
  GdkWindow *window;
  GdkRectangle old_rect;

  if (gtk_widget_get_visible (preview->preview_window)
      && preview->tile_rect.x == tile_rect->x
      && preview->tile_rect.y == tile_rect->y
      && preview->tile_rect.width == tile_rect->width
      && preview->tile_rect.height == tile_rect->height)
    return; /* nothing to do */

  gtk_widget_show (preview->preview_window);
  window = gtk_widget_get_window (preview->preview_window);
  meta_core_lower_beneath_focus_window (gdk_x11_get_default_xdisplay(),
                                        GDK_WINDOW_XID (window),
                                        gtk_get_current_event_time ());

  old_rect.x = old_rect.y = 0;
  old_rect.width = preview->tile_rect.width;
  old_rect.height = preview->tile_rect.height;

  gdk_window_invalidate_rect (window, &old_rect, FALSE);

  preview->tile_rect = *tile_rect;

  gdk_window_move_resize (window,
                          preview->tile_rect.x, preview->tile_rect.y,
                          preview->tile_rect.width, preview->tile_rect.height);

  if (!preview->has_alpha)
    {
      cairo_rectangle_int_t outer_rect, inner_rect;
      cairo_region_t *outer_region, *inner_region;
      GdkRGBA clr = {0.17255, 0.65490, 0.97255, 1.0};

      gdk_window_set_background_rgba (window, &clr);

      outer_rect.x = outer_rect.y = 0;
      outer_rect.width = preview->tile_rect.width;
      outer_rect.height = preview->tile_rect.height;

      inner_rect.x = OUTLINE_WIDTH;
      inner_rect.y = OUTLINE_WIDTH;
      inner_rect.width = outer_rect.width - 2 * OUTLINE_WIDTH;
      inner_rect.height = outer_rect.height - 2 * OUTLINE_WIDTH;

      outer_region = cairo_region_create_rectangle (&outer_rect);
      inner_region = cairo_region_create_rectangle (&inner_rect);

      cairo_region_subtract (outer_region, inner_region);
      cairo_region_destroy (inner_region);

      gtk_widget_shape_combine_region (preview->preview_window, outer_region);
      cairo_region_destroy (outer_region);
    }
  else
    {
      gtk_widget_shape_combine_region (preview->preview_window, NULL);
    }
}

void
meta_tile_preview_hide (MetaTilePreview *preview)
{
  gtk_widget_hide (preview->preview_window);
}
