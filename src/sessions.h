#ifndef SESSIONS_H
#define SESSIONS_H

void termit_init_sessions();
void termit_load_session(const gchar* sessionFile);
void termit_save_session(const gchar* sessionFile);

#endif /* SESSIONS_H */

