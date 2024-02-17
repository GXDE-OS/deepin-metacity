/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/**
 * \file screen-private.h  Screens which Metacity manages
 *
 * Managing X screens.
 * This file contains methods on this class which are available to
 * routines in core but not outside it.  (See screen.h for the routines
 * which the rest of the world is allowed to use.)
 */

/*
 * Copyright (C) 2001 Havoc Pennington
 * Copyright (C) 2003 Rob Adams
 * Copyright (C) 2004-2006 Elijah Newren
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

#ifndef META_SCREEN_PRIVATE_H
#define META_SCREEN_PRIVATE_H

#include "display-private.h"
#include "screen.h"
#include <X11/Xutil.h>
#include "ui.h"

typedef struct _MetaXineramaScreenInfo MetaXineramaScreenInfo;
typedef struct _DeepinWMBackground DeepinWMBackground;
typedef struct _DeepinDesktopBackground DeepinDesktopBackground;

struct _MetaXineramaScreenInfo
{
  int number;
  MetaRectangle rect;
};

typedef void (* MetaScreenWindowFunc) (MetaScreen *screen, MetaWindow *window,
                                       gpointer user_data);

typedef enum
{
  META_SCREEN_TOPLEFT,
  META_SCREEN_TOPRIGHT,
  META_SCREEN_BOTTOMLEFT,
  META_SCREEN_BOTTOMRIGHT
} MetaScreenCorner;

typedef enum
{
  META_SCREEN_UP,
  META_SCREEN_DOWN,
  META_SCREEN_LEFT,
  META_SCREEN_RIGHT
} MetaScreenDirection;

typedef enum
{
    CORNER_UPDATE_NONE  = 0x00,
    CORNER_UPDATE_POS   = 0x01,
    CORNER_UPDATE_STACK = 0x02,
} CornerUpdateMask;

#define META_WIREFRAME_XOR_LINE_WIDTH 2

struct _MetaScreen
{
  MetaDisplay *display;
  int number;
  char *screen_name;
  Screen *xscreen;
  Window xroot;
  int default_depth;
  Visual *default_xvisual;
  MetaRectangle rect;  /* Size of screen; rect.x & rect.y are always 0 */
  MetaUI *ui;
  DeepinTabPopup *tab_popup;
  DeepinWMBackground* ws_previewer;
  GtkWidget* exposing_windows_popup;
  GtkWidget* workspace_indicator;

  MetaTilePreview *tile_preview;

  guint tile_preview_timeout_id;

  MetaWorkspace *active_workspace;

  /* This window holds the focus when we don't want to focus
   * any actual clients
   */
  Window no_focus_window;

  GList *workspaces;

  MetaStack *stack;

  MetaCursor current_cursor;

  Window flash_window;

  Window wm_sn_selection_window;
  Atom wm_sn_atom;
  guint32 wm_sn_timestamp;

  MetaXineramaScreenInfo *xinerama_infos;
  int n_xinerama_infos;

  /* Cache the current Xinerama */
  int last_xinerama_index;

#ifdef HAVE_STARTUP_NOTIFICATION
  SnMonitorContext *sn_context;
  GSList *startup_sequences;
  guint startup_sequence_timeout;
#endif

#ifdef HAVE_COMPOSITE_EXTENSIONS
  Window wm_cm_selection_window;
  guint32 wm_cm_timestamp;
#endif

  guint work_area_idle;

  int rows_of_workspaces;
  int columns_of_workspaces;
  MetaScreenCorner starting_corner;
  guint vertical_workspaces : 1;

  guint keys_grabbed : 1;
  guint all_keys_grabbed : 1;

  guint corner_actions_enabled: 1;

  int closing;

  /* gc for XOR on root window */
  GC root_xor_gc;

  /* Managed by compositor.c */
  gpointer compositor_data;
  
  /* Instead of unmapping withdrawn windows we can leave them mapped
   * and restack them below a guard window. When using a compositor
   * this allows us to provide live previews of unmapped windows */
  Window guard_window;

  /* Windows used to support hot-zone functionality.
   */
  Window corner_windows[4];
  gint corner_enabled[4];

  GtkWidget *corner_indicator[4];

  /* DeepinDesktopBackground's */
  GPtrArray* desktop_bgs;
  GArray* desktop_bg_windows;
};

MetaScreen*   meta_screen_new                 (MetaDisplay                *display,
                                               int                         number,
                                               guint32                     timestamp);
void          meta_screen_free                (MetaScreen                 *screen,
                                               guint32                     timestamp);
void          meta_screen_manage_all_windows  (MetaScreen                 *screen);
void          meta_screen_foreach_window      (MetaScreen                 *screen,
                                               MetaScreenWindowFunc        func,
                                               gpointer                    data);
void          meta_screen_queue_frame_redraws (MetaScreen                 *screen);
void          meta_screen_queue_window_resizes (MetaScreen                 *screen);

int           meta_screen_get_n_workspaces    (MetaScreen                 *screen);

MetaWorkspace* meta_screen_get_workspace_by_index (MetaScreen    *screen,
                                                   int            index);

MetaWorkspace* meta_screen_new_workspace      (MetaScreen                 *screen);
void          meta_screen_reorder_workspace   (MetaScreen                 *screen,
                                               MetaWorkspace              *workspace,
                                               int                         new_index);
void          meta_screen_remove_workspace    (MetaScreen                 *screen,
                                               MetaWorkspace              *workspace);
void          meta_screen_set_cursor          (MetaScreen                 *screen,
                                               MetaCursor                  cursor);
void          meta_screen_update_cursor       (MetaScreen                 *screen);

void          meta_screen_ensure_tab_popup    (MetaScreen                 *screen,
                                               MetaTabList                 list_type,
                                               MetaTabShowType             show_type);

void          meta_screen_ensure_workspace_indicator  (MetaScreen *screen);
void          meta_screen_ensure_previewing_workspace (MetaScreen *screen);
void          meta_screen_ensure_exposing_windows     (MetaScreen *screen);

void          meta_screen_tile_preview_update          (MetaScreen    *screen,
                                                        gboolean       delay);
void          meta_screen_tile_preview_hide            (MetaScreen    *screen);

MetaWindow*   meta_screen_get_mouse_window     (MetaScreen                 *screen,
                                                MetaWindow                 *not_this_one);

const MetaXineramaScreenInfo* meta_screen_get_current_xinerama    (MetaScreen    *screen);
const MetaXineramaScreenInfo* meta_screen_get_xinerama_for_rect   (MetaScreen    *screen,
                                                                   MetaRectangle *rect);
const MetaXineramaScreenInfo* meta_screen_get_xinerama_for_window (MetaScreen    *screen,
                                                                   MetaWindow    *window);


const MetaXineramaScreenInfo* meta_screen_get_xinerama_neighbor (MetaScreen *screen,
                                                                 int         which_xinerama,
                                                                 MetaScreenDirection dir);
void          meta_screen_get_natural_xinerama_list (MetaScreen *screen,
                                                     int**       xineramas_list,
                                                     int*        n_xineramas);

void          meta_screen_update_workspace_layout (MetaScreen             *screen);
void          meta_screen_update_workspace_names  (MetaScreen             *screen);
void          meta_screen_queue_workarea_recalc   (MetaScreen             *screen);

Window meta_create_offscreen_window (Display *xdisplay,
                                     Window   parent,
                                     long     valuemask);

typedef struct MetaWorkspaceLayout MetaWorkspaceLayout;

struct MetaWorkspaceLayout
{
  int rows;
  int cols;
  int *grid;
  int grid_area;
  int current_row;
  int current_col;
};

void meta_screen_calc_workspace_layout (MetaScreen          *screen,
                                        int                  num_workspaces,
                                        int                  current_space,
                                        MetaWorkspaceLayout *layout);
void meta_screen_free_workspace_layout (MetaWorkspaceLayout *layout);

void     meta_screen_minimize_all_on_active_workspace_except (MetaScreen *screen,
                                                              MetaWindow *keep);

/* Show/hide the desktop (temporarily hide all windows) */
void     meta_screen_show_desktop        (MetaScreen *screen,
                                          guint32     timestamp);
void     meta_screen_unshow_desktop      (MetaScreen *screen);

/* Update whether the destkop is being shown for the current active_workspace */
void     meta_screen_update_showing_desktop_hint          (MetaScreen *screen);

gboolean meta_screen_apply_startup_properties (MetaScreen *screen,
                                               MetaWindow *window);
void	 meta_screen_composite_all_windows (MetaScreen *screen);

void meta_screen_request_hide_windows(MetaScreen* screen);
void meta_screen_cancel_hide_windows(MetaScreen* screen);

void meta_screen_invalidate_backgrounds(MetaScreen* screen, MetaRectangle* bounds);

void meta_screen_enter_corner (MetaScreen *screen,
                               MetaScreenCorner corner);
void meta_screen_leave_corner (MetaScreen *screen,
                               MetaScreenCorner corner);
void          meta_screen_enable_corner_actions (MetaScreen *screen, gboolean enable);
void          meta_screen_enable_corner        (MetaScreen                 *screen,
                                               MetaScreenCorner            corner, 
                                               gboolean                    val);

void meta_screen_update_corner (MetaScreen* screen, CornerUpdateMask update);

#endif
