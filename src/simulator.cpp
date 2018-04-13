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

#include <random>
#include <glib.h>

#include "simulator.hpp"
#include "main.hpp"

#define MAX_HOSTS (10)
#define MAX_JOBS (100)
#define MAX_HOST_JOBS (20)
#define SIM_INTERVAL (20)

class Simulator: public Scheduler {
public:
    Simulator(std::uint_fast32_t seed);
    virtual ~Simulator() {}

    virtual std::string getNetName() const override { return "ICECREAM"; }
    virtual std::string getSchedulerName() const override { return "simulator"; }

private:
    static gboolean process_simulator(gpointer user_data);

    void onTimer();

    void addHost();
    void removeHost();
    void chooseSourceHost();
    std::shared_ptr<Host> getSourceHost();

    void addPendingJob();
    void activateJob();
    void addLocalJob();
    void removeJob();

    template<typename T>
    std::shared_ptr<T> chooseRandom(std::map<uint32_t, std::shared_ptr<T> > const &);

    Host::Map getAvailableHosts(uint32_t exclude = 0) const;

    std::minstd_rand random_generator;
    GlibSource timer_source;
    uint32_t next_host_id = 1;
    uint32_t next_job_id = 1;
    uint32_t source_host = 0;

    struct Action {
        uint32_t weight;
        void (Simulator::*action)();
    };

    static const Action actionTable[];
};

const Simulator::Action Simulator::actionTable[] = {
    {   5, &Simulator::addPendingJob },
    {   1, &Simulator::addLocalJob },
    {   5, &Simulator::activateJob },
    {   5, &Simulator::removeJob },
    {   1, &Simulator::chooseSourceHost },
};

gboolean Simulator::process_simulator(gpointer user_data)
{
    auto *self = static_cast<Simulator*>(user_data);
    self->onTimer();
    return TRUE;
}

Simulator::Simulator(std::uint_fast32_t seed):
    Scheduler(), random_generator(seed)
{
    for (int i = 0; i < MAX_HOSTS; i++)
        addHost();

    timer_source.set(g_timeout_add(SIM_INTERVAL, process_simulator, this));
}

template<typename T>
std::shared_ptr<T> Simulator::chooseRandom(std::map<uint32_t, std::shared_ptr<T> > const &map)
{
    std::vector<std::shared_ptr<T> > items;
    for (auto const m : map)
        items.push_back(m.second);

    if (!items.size())
        return nullptr;

    std::uniform_int_distribution<> dis(0, items.size() - 1);
    return items[dis(random_generator)];
}

void Simulator::addHost()
{
    auto h = Host::create(next_host_id++);
    {
        std::ostringstream ss;
        ss << "Host " << h->id;
        h->attr["Name"] = ss.str();
    }

    // Poor man's normal distribution
    h->attr["MaxJobs"] = std::to_string(
            random_generator() % (MAX_HOST_JOBS / 2) + random_generator() % (MAX_HOST_JOBS / 2 - 1) + 1
            );
    h->attr["NoRemote"] = ((rand() % 10) == 0 ? "true" : "false");
    h->attr["Platform"] = "x86_64";
    h->attr["Speed"] = "100.000";
}

void Simulator::removeHost()
{
    auto host = chooseRandom(Host::hosts);
    if (host)
        Host::remove(host->id);
}

void Simulator::chooseSourceHost()
{
    auto host = chooseRandom(Host::hosts);
    if (host)
        source_host = host->id;
    else
        source_host = 0;
}

std::shared_ptr<Host> Simulator::getSourceHost()
{
    auto host = Host::find(source_host);
    if (!host) {
        chooseSourceHost();
        host = Host::find(source_host);
    }

    return host;
}

void Simulator::onTimer()
{
    uint32_t total_weight = 0;
    for (auto const &a : actionTable)
        total_weight += a.weight;

    std::uniform_int_distribution<> dis(0, total_weight);

    uint32_t r = dis(random_generator);

    for (auto const &a : actionTable) {
        if (r < a.weight) {
            (this->*a.action)();
            break;
        }
        r -= a.weight;
    }
}

void Simulator::addPendingJob()
{
    // Don't add a pending job if there are no free executors
    auto avail = getAvailableHosts();
    auto host = getSourceHost();
    if (host && avail.size()) {
        uint32_t id = next_job_id++;
        std::ostringstream ss;
        ss << "Job_" << id << ".c";

        Job::createPending(id, host->id, ss.str());
    }
}

void Simulator::activateJob()
{
    if (Job::pendingJobs.size()) {
        auto const job = chooseRandom(Job::pendingJobs);
        auto host = chooseRandom(getAvailableHosts(job->clientid));
        if (host)
            Job::createRemote(job->id, host->id);
    }
}

void Simulator::addLocalJob()
{
    auto host = getSourceHost();
    if (host && host->getCurrentJobs().size() < host->getMaxJobs()) {
        uint32_t id = next_job_id++;
        std::ostringstream ss;
        ss << "Job_" << id << ".c";

        Job::createLocal(id, host->id, ss.str());
    }
}

void Simulator::removeJob()
{
    auto job = chooseRandom(Job::activeJobs);
    if (job)
        Job::remove(job->id);

    // Assign a new job if possible
    activateJob();
}

Host::Map Simulator::getAvailableHosts(uint32_t except) const
{
    Host::Map result;

    for (auto h : Host::hosts) {
        if (h.first != except && h.second->getCurrentJobs().size() < h.second->getMaxJobs())
            result[h.first] = h.second;
    }

    return result;
}

std::unique_ptr<Scheduler> create_simulator(std::uint_fast32_t seed)
{
    return std::make_unique<Simulator>(seed);
}

