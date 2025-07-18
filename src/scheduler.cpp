/*
 * Command line Icecream status monitor
 * Copyright (C) 2018-2019 by Garmin Ltd. or its subsidiaries.
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
#include <iostream>

#include <glib.h>
#include <glib-unix.h>
#include <icecc/comm.h>
#include <poll.h>

#include "main.hpp"
#include "scheduler.hpp"

class IcecreamScheduler: public Scheduler {
public:
    IcecreamScheduler(std::string const &netname, std::string const &schedname) : Scheduler()
    {
        reconnect(netname, schedname);
    }

    virtual ~IcecreamScheduler() {}

    virtual std::string getNetName() const override { return current_net_name; }
    virtual std::string getSchedulerName() const override { return current_scheduler_name; }

private:
    static gboolean scheduler_process(gint fd, GIOCondition condition, gpointer);
    static gboolean on_reconnect_timer(gpointer);

    bool process_message(MsgChannel *sched);
    void discover_scheduler(std::string const &netname, std::string const &schedname);
    void reconnect(std::string const &netname, std::string const &schedname);

    std::unique_ptr<MsgChannel> scheduler = nullptr;
    GlibSource scheduler_source;
    std::string current_net_name;
    std::string current_scheduler_name;
    GlibSource reconnect_source;
};

gboolean IcecreamScheduler::scheduler_process(gint, GIOCondition, gpointer user_data)
{
    auto *self = static_cast<IcecreamScheduler*>(user_data);

    while (!self->scheduler->read_a_bit() || self->scheduler->has_msg()) {
        if (!self->process_message(self->scheduler.get()))
            break;
    }

    if (self->scheduler->at_eof())
        self->reconnect(self->current_net_name, self->current_scheduler_name);

    return TRUE;
}

void IcecreamScheduler::discover_scheduler(std::string const &netname, std::string const &schedname)
{
    if (scheduler)
        return;

    auto discover = std::make_unique<DiscoverSched>(netname, 2, schedname);

    scheduler.reset(discover->try_get_scheduler());

    Host::hosts.clear();
    Job::clearAll();
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

            poll(&pfd, 1, 500);

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

bool IcecreamScheduler::process_message(MsgChannel *sched)
{
    std::unique_ptr<Msg> msg(sched->get_msg());
    if (!msg)
        return false;

    switch (*msg) {
    case Msg::MON_LOCAL_JOB_BEGIN: {
        auto *m = dynamic_cast<MonLocalJobBeginMsg*>(msg.get());
        Job::createLocal(m->job_id, m->hostid, m->file);
        break;
    }
    case Msg::JOB_LOCAL_DONE: {
        auto *m = dynamic_cast<JobLocalDoneMsg*>(msg.get());
        Job::remove(m->job_id);
        break;
    }
    case Msg::MON_JOB_BEGIN: {
        auto *m = dynamic_cast<MonJobBeginMsg*>(msg.get());
        Job::createRemote(m->job_id, m->hostid);
        break;
    }
    case Msg::MON_JOB_DONE: {
        auto *m = dynamic_cast<MonJobDoneMsg*>(msg.get());
        Job::remove(m->job_id);
        break;
    }
    case Msg::MON_GET_CS: {
        auto *m = dynamic_cast<MonGetCSMsg*>(msg.get());
        Job::createPending(m->job_id, m->clientid, m->filename);
        break;
    }
    case Msg::MON_STATS: {
        auto *m = dynamic_cast<MonStatsMsg*>(msg.get());
        auto host = Host::create(m->hostid);

        std::stringstream ss(m->statmsg);
        std::string key;
        std::string value;
        bool alive = false;

        while (std::getline(ss, key, ':') && std::getline(ss, value)) {
            if (key == "Name")
                alive = true;

            host->attr[key] = value;
        }

        if (!alive)
            Host::remove(m->hostid);

        if (interface)
            interface->triggerRedraw();
        break;
    }
    case Msg::END:
        reconnect(current_net_name, current_scheduler_name);
        break;
    default:
        break;
    }

    return true;
}

gboolean IcecreamScheduler::on_reconnect_timer(gpointer user_data)
{
    auto *self = static_cast<IcecreamScheduler*>(user_data);

    self->reconnect(self->current_net_name, self->current_scheduler_name);

    return TRUE;
}

void IcecreamScheduler::reconnect(std::string const &netname, std::string const &schedname)
{
    scheduler.reset();
    scheduler_source.remove();

    if (interface)
        interface->suspend();

    discover_scheduler(netname, schedname);

    if (scheduler) {
        scheduler_source.set(g_unix_fd_add(scheduler->fd, G_IO_IN, scheduler_process, this));
        reconnect_source.remove();
    } else {
        reconnect_source.set(g_timeout_add(5000, on_reconnect_timer, this));
    }

    if (interface) {
        interface->resume();
        interface->triggerRedraw();
    }
}


std::unique_ptr<Scheduler> connect_to_scheduler(std::string const &netname, std::string const &schedname)
{
    return std::make_unique<IcecreamScheduler>(netname, schedname);
}

