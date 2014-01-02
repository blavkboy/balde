/*
 * balde: A microframework for C based on GLib and bad intentions.
 * Copyright (C) 2013 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 *
 * This program can be distributed under the terms of the LGPL-2 License.
 * See the file COPYING.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */


/*
 * balde-template-gen is a damn simple code generator, that converts an HTML
 * template to C source, that should be compiled and linked to the balde app.
 *
 * This isn't easy to use but is really fast, and this is what matters! :P
 *
 * Usage: $ balde-template-gen template.html template.[ch]
 *
 * This will generate template.c or template.h. You should add both files to
 * the foo_SOURCES variable in your Makefile.am file, and include template.h
 * in your app source.
 *
 * Also, you may want to add a rule to rebuild your template.c and template.h
 * files when you change template.html.
 */

#include <glib.h>
#include <stdlib.h>
#include <string.h>


static gboolean
replace_template_variables_cb(const GMatchInfo *info, GString *res, gpointer data)
{
    gchar *match = g_match_info_fetch(info, 1);
    GSList **tmp = (GSList**) data;
    *tmp = g_slist_append(*tmp, (gchar*) g_strdup(match));
    data = tmp;
    g_string_append(res, "%s");
    g_free (match);
    return FALSE;
}


gchar*
generate_source(const gchar *template_name, const gchar *template_source)
{
    // escape newlines.
    gchar *escaped_nl_source = g_strescape(template_source, "");

    // escape % chars.
    GRegex *re_percent = g_regex_new("%", 0, 0, NULL);
    gchar *escaped_perc_source = g_regex_replace_literal(re_percent,
            escaped_nl_source, -1, 0, "%%", 0, NULL);
    g_regex_unref(re_percent);
    g_free(escaped_nl_source);

    // replace variables
    GRegex *re_variable = g_regex_new("{{ *([a-zA-Z][a-zA-Z0-9_]*) *}}", 0, 0,
        NULL);
    GSList *variables = NULL;
    gchar *final_source = g_regex_replace_eval(re_variable,
        escaped_perc_source, -1, 0, 0, replace_template_variables_cb,
        &variables, NULL);
    g_regex_unref(re_variable);
    g_free(escaped_perc_source);
    GString *rv = g_string_new("");
    g_string_append_printf(rv,
        "// WARNING: this file was generated automatically by balde-template-gen\n"
        "\n"
        "#include <balde.h>\n"
        "#include <glib.h>\n"
        "\n"
        "const gchar *balde_template_%s_format = \"%s\";\n"
        "\n"
        "void\n"
        "balde_template_%s(balde_response_t *response)\n"
        "{\n",
        template_name, final_source, template_name);
    if (variables == NULL) {
        g_string_append_printf(rv,
            "    gchar *rv = g_strdup(balde_template_%s_format);\n",
            template_name);
    }
    else {
        g_string_append_printf(rv,
            "    gchar *rv = g_strdup_printf(balde_template_%s_format,\n",
            template_name);
    }
    for (GSList *tmp = variables; tmp != NULL; tmp = tmp->next) {
        g_string_append_printf(rv,
            "        balde_response_get_tmpl_var(response, \"%s\")",
            (gchar*) tmp->data);
        if (tmp->next != NULL)
            g_string_append(rv, ",\n");
        else
            g_string_append(rv, ");\n");
    }
    g_slist_free_full(variables, g_free);
    g_string_append(rv,
        "    balde_response_append_body(response, rv);\n"
        "    g_free(rv);\n"
        "}\n");
    g_free(final_source);
    return g_string_free(rv, FALSE);
}


gchar*
generate_header(const gchar *template_name)
{
    return g_strdup_printf(
        "// WARNING: this file was generated automatically by balde-template-gen\n"
        "\n"
        "#ifndef __%s_balde_template\n"
        "#define __%s_balde_template\n"
        "\n"
        "#include <balde.h>\n"
        "\n"
        "const gchar *balde_template_%s_format;\n"
        "void balde_template_%s(balde_response_t *response);\n"
        "\n"
        "#endif\n", template_name, template_name, template_name, template_name);
}


gchar*
get_template_name(const gchar *template_basename)
{
    gchar *template_name = g_path_get_basename(template_basename);
    for (guint i = strlen(template_name); i != 0; i--) {
        if (template_name[i] == '.') {
            template_name[i] = '\0';
            break;
        }
    }
    for (guint i = 0; template_name[i] != '\0'; i++) {
        if (!g_ascii_isalpha(template_name[i])) {
            template_name[i] = '_';
        }
    }
    return template_name;
}


int
main(int argc, char **argv)
{
    int rv = EXIT_SUCCESS;
    if (argc != 3) {
        g_printerr("Usage: $ balde-template-gen template.html template.[ch]\n");
        rv = EXIT_FAILURE;
        goto point1;
    }
    gchar *template_name = get_template_name(argv[2]);
    gchar *template_source = NULL;
    gchar *source = NULL;
    if (g_str_has_suffix(argv[2], ".c")) {
        if (!g_file_get_contents(argv[1], &template_source, NULL, NULL)) {
            g_printerr("Failed to read source file: %s\n", argv[1]);
            rv = EXIT_FAILURE;
            goto point2;
        }
        source = generate_source(template_name, template_source);
    }
    else if (g_str_has_suffix(argv[2], ".h")) {
        source = generate_header(template_name);
    }
    else {
        g_printerr("Invalid filename: %s\n", argv[2]);
        rv = EXIT_FAILURE;
        goto point3;
    }
    if (!g_file_set_contents(argv[2], source, -1, NULL)) {
        g_printerr("Failed to write file: %s\n", argv[2]);
        rv = EXIT_FAILURE;
        goto point3;  // duh!
    }
point3:
    if (source != NULL)
        g_free(source);
    if (template_source != NULL)
        g_free(template_source);
point2:
    if (template_name != NULL)
        g_free(template_name);
point1:
    return rv;
}
