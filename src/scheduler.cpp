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

#include <memory>

#include <glib.h>
#include <glib-unix.h>
#include <icecc/comm.h>
#include <poll.h>

#include "main.hpp"
#include "draw.hpp"
#include "scheduler.hpp"

static std::unique_ptr<MsgChannel> scheduler;
static guint scheduler_source = 0;

static bool process_message(MsgChannel *sched);

static gboolean scheduler_process(gint fd, GIOCondition condition, gpointer)
{
    while (!scheduler->read_a_bit() || scheduler->has_msg()) {
        if (!process_message(scheduler.get()))
            break;
    }

    return TRUE;
}

static void discover_scheduler(std::string const &netname, std::string const &schedname)
{
    if (scheduler)
        return;

    auto discover = std::make_unique<DiscoverSched>(netname, 2, schedname);

    scheduler.reset(discover->try_get_scheduler());

    hosts.clear();
    jobs.clear();
    current_scheduler_name.clear();
    current_net_name.clear();

    while (!scheduler && !discover->timed_out()) {
        if (discover->listen_fd() < 0) {
            usleep(500);
            scheduler.reset(discover->try_get_scheduler());
        } else {
            struct pollfd pfd;
            pfd.fd = discover->listen_fd();
            pfd.events = POLLIN;

            std::cout << "Waiting " << pfd.fd << std::endl;
            poll(&pfd, 1, -1);

            scheduler.reset(discover->try_get_scheduler());
        }
    }
    std::cout << "Done waiting" << std::endl;

    if (!scheduler) {
        std::cout << "Cannot get scheduler" << std::endl;
        return;
    }

    current_scheduler_name = discover->schedulerName();
    current_net_name = discover->networkName();
    if (current_net_name.empty())
        current_net_name = "ICECREAM";

    std::cout << "Got scheduler " << current_scheduler_name << std::endl;
    scheduler->setBulkTransfer();

    if (!scheduler->send_msg(MonLoginMsg())) {
        std::cout << "Cannot login" << std::endl;
        scheduler.reset();
    }
}

static bool process_message(MsgChannel *sched)
{
    std::unique_ptr<Msg> msg(sched->get_msg());
    if (!msg)
        return false;

    switch (msg->type) {
    case M_MON_LOCAL_JOB_BEGIN: {
        auto *m = dynamic_cast<MonLocalJobBeginMsg*>(msg.get());
        auto &job = jobs[m->job_id];

        job.active = true;
        job.hostid = m->hostid;
        job.clientid = m->hostid;
        job.is_local = true;

        //TODO: Should the total include local jobs?
        //hosts[job.hostid].total_in++;
        //hosts[job.clientid].total_out++;
        //total_jobs++;
        trigger_redraw();
        break;
    }
    case M_JOB_LOCAL_DONE: {
        auto *m = dynamic_cast<JobLocalDoneMsg*>(msg.get());
        auto job = jobs.find(m->job_id);
        if (job != jobs.end())
            jobs.erase(job);
        trigger_redraw();
        break;
    }
    case M_MON_JOB_BEGIN: {
        auto *m = dynamic_cast<MonJobBeginMsg*>(msg.get());
        auto &job = jobs[m->job_id];

        job.active = true;
        job.hostid = m->hostid;

        hosts[job.hostid].total_in++;
        hosts[job.clientid].total_out++;
        total_jobs++;
        trigger_redraw();
        break;
    }
    case M_MON_JOB_DONE: {
        auto *m = dynamic_cast<MonJobDoneMsg*>(msg.get());
        auto job = jobs.find(m->job_id);
        if (job != jobs.end())
            jobs.erase(job);
        trigger_redraw();
        break;
    }
    case M_MON_GET_CS: {
        auto *m = dynamic_cast<MonGetCSMsg*>(msg.get());
        auto &job = jobs[m->job_id];

        job.clientid = m->clientid;
        trigger_redraw();
        break;
    }
    case M_MON_STATS: {
        auto *m = dynamic_cast<MonStatsMsg*>(msg.get());

        std::stringstream ss(m->statmsg);
        std::string key;
        std::string value;
        bool alive = false;

        while (std::getline(ss, key, ':') && std::getline(ss, value)) {
            if (key == "Name")
                alive = true;

            hosts[m->hostid].attr[key] = value;
        }

        if (!alive)
            hosts.erase(m->hostid);

        trigger_redraw();
        break;
    }
    case M_END:
        connect_to_scheduler(current_net_name, current_scheduler_name, true);
        break;
    default:
        break;
    }

    return true;
}

void connect_to_scheduler(std::string const &netname, std::string const &schedname, bool reset)
{
    if (reset)
        scheduler.reset();

    discover_scheduler(netname, schedname);

    if (scheduler_source) {
        g_source_remove(scheduler_source);
        scheduler_source = 0;
    }

    if (scheduler)
        scheduler_source = g_unix_fd_add(scheduler->fd, G_IO_IN, scheduler_process, nullptr);

    trigger_redraw();
}


