

void termit_load_default_key_bindings()
{
    struct KeyBindging kb;
    // previous tab - Alt-Left
    kb.name = "prev_tab";
    kb.state = GDK_MOD1_MASK;
    kb.keyval = GDK_Left;
    kb.callback = termit_prev_tab;
    g_array_append_val(configs.key_bindings, kb);
    // next tab - Alt-Right
    kb.name = "next_tab";
    kb.state = GDK_MOD1_MASK;
    kb.keyval = GDK_Right;
    kb.callback = termit_next_tab;
    g_array_append_val(configs.key_bindings, kb);
    // copu - Ctrl-Insert
    kb.name = "copy";
    kb.state = GDK_CONTROL_MASK;
    kb.keyval = GDK_Insert;
    kb.callback = termit_copy;
    g_array_append_val(configs.key_bindings, kb);
    // paste - Shift-Insert
    kb.name = "paste";
    kb.state = GDK_SHIFT_MASK;
    kb.keyval = GDK_Insert;
    kb.callback = termit_paste;
    g_array_append_val(configs.key_bindings, kb);
    // open tab - Ctrl-t
    kb.name = "open_tab";
    kb.state = GDK_CONTROL_MASK;
    kb.keyval = GDK_t;
    kb.callback = termit_new_tab;
    g_array_append_val(configs.key_bindings, kb);
    // close tab - Ctrl-w
    kb.name = "close_tab";
    kb.state = GDK_CONTROL_MASK;
    kb.keyval = GDK_w;
    kb.callback = termit_close_tab;
    g_array_append_val(configs.key_bindings, kb);
    int i = 0;
    for (; i<configs.key_bindings->len; ++i)
    {
        struct KeyBindging* kb = &g_array_index(configs.key_bindings, struct KeyBindging, i);
        TRACE_STR(kb->name);
        TRACE_NUM(kb->state);
        TRACE_NUM(kb->keyval);
    }
}

static gint get_kb_index(const gchar* name)
{
    int i = 0;
    for (; i<configs.key_bindings->len; ++i)
    {
        struct KeyBindging* kb = &g_array_index(configs.key_bindings, struct KeyBindging, i);
        if (!g_ascii_strcasecmp(kb->name, name))
            return i;
    }
    return -1;
}


void termit_load_keybindings(GKeyFile* key_file)
{
    termit_load_default_key_bindings();
    
    const gchar* kb_group = "keybindings";
    GError * error = NULL;
    gsize len = 0;
    gchar **names = g_key_file_get_keys(keyfile, kb_group, &len, &error);
    
    int i = 0;
    for (; i<len; ++i)
    {
        TRACE_STR(names[i]);
        if (get_kb_index(names[i]) < 0)
            continue;
        gchar* value = g_key_file_get_value(keyfile, kb_group, names[i], &error);
        TRACE_STR(value);
        g_free(value);
    }

    g_strfreev(names);
}

