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

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <glib.h>

extern bool all_expanded;

struct Host;

struct Job {
    typedef std::map<uint32_t, std::shared_ptr<Job> > Map;
    typedef std::vector<std::shared_ptr<Job> > List;

    virtual ~Job() {}

    const uint32_t id;
    uint32_t clientid = 0;
    uint32_t hostid = 0;
    bool active = false;
    bool is_local = false;
    std::string filename;
    size_t host_slot = SIZE_MAX;
    guint64 start_time = 0;

    std::shared_ptr<Host> getClient() const;
    std::shared_ptr<Host> getHost() const;

    static std::shared_ptr<Job> find(uint32_t id);
    static void remove(uint32_t id);

    static void createLocal(uint32_t id, uint32_t hostid, std::string const& filename);
    static void createPending(uint32_t id, uint32_t clientid, std::string const& filename);
    static void createRemote(uint32_t id, uint32_t hostid);
    static void clearAll();

    static Map allJobs;
    static Map pendingJobs;
    static Map activeJobs;
    static Map localJobs;
    static Map remoteJobs;

protected:
    explicit Job(uint32_t jobid) : id(jobid) {}

private:
    static std::shared_ptr<Job> create(uint32_t id);
    static void removeFromMap(Map &map, uint32_t id);
    static void removeTypes(uint32_t id);
};

struct Host {
    typedef std::map<uint32_t, std::shared_ptr<Host> > Map;
    typedef std::vector<std::shared_ptr<Host> > List;

    typedef std::map<std::string, std::string> Attributes;

    virtual ~Host() {}

    const uint32_t id;
    Attributes attr;
    bool expanded;
    bool highlighted = false;
    size_t current_position = 0;
    int total_out = 0;
    int total_in = 0;
    int total_local = 0;

    Job::Map getPendingJobs() const;
    Job::Map getActiveJobs() const;
    Job::Map getCurrentJobs() const;

    std::string getName() const
    {
        return getStringAttr("Name");
    }

    size_t getMaxJobs() const
    {
        return getNumberAttr<size_t>("MaxJobs");
    }

    double getSpeed() const
    {
        return getNumberAttr<double>("Speed");
    }

    bool getNoRemote() const
    {
        return getBoolAttr("NoRemote");
    }

    int getColor() const;

    static void addColor(int ident)
    {
        host_color_ids.push_back(ident);
    }

    static void clearColors()
    {
        host_color_ids.clear();
    }

    static void setLocalhostColor(int ident)
    {
        localhost_color_id = ident;
    }

    static std::shared_ptr<Host> create(uint32_t id);
    static std::shared_ptr<Host> find(uint32_t id);
    static void remove(uint32_t id);
    static Map hosts;

protected:
    explicit Host(uint32_t hostid) : id(hostid), expanded(all_expanded)
        {}
private:
    std::string getStringAttr(std::string const &name, std::string const &dflt = "") const
    {
        auto const i = attr.find(name);
        if (i == attr.end())
            return dflt;
        return i->second;
    }

    template <typename T>
    T getNumberAttr(std::string const &name, T dflt = 0) const
    {
        auto const i = attr.find(name);
        if (i == attr.end())
            return dflt;

        T val;
        std::istringstream(i->second) >> val;
        return val;
    }

    bool getBoolAttr(std::string const &name, bool dflt = false) const
    {
        auto const i = attr.find(name);
        if (i == attr.end())
            return dflt;

        bool val;
        std::istringstream(i->second) >> std::boolalpha >> val;
        return val;
    }

    size_t hashName() const;

    static std::vector<int> host_color_ids;
    static int localhost_color_id;
};

class Scheduler {
public:
    Scheduler() {}
    virtual ~Scheduler() {}

    virtual void onInput(int) {};

    virtual std::string getNetName() const = 0;
    virtual std::string getSchedulerName() const = 0;
};

class UserInterface {
public:
    UserInterface() {}
    virtual ~UserInterface() {}

    virtual void triggerRedraw() = 0;
    virtual int processInput() = 0;
    virtual int getInputFd() = 0;
    virtual void suspend() = 0;
    virtual void resume() = 0;
    virtual void set_anonymize(bool) = 0;
};

class GlibSource {
public:
    GlibSource(): m_source(0) {}
    explicit GlibSource(guint source) : m_source(source) {}

    GlibSource& operator=(const GlibSource&) = delete;

    virtual ~GlibSource()
    {
        remove();
    }

    guint get() const { return m_source; }

    void set(guint source)
    {
        remove();
        m_source = source;
    }

    void remove()
    {
        if (m_source) {
            g_source_remove(m_source);
            m_source = 0;
        }
    }

    void clear()
    {
        m_source = 0;
    }

private:
    guint m_source;
};

extern GMainLoop *main_loop;
extern int total_remote_jobs;
extern int total_local_jobs;
extern std::unique_ptr<Scheduler> scheduler;
extern std::unique_ptr<UserInterface> interface;

