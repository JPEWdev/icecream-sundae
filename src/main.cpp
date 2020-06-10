/*
 * Command line Icecream status monitor
 * Copyright (C) 2018-2020 by Garmin Ltd. or its subsidiaries.
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

#include <cassert>
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
#include "simulator.hpp"

int total_remote_jobs = 0;
int total_local_jobs = 0;
GMainLoop *main_loop = nullptr;
bool all_expanded = false;
std::unique_ptr<Scheduler> scheduler;
std::unique_ptr<UserInterface> interface;

Job::Map Job::allJobs;
Job::Map Job::pendingJobs;
Job::Map Job::activeJobs;
Job::Map Job::localJobs;
Job::Map Job::remoteJobs;

std::vector<int> Host::host_color_ids;
int Host::localhost_color_id;
std::map<uint32_t, std::shared_ptr<Host> > Host::hosts;

static std::string schedname = std::string();
static std::string netname = std::string();
static gboolean opt_simulate = FALSE;
static gboolean opt_anonymize = FALSE;
static gint opt_sim_seed = 12345;
static gint opt_sim_cycles = -1;
static gint opt_sim_speed = 20;

std::shared_ptr<Job> Job::create(uint32_t id)
{
    class RealJob: public Job {
    public:
        explicit RealJob(uint32_t id): Job(id) {}
        virtual ~RealJob() {}
    };

    auto job = find(id);

    if (!job) {
        job = std::make_shared<RealJob>(id);
        allJobs[id] = job;
    }

    return job;
}

std::shared_ptr<Job> Job::find(uint32_t id)
{
    auto j = allJobs.find(id);

    if (j != allJobs.end())
        return j->second;
    return nullptr;
}

void Job::remove(uint32_t id)
{
    removeTypes(id);
    removeFromMap(allJobs, id);

    if (interface)
        interface->triggerRedraw();
}

void Job::removeFromMap(Map &map, uint32_t id)
{
    auto j = map.find(id);
    if (j != map.end())
        map.erase(j);
}

void Job::removeTypes(uint32_t id)
{
    removeFromMap(pendingJobs, id);
    removeFromMap(activeJobs, id);
    removeFromMap(localJobs, id);
    removeFromMap(remoteJobs, id);
}

void Job::createLocal(uint32_t id, uint32_t hostid, std::string const& filename)
{
    auto job = Job::create(id);

    job->active = true;
    job->clientid = hostid;
    job->hostid = hostid;
    job->is_local = true;
    job->filename = filename;
    job->start_time = g_get_monotonic_time();

    auto h = job->getClient();
    if (h)
        h->total_local++;
    total_local_jobs++;

    removeTypes(id);
    localJobs[id] = job;
    activeJobs[id] = job;

    if (interface)
        interface->triggerRedraw();
}

void Job::createPending(uint32_t id, uint32_t clientid, std::string const& filename)
{
    auto job = Job::create(id);

    job->clientid = clientid;
    job->filename = filename;

    removeTypes(id);
    pendingJobs[id] = job;

    if (interface)
        interface->triggerRedraw();
}

void Job::createRemote(uint32_t id, uint32_t hostid)
{
    auto job = Job::find(id);

    if (!job)
        return;

    job->active = true;
    job->hostid = hostid;
    job->start_time = g_get_monotonic_time();

    auto host = job->getHost();
    if (host)
        host->total_in++;

    auto client = job->getClient();
    if (client)
        client->total_out++;
    total_remote_jobs++;

    removeTypes(id);
    activeJobs[id] = job;
    remoteJobs[id] = job;

    if (interface)
        interface->triggerRedraw();
}

void Job::clearAll()
{
    allJobs.clear();
    pendingJobs.clear();
    activeJobs.clear();
    localJobs.clear();
    remoteJobs.clear();
}

std::shared_ptr<Host> Job::getClient() const
{
    if (!clientid)
        return nullptr;

    return Host::find(clientid);
}

std::shared_ptr<Host> Job::getHost() const
{
    if (!hostid)
        return nullptr;

    return Host::find(hostid);
}

std::shared_ptr<Host> Host::create(uint32_t id)
{
    class RealHost: public Host {
    public:
        explicit RealHost(uint32_t id): Host(id) {}
        virtual ~RealHost() {}
    };

    auto host = find(id);

    if (!host) {
        host = std::make_shared<RealHost>(id);
        hosts[id] = host;
    }

    if (interface)
        interface->triggerRedraw();
    return host;
}

std::shared_ptr<Host> Host::find(uint32_t id)
{
    auto h = hosts.find(id);

    if (h != hosts.end())
        return h->second;

    return nullptr;
}

void Host::remove(uint32_t id)
{
    auto h = hosts.find(id);

    if (h != hosts.end()) {
        hosts.erase(h);
        if (interface)
            interface->triggerRedraw();
    }
}

Job::Map Host::getPendingJobs() const
{
    Job::Map map;

    for (auto&& j : Job::pendingJobs) {
        if (j.second->clientid == id)
            map[j.first] = j.second;
    }

    return map;
}

Job::Map Host::getActiveJobs() const
{
    Job::Map map;

    for (auto&& j : Job::activeJobs) {
        if (j.second->clientid == id)
            map[j.first] = j.second;
    }

    return map;
}

Job::Map Host::getCurrentJobs() const
{
    Job::Map map;

    for (auto&& j : Job::activeJobs) {
        if (j.second->hostid == id)
            map[j.first] = j.second;
    }

    return map;
}

int Host::getColor() const
{
    char buffer[1024];

    if (gethostname(buffer, sizeof(buffer)) == 0 ) {
        buffer[sizeof(buffer) - 1] = '\0';
        if (getName() == buffer)
            return localhost_color_id;
    }

    return host_color_ids[hashName() % host_color_ids.size()];
}

size_t Host::hashName() const
{
    return std::hash<std::string>{}(getName());
}

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
        { "scheduler", 's', 0, G_OPTION_ARG_STRING, &opt_scheduler, "Icecream scheduler hostname", NULL },
        { "netname", 'n', 0, G_OPTION_ARG_STRING, &opt_netname, "Icecream network name", NULL },
        { "simulate", 0, 0, G_OPTION_ARG_NONE, &opt_simulate, "Simulate activity", NULL },
        { "sim-seed", 0, 0, G_OPTION_ARG_INT, &opt_sim_seed, "Simulator seed", NULL },
        { "sim-cycles", 0, 0, G_OPTION_ARG_INT, &opt_sim_cycles, "Number of simulator cycles to run. -1 for no limit", NULL },
        { "sim-speed", 0, 0, G_OPTION_ARG_INT, &opt_sim_speed, "Simulator speed (milliseconds between cycles)", NULL },
        { "anonymize", 0, 0, G_OPTION_ARG_NONE, &opt_anonymize, "Anonymize hosts and files (for demos)", NULL },
        { "about", 0, 0, G_OPTION_ARG_NONE, &opt_about, "Show about", NULL },
        { "version", 0, 0, G_OPTION_ARG_NONE, &opt_version, "Show version", NULL },
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

static gboolean on_quit_signal(gpointer)
{
    g_main_loop_quit(main_loop);
    return TRUE;
}

static gboolean process_input(gint, GIOCondition, gpointer)
{
    int c = interface->processInput();
    if (c && scheduler)
        scheduler->onInput(c);
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

    if (opt_simulate)
        scheduler = create_simulator(opt_sim_seed, opt_sim_cycles, opt_sim_speed);
    else
        scheduler = connect_to_scheduler(netname, schedname);
    interface = create_ncurses_interface();
    interface->set_anonymize(opt_anonymize);

    int input_fd = interface->getInputFd();
    GlibSource input_source;

    if (input_fd >= 0)
        input_source.set(g_unix_fd_add(input_fd, G_IO_IN, process_input, nullptr));

    GlibSource sigint_source(g_unix_signal_add(SIGINT, reinterpret_cast<GSourceFunc>(on_quit_signal), nullptr));
    GlibSource sigterm_source(g_unix_signal_add(SIGTERM, reinterpret_cast<GSourceFunc>(on_quit_signal), nullptr));

    g_main_loop_run(main_loop);

    scheduler.reset();
    interface.reset();

    g_main_loop_unref(main_loop);

    Job::clearAll();
    Host::hosts.clear();

    return 0;
}
