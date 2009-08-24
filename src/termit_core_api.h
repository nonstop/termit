#ifndef TERMIT_CORE_API_H
#define TERMIT_CORE_API_H

#include "termit.h"
#include "configs.h"

void termit_reconfigure();
void termit_after_show_all();

void termit_append_tab();
void termit_append_tab_with_command(const gchar* command);
void termit_append_tab_with_details(const struct TabInfo*);

void termit_activate_tab(gint tab_index);
void termit_prev_tab();
void termit_next_tab();
void termit_paste();
void termit_copy();
void termit_close_tab();
void termit_toggle_menubar();
void termit_set_window_title(const gchar* title);
void termit_set_statusbar_encoding(gint page);
void termit_set_encoding(const gchar* encoding);
void termit_quit();

void termit_tab_set_font(struct TermitTab* pTab, const gchar* font_name);
void termit_tab_set_transparency(struct TermitTab* pTab, gdouble transparency);
void termit_tab_set_style(gint tab_index, const struct TermitStyle*);
void termit_tab_set_color_foreground(struct TermitTab* pTab, const GdkColor* p_color);
void termit_tab_set_color_background(struct TermitTab* pTab, const GdkColor* p_color);
void termit_tab_set_color_foreground_by_index(gint tab_index, const GdkColor*);
void termit_tab_set_color_background_by_index(gint tab_index, const GdkColor*);
void termit_tab_set_title(struct TermitTab* pTab, const gchar* title);
void termit_tab_set_audible_bell(struct TermitTab* pTab, gboolean audible_bell);
void termit_tab_set_visible_bell(struct TermitTab* pTab, gboolean visible_bell);

int termit_get_current_tab_index();

/**
 * function to switch key processing policy
 * keycodes - kb layout independent
 * keysyms - kb layout dependent
 * */
void termit_set_kb_policy(enum TermitKbPolicy kbp);

#endif /* TERMIT_CORE_API_H */

