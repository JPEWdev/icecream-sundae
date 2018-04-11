/*
 * Command line Icecream status monitor
 * Copyright (C) 2018 by Garmin Ltd. or its subsidiaries.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include <algorithm>
#include <vector>
#include <memory>
#include <iostream>
#include <map>
#include <glib.h>
#include <glib-unix.h>

#include "main.hpp"
#include "draw.hpp"
#include "scheduler.hpp"

std::map<uint32_t, Job> jobs;
int total_jobs = 0;
std::map<uint32_t, Host> hosts;
GMainLoop *main_loop = nullptr;
bool all_expanded = false;
std::vector<int> Host::host_color_ids;
std::string current_scheduler_name;
std::string current_net_name;

static std::string schedname = std::string();
static std::string netname = std::string();

static bool parse_args(int *argc, char ***argv)
{
    class GOptionContextDelete
    {
    public:
        void operator()(GOptionContext* ptr) const
        {
            g_option_context_free(ptr);
        }
    };

    static gchar *opt_scheduler = NULL;
    static gchar *opt_netname = NULL;
    static gboolean opt_about = FALSE;
    static gboolean opt_version = FALSE;

    static const GOptionEntry opts[] =
    {
        { "scheduler", 's', 0, G_OPTION_ARG_STRING, &opt_scheduler, "Icecream scheduler hostname" },
        { "netname", 'n', 0, G_OPTION_ARG_STRING, &opt_netname, "Icecream network name" },
        { "about", 0, 0, G_OPTION_ARG_NONE, &opt_about, "Show about" },
        { "version", 0, 0, G_OPTION_ARG_NONE, &opt_version, "Show version" },
        {}
    };

    std::unique_ptr<GOptionContext, GOptionContextDelete> context(g_option_context_new(nullptr));

    g_option_context_add_main_entries(context.get(), opts, NULL);

    GError *error = NULL;
    if (!g_option_context_parse(context.get(), argc, argv, &error)) {
        std::cout << "Option parsing failed: " << error->message << std::endl;
        g_clear_error(&error);
        return false;
    }

    if (opt_scheduler)
        schedname = opt_scheduler;

    if (opt_netname)
        netname = opt_netname;

    if (opt_version) {
        std::cout << VERSION << std::endl;
        return false;
    }

    if (opt_about) {
        std::cout << "Command line Icecream status monitor" << std::endl;
        std::cout << "Version: " << VERSION << std::endl;
        std::cout <<
            "Copyright (C) 2018 by Garmin Ltd. or its subsidiaries." << std::endl <<
            std::endl <<
            "This program is free software; you can redistribute it and/or" << std::endl <<
            "modify it under the terms of the GNU General Public License" << std::endl <<
            "as published by the Free Software Foundation; either version 2" << std::endl <<
            "of the License, or (at your option) any later version." << std::endl <<
            std::endl <<
            "This program is distributed in the hope that it will be useful," << std::endl <<
            "but WITHOUT ANY WARRANTY; without even the implied warranty of" << std::endl <<
            "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the" << std::endl <<
            "GNU General Public License for more details." << std::endl <<
            std::endl <<
            "You should have received a copy of the GNU General Public License" << std::endl<<
            "along with this program; if not, write to the Free Software" << std::endl <<
            "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA." << std::endl;
        return false;
    }

    return true;
}

static gboolean on_quit_signal(gpointer user_data)
{
    g_main_loop_quit(main_loop);
    return TRUE;
}

int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");
    if (!parse_args(&argc, &argv))
        return 1;

    std::cout <<
        "Command line Icecream status monitor, Version " << VERSION << std::endl <<
        "Copyright (C) 2018 by Garmin Ltd. or its subsidiaries." << std::endl <<
        "This is free software, and you are welcome to redistribute it" << std::endl <<
        "under certain conditions; run with '--about' for details." << std::endl;


    main_loop = g_main_loop_new(nullptr, false);

    connect_to_scheduler(netname, schedname);

    CursesMode curses_mode;

    g_unix_signal_add(SIGINT, reinterpret_cast<GSourceFunc>(on_quit_signal), nullptr);
    g_unix_signal_add(SIGTERM, reinterpret_cast<GSourceFunc>(on_quit_signal), nullptr);

    g_main_loop_run(main_loop);

    g_main_loop_unref(main_loop);

    return 0;
}
