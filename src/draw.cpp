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

#include <algorithm>
#include <cstdint>
#include <string>
#include <map>
#include <memory>
#include <unordered_set>
#include <iomanip>

#include <glib.h>
#include <glib-unix.h>
#include <ncurses.h>

#include "main.hpp"
#include "draw.hpp"

class Column;

class NCursesInterface: public UserInterface {
public:
    NCursesInterface();
    virtual ~NCursesInterface();

    virtual void triggerRedraw() override;
    virtual int processInput() override;

    virtual int getInputFd() override
    {
        return STDIN_FILENO;
    }

    void print_job_graph(Job::Map const &jobs, int max_jobs) const;

private:
    static gboolean on_idle_draw(gpointer user_data);
    static gboolean on_winch_signal(gpointer user_data);
    static gboolean on_redraw_timer(gpointer user_data);

    void doRender();
    void doRedraw();
    int assign_color(int fg, int bg);

    std::vector<uint32_t> host_order;
    std::vector<std::unique_ptr<Column> > columns;
    GlibSource idle_source;
    GlibSource sigwinch_source;
    GlibSource redraw_source;
    int header_color;
    int expand_color;
    int highlight_color;
    uint32_t current_host = 0;
    size_t current_col = 0;
    bool sort_reversed = false;
    bool track_jobs = false;
    int next_color_id = 1;
};

struct HostCache {
    typedef std::vector<std::shared_ptr<HostCache> > List;

    std::shared_ptr<Host> host;
    Job::Map pending_jobs;
    Job::Map active_jobs;
    Job::Map current_jobs;
};

class Attr {
    public:
        Attr(int a, bool on=true) : m_attr(a), m_on(false)
        {
            setOn(on);
        }

        ~Attr()
        {
            off();
        }

        bool getOn() const { return m_on; }
        int getAttr() const { return m_attr; }

        void setOn(bool on)
        {
            if (m_on != on) {
                if (on)
                    attron(m_attr);
                else
                    attroff(m_attr);
                m_on = on;
            }

        }

        void toggle()
        {
            setOn(!m_on);
        }

        void on()
        {
            setOn(true);
        }

        void off()
        {
            setOn(false);
        }

    private:
        int m_attr;
        bool m_on;
};

class Column {
    public:
        typedef bool (*Compare)(const std::shared_ptr<const HostCache>, const std::shared_ptr<const HostCache>);

        virtual ~Column() {}

        virtual size_t getWidth(HostCache::List const &hosts) const
        {
            size_t max_width = std::max(getHeader().size(), getMinWidth());

            for (auto const h : hosts) {
                std::string s = getOutputString(h);
                max_width = std::max(max_width, s.size());
            }
            return max_width;
        }

        virtual std::string getHeader() const = 0;

        virtual void output(NCursesInterface const*, int row, const std::shared_ptr<const HostCache> host) const
        {
            move(row, m_column);
            addstr(getOutputString(host).c_str());
        }

        void setColumn(int col)
        {
            m_column = col;
        }

        int getColumn() const
        {
            return m_column;
        }

        virtual Compare get_compare() const = 0;

    protected:
        Column() {}

        virtual std::string getOutputString(const std::shared_ptr<const HostCache>) const
        {
            return "";
        }

        virtual size_t getMinWidth() const
        {
            return 0;
        }

        int m_column;
};

class NameColumn: public Column {
    public:
        NameColumn() : Column() {}
        virtual ~NameColumn() {}

        virtual std::string getHeader() const override
        {
            return "NAME";
        }

        virtual void output(NCursesInterface const*, int row, const std::shared_ptr<const HostCache> host) const override
        {
            move(row, m_column);
            {
                Attr attr(COLOR_PAIR(host->host->getColor()) | ( host->host->getNoRemote() ? A_UNDERLINE : 0 ));
                addstr(getOutputString(host).c_str());
            }
        }

        virtual Compare get_compare() const override
        {
            return compare;
        }

    protected:
        virtual std::string getOutputString(const std::shared_ptr<const HostCache> host) const override
        {
            std::ostringstream ss;
            ss << host->host->getName();
            return ss.str();
        }

    private:
        static bool compare(const std::shared_ptr<const HostCache> a, const std::shared_ptr<const HostCache> b)
        {
            return a->host->getName() < b->host->getName();
        }
};

class JobsColumn: public Column {
    public:
        JobsColumn() : Column() {}
        virtual ~JobsColumn() {}

        virtual size_t getWidth(HostCache::List const &hosts) const override
        {
            size_t max_width = getHeader().size();

            for (auto const h : hosts)
                max_width = std::max(max_width, static_cast<size_t>(h->host->getMaxJobs()) + 2);

            return max_width;
        }

        virtual std::string getHeader() const override
        {
            return "JOBS";
        }

        virtual void output(NCursesInterface const* interface, int row, const std::shared_ptr<const HostCache> host) const override
        {
            move(row, m_column);
            interface->print_job_graph(host->current_jobs, host->host->getMaxJobs());
        }

        virtual Compare get_compare() const override
        {
            return compare;
        }

    private:
        static bool compare(const std::shared_ptr<const HostCache> a, const std::shared_ptr<const HostCache> b)
        {
            return a->current_jobs.size() < b->current_jobs.size();
        }
};

#define SIMPLE_COLUMN(_name, _header, _attr, _min_width) \
    class _name: public Column { \
        public: \
            _name() : Column() {} \
            virtual ~_name() {} \
            virtual std::string getHeader() const override { return _header; } \
            virtual Compare get_compare() const override { return compare; } \
        protected: \
            virtual std::string getOutputString(const std::shared_ptr<const HostCache> host) const override \
            { \
                std::ostringstream ss; ss << std::setprecision(6) << host->_attr; return ss.str(); \
            } \
            virtual size_t getMinWidth() const override { return _min_width; } \
        private: \
            static bool compare(const std::shared_ptr<const HostCache> a, const std::shared_ptr<const HostCache> b) \
            { \
                return a->_attr < b->_attr; \
            } \
    }

SIMPLE_COLUMN(InJobsColumn, "IN", host->total_in, 5);
SIMPLE_COLUMN(OutJobsColumn, "OUT", host->total_out, 5);
SIMPLE_COLUMN(ActiveJobsColumn, "ACTIVE", active_jobs.size(), 0);
SIMPLE_COLUMN(PendingJobsColumn, "PENDING", pending_jobs.size(), 0);
SIMPLE_COLUMN(LocalJobsColumn, "LOCAL", host->total_local, 5);
SIMPLE_COLUMN(CurrentJobsColumn, "CUR", current_jobs.size(), 0);
SIMPLE_COLUMN(MaxJobsColumn, "MAX", host->getMaxJobs(), 0);
SIMPLE_COLUMN(IDColumn, "ID", host->id, 0);
SIMPLE_COLUMN(SpeedColumn, "SPEED", host->getSpeed(), 0);

static const std::string local_job_track("abcdefghijklmnopqrstuvwxyz");
static const std::string remote_job_track("ABCDEFGHIJKLMNOPQRSTUVWXYZ");

int NCursesInterface::processInput()
{
    int c = getch();
    auto cur_host = Host::find(current_host);
    bool consumed = true;

    if (cur_host)
        cur_host->highlighted = false;
    else
        current_host = 0;

    switch(c) {
    case KEY_UP:
    case 'k':
        if (cur_host) {
            if (cur_host->current_position > 0) {
                current_host = host_order[cur_host->current_position - 1];
            }
        } else {
            current_host = host_order[0];
        }
        break;

    case KEY_DOWN:
    case 'j':
        if (cur_host) {
            if (cur_host->current_position < host_order.size() - 1) {
                current_host = host_order[cur_host->current_position + 1];
            }
        } else {
            current_host = host_order[0];
        }
        break;

    case KEY_LEFT:
    case 'h':
        if (current_col > 0)
            current_col--;
        break;

    case KEY_RIGHT:
    case 'l':
        if (current_col < columns.size() - 1)
            current_col++;
        break;

    case '\t':
        current_col = (current_col + 1) % columns.size();
        break;

    case 'T':
        track_jobs = !track_jobs;
        break;

    case ' ':
        if (cur_host)
            cur_host->expanded = !cur_host->expanded;
        break;

    case 'a':
        all_expanded = !all_expanded;
        for (auto h : Host::hosts)
            h.second->expanded = all_expanded;
        break;

    case 'r':
        sort_reversed = !sort_reversed;
        break;

    case 'q':
        g_main_loop_quit(main_loop);
        break;

    default:
        consumed = false;
        break;
    }

    if (current_host)
        Host::hosts.at(current_host)->highlighted = true;

    triggerRedraw();
    return consumed ? 0 : c;
}

void NCursesInterface::print_job_graph(Job::Map const &jobs, int max_jobs) const
{
    addch('[');

    int cnt = 0;

    for (auto const j : jobs) {
        if (!j.second->active)
            continue;

        int color = 0;
        auto const h = j.second->getClient();
        if (h)
            color = h->getColor();

        Attr clr(COLOR_PAIR(color));
        char c;
        if (track_jobs)  {
            std::string const *s;
            if (j.second->is_local)
                s = &local_job_track;
            else
                s = &remote_job_track;
            c = s->at(j.first % s->size());
        } else if (j.second->is_local) {
            c = '%';
        } else {
            c = '=';
        }
        addch(c);

        cnt++;
    }

    for (int i = cnt; i < max_jobs; ++i)
        addch(' ');

    addch(']');
}

gboolean NCursesInterface::on_idle_draw(gpointer user_data)
{
    auto *self = static_cast<NCursesInterface*>(user_data);
    self->doRedraw();
    self->idle_source.clear();
    return FALSE;
}

gboolean NCursesInterface::on_winch_signal(gpointer user_data)
{
    auto *self = static_cast<NCursesInterface*>(user_data);
    self->triggerRedraw();
    return TRUE;
}

gboolean NCursesInterface::on_redraw_timer(gpointer user_data)
{
    auto *self = static_cast<NCursesInterface*>(user_data);
    self->triggerRedraw();
    return TRUE;
}

int NCursesInterface::assign_color(int fg, int bg)
{
    int ident = next_color_id++;
    init_pair(ident, fg, bg);
    return ident;
}

void NCursesInterface::doRender()
{
    int total_job_slots = 0;
    int avail_servers = 0;

    int screen_rows;
    int screen_cols;
    std::unordered_set<uint32_t> used_hosts;

    getmaxyx(stdscr, screen_rows, screen_cols);

    HostCache::List host_cache;

    for (auto j : Job::allJobs) {
        if (j.second->getHost()) {
            used_hosts.insert(j.second->getHost()->id);
        }
    }

    for (auto const h : Host::hosts) {
        if (!h.second->getNoRemote()) {
            avail_servers++;
            total_job_slots += h.second->getMaxJobs();
        }
        auto c = std::make_shared<HostCache>();
        c->host = h.second;
        c->pending_jobs = c->host->getPendingJobs();
        c->active_jobs = c->host->getActiveJobs();
        c->current_jobs = c->host->getCurrentJobs();

        host_cache.push_back(c);
    }

    int row = 0;
    #define next_row() if (++row >= screen_rows) return

    move(row, 0);
    {
        Attr bold(A_BOLD);
        addstr("Scheduler: ");
    }
    addstr(scheduler->getSchedulerName().c_str());

    {
        Attr bold(A_BOLD);
        addstr(" Netname: ");
    }
    addstr(scheduler->getNetName().c_str());
    next_row();


    move(row, 0);
    {
        Attr bold(A_BOLD);
        addstr("Servers: ");
    }
    {
        std::ostringstream ss;
        ss << "Total:" << Host::hosts.size() << " Available:" << avail_servers << " Active:" << used_hosts.size();
        addstr(ss.str().c_str());
    }
    next_row();

    move(row, 0);
    {
        Attr bold(A_BOLD);
        addstr("Total: ");
    }
    {
        std::ostringstream ss;
        ss << "Remote:" << total_remote_jobs << " Local:" << total_local_jobs;
        addstr(ss.str().c_str());
    }
    next_row();
    move(row, 0);
    {
        Attr bold(A_BOLD);
        addstr("Jobs: ");
    }
    {
        std::ostringstream ss;
        ss << "Maxiumum:" << total_job_slots << " Active:" << Job::activeJobs.size() <<
            " Local:" << Job::localJobs.size() << " Pending:" << Job::pendingJobs.size();
        addstr(ss.str().c_str());
    }
    next_row();

    move(row, 6);
    print_job_graph(Job::allJobs, total_job_slots);
    next_row();
    next_row();

    move(row, 0);
    {
        Attr color(COLOR_PAIR(header_color));
        Attr highlight(COLOR_PAIR(highlight_color), false);

        add_wch(sort_reversed ? WACS_UARROW : WACS_DARROW);
        for (int i = 1; i < screen_cols; i++)
            addch(' ');

        int col = 2;
        for (size_t i = 0; i < columns.size(); i++) {
            auto &c = columns[i];
            size_t width = c->getWidth(host_cache);
            c->setColumn(col);

            if (current_col == i) {
                color.off();
                highlight.on();
            }

            mvprintw(row, col, "%-*s", width, c->getHeader().c_str());

            if (current_col == i) {
                highlight.off();
                color.on();
            }

            col += width + 1;
        }
    }
    next_row();

    if (current_col < columns.size()) {
        auto compare = columns[current_col]->get_compare();
        if (sort_reversed)
            std::sort(host_cache.rbegin(), host_cache.rend(), compare);
        else
            std::sort(host_cache.begin(), host_cache.end(), compare);
    }

    host_order.clear();

    for (auto cache: host_cache) {
        auto &host = cache->host;
        if (!host->id)
            continue;

        host->current_position = host_order.size();
        host_order.push_back(host->id);

        move(row, 0);
        {
            Attr color(COLOR_PAIR(host->highlighted ? highlight_color : expand_color));
            addch(host->expanded ? '-' : '+');
        }

        for (auto const &c: columns)
            c->output(this, row, cache);

        if (host->expanded) {
            for (size_t i = 0; i < host->getMaxJobs(); i++) {
                next_row();
                move(row, 2);
                {
                    Attr bold(A_BOLD);
                    printw("Job %d: ", i + 1);
                }

                std::shared_ptr<Job> job;

                // Find assigned job
                for (auto j : cache->current_jobs) {
                    if (j.second->host_slot == i) {
                        job = j.second;
                        break;
                    }
                }

                // If no existing job was found, assign a new one
                if (!job) {
                    for (auto j : cache->current_jobs) {
                        if (j.second->host_slot == SIZE_MAX) {
                            job = j.second;
                            j.second->host_slot = i;
                            break;
                        }
                    }
                }

                if (job) {
                    printw("(%5.1lfs) ", (double)((g_get_monotonic_time() - job->start_time) / 1000000.0));

                    int color = 0;
                    auto const h = job->getClient();
                    if (h)
                        color = h->getColor();

                    Attr clr(COLOR_PAIR(color));
                    if (job->filename.empty())
                        addstr("<unknown>");
                    else
                        addstr(job->filename.c_str());
                }
            }

            size_t width = 0;
            for (auto const &a : host->attr) {
                width = std::max(width, a.first.size());
            }

            for (auto const &a : host->attr) {
                next_row();
                move(row, 2);
                {
                    Attr bold(A_BOLD);
                    addstr(a.first.c_str());
                }
                move(row, 2 + width + 1);
                addstr(a.second.c_str());
            }
        }
        next_row();
    }
}

void NCursesInterface::doRedraw()
{
    clear();
    doRender();
    refresh();
}

void NCursesInterface::triggerRedraw()
{
    if (!idle_source.get())
        idle_source.set(g_idle_add(reinterpret_cast<GSourceFunc>(on_idle_draw), this));
}

NCursesInterface::NCursesInterface() :
    UserInterface()
{
    initscr();

    cbreak();
    use_default_colors();
    start_color();
    curs_set(0);
    noecho();
    nonl();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);

    Host::addColor(assign_color(COLOR_RED, -1));
    Host::addColor(assign_color(COLOR_GREEN, -1));
    Host::addColor(assign_color(COLOR_YELLOW, -1));
    Host::addColor(assign_color(COLOR_BLUE, -1));
    Host::addColor(assign_color(COLOR_MAGENTA, -1));
    Host::addColor(assign_color(COLOR_CYAN, -1));
    Host::addColor(assign_color(COLOR_WHITE, -1));

    header_color = assign_color(COLOR_BLACK, COLOR_GREEN);
    expand_color = assign_color(COLOR_GREEN, -1);
    highlight_color = assign_color(COLOR_BLACK, COLOR_CYAN);

    if (glib_check_version(2, 54, 0) == NULL)
        sigwinch_source.set(g_unix_signal_add(SIGWINCH, reinterpret_cast<GSourceFunc>(on_winch_signal), this));

    columns.emplace_back(std::make_unique<IDColumn>());
    columns.emplace_back(std::make_unique<NameColumn>());
    columns.emplace_back(std::make_unique<InJobsColumn>());
    columns.emplace_back(std::make_unique<CurrentJobsColumn>());
    columns.emplace_back(std::make_unique<MaxJobsColumn>());
    columns.emplace_back(std::make_unique<JobsColumn>());
    columns.emplace_back(std::make_unique<OutJobsColumn>());
    columns.emplace_back(std::make_unique<LocalJobsColumn>());
    columns.emplace_back(std::make_unique<ActiveJobsColumn>());
    columns.emplace_back(std::make_unique<PendingJobsColumn>());
    columns.emplace_back(std::make_unique<SpeedColumn>());

    redraw_source.set(g_timeout_add(1000, on_redraw_timer, this));

    triggerRedraw();
}

NCursesInterface::~NCursesInterface()
{
    endwin();
}

std::unique_ptr<UserInterface> create_ncurses_interface()
{
    return std::make_unique<NCursesInterface>();
}
