/*
 * This is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "tilda-url-spawner.h"

#include <glib/gi18n.h>

#include "configsys.h"
#include "debug.h"

void launch_configured_web_browser (const char * match);

void tilda_url_spawner_spawn_browser_for_match (GtkWindow * parent,
                                                const gchar * match,
                                                TildaMatchRegistryEntry * entry)
{
    gchar * uri;
    TildaMatchRegistryFlavor flavor;

    if (match == NULL)
        return;

    flavor = tilda_match_registry_entry_get_flavor (entry);

    if (flavor == TILDA_MATCH_FLAVOR_URL ||
        flavor == TILDA_MATCH_FLAVOR_DEFAULT_TO_HTTP)
    {
        /* Starting with version 1.6 and above by default tilda uses
         * the gtk_show_uri function family to open URIs, this option
         * allows to go back to the legacy behavior and use a configured
         * command to spawn a browser. We will eventually remove this option. */
        gboolean use_custom_web_browser = config_getbool("use_custom_web_browser");

        if (use_custom_web_browser) {
            launch_configured_web_browser (match);
            return;
        }
    }

    if (!tilda_match_registry_flavor_is_uri(entry)) {
        return;
    }

    uri = tilda_match_registry_entry_get_uri_from_match (entry, match);

    if (uri == NULL) {
        g_debug ("Url not opened, because match flavor is unknown.");

        return;
    }

    GError * error = NULL;

    gtk_show_uri_on_window(parent, uri, GDK_CURRENT_TIME, &error);

    if (error != NULL) {
        g_critical("Unable to open URI: %s", error->message);
        g_error_free(error);
    }

    g_free (uri);
}

static gchar **
build_browser_argv (const gchar *browser_cmd,
                    const gchar *match)
{
    gchar **argv = NULL;
    gint argc = 0;
    GError *error = NULL;

    if (!g_shell_parse_argv (browser_cmd, &argc, &argv, &error))
    {
        g_critical (_("Failed to parse web browser command `%s': %s\n"),
                    browser_cmd, error->message);
        g_error_free (error);
        return NULL;
    }

    argv = g_realloc (argv, (argc + 2) * sizeof (gchar *));
    argv[argc]     = g_strdup (match);
    argv[argc + 1] = NULL;

    return argv;
}

void launch_configured_web_browser (const char * match)
{
    const gchar *browser_cmd = config_getstr ("web_browser");
    gchar **argv = build_browser_argv (browser_cmd, match);

    if (argv == NULL)
        return;

    GError *error = NULL;

    g_debug ("Launching browser: `%s' with url: `%s'", browser_cmd, match);

    gboolean result = g_spawn_async (NULL,
                                     argv,
                                     NULL,
                                     G_SPAWN_SEARCH_PATH,
                                     NULL, NULL, NULL,
                                     &error);

    if (!result)
    {
        g_critical (_("Failed to launch the web browser `%s': %s\n"),
                    browser_cmd, error->message);
        TILDA_PERROR ();
        g_clear_error (&error);
    }

    g_strfreev (argv);
}
