#ifndef TERMIT_CORE_API_H
#define TERMIT_CORE_API_H

#include "configs.h"

void termit_reconfigure();

void termit_append_tab();
void termit_append_tab_with_command(const gchar* command);
struct TabInfo;
void termit_append_tab_with_details(const struct TabInfo*);

void termit_activate_tab(gint tab_index);
void termit_prev_tab();
void termit_next_tab();
void termit_paste();
void termit_copy();
void termit_close_tab();

void termit_quit();

void termit_set_window_title(const gchar* title);
void termit_set_font(const gchar* font_name);
void termit_set_default_colors();
void termit_set_foreground_color(const GdkColor*);
void termit_hide_scrollbars();
void termit_hide_tab_scrollbar();
void termit_set_encoding(const gchar* encoding);
void termit_set_tab_name(guint tab_index, const gchar* name);
void termit_set_statusbar_encoding(gint page);

struct TermitTab* termit_get_tab_by_index(gint index);

/**
 * function to switch key processing policy
 * keycodes - kb layout independent
 * keysyms - kb layout dependent
 * */
void termit_set_kb_policy(enum TermitKbPolicy kbp);

#endif /* TERMIT_CORE_API_H */

