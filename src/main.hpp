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

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <glib.h>

extern bool all_expanded;

struct Job {
    uint32_t clientid = 0;
    uint32_t hostid = 0;
    bool active = false;
    bool is_local = false;
    std::string filename;
    int host_slot = -1;
    guint64 start_time = 0;
};

struct Host {
    Host() : expanded(all_expanded)
        {}
    typedef std::map<std::string, std::string> Attributes;

    Attributes attr;
    bool expanded;
    bool highlighted = false;
    size_t current_position = 0;
    int total_out = 0;
    int total_in = 0;
    int total_local = 0;

    std::string getName() const
    {
        return getStringAttr("Name");
    }

    int getMaxJobs() const
    {
        return getIntAttr("MaxJobs");
    }

    bool getNoRemote() const
    {
        return getBoolAttr("NoRemote");
    }

    int getColor() const
    {
        return host_color_ids[std::hash<std::string>{}(getName()) % host_color_ids.size()];
    }

    static void addColor(int ident)
    {
        host_color_ids.push_back(ident);
    }

private:
    std::string getStringAttr(std::string const &name, std::string const &dflt = "") const
    {
        auto const i = attr.find(name);
        if (i == attr.end())
            return dflt;
        return i->second;
    }

    int getIntAttr(std::string const &name, int dflt = 0) const
    {
        auto const i = attr.find(name);
        if (i == attr.end())
            return dflt;

        int val;
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

    static std::vector<int> host_color_ids;
};

extern GMainLoop *main_loop;
extern std::map<uint32_t, Job> jobs;
extern int total_remote_jobs;
extern int total_local_jobs;
extern std::map<uint32_t, Host> hosts;
extern std::string current_scheduler_name;
extern std::string current_net_name;

