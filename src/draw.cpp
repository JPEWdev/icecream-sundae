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

#include <algorithm>
#include <cstdint>
#include <functional>
#include <string>
#include <iomanip>
#include <map>
#include <memory>
#include <unordered_set>
#include <iomanip>

#include <assert.h>
#include <glib.h>
#include <glib-unix.h>
#include <math.h>
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

    virtual void suspend() override;
    virtual void resume() override;

    virtual void set_anonymize(bool a) override
    {
        anonymize = a;
    }

    bool get_anonymize() const
    {
        return anonymize;
    }

    void print_job_graph(Job::Map const &jobs, int max_graph_jobs, int max_host_jobs) const;

private:
    static gboolean on_idle_draw(gpointer user_data);
    static gboolean on_redraw_timer(gpointer user_data);

    void init();
    void doRender();
    void doRedraw();
    int assign_color(int fg, int bg);

    std::vector<uint32_t> host_order;
    std::vector<std::shared_ptr<Column> > columns;
    GlibSource idle_source;
    GlibSource redraw_source;
    int header_color;
    int expand_color;
    int highlight_color;
    uint32_t current_host = 0;
    size_t current_col = 0;
    bool sort_reversed = false;
    int next_color_id = 1;
    bool anonymize = false;
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
        using Compare = std::function<bool(const HostCache&, const HostCache&)>;

        virtual ~Column() {}

        virtual std::pair<size_t, size_t> getWidthConstraint(HostCache::List const &hosts) const
        {
            size_t min_width = std::max(getHeader().size(), getMinWidth());

            for (auto const h : hosts) {
                std::string s = getOutputString(h);
                min_width = std::max(min_width, s.size());
            }
            return std::pair<size_t, size_t>(min_width, min_width);
        }

        virtual std::string getHeader() const = 0;

        virtual void output(int row, int column, int /* width */, const std::shared_ptr<const HostCache> &host) const
        {
            move(row, column);
            addstr(getOutputString(host).c_str());
        }

        virtual Compare get_compare() const = 0;

    protected:
        explicit Column(const NCursesInterface *const interface): m_interface(interface) {}

        virtual std::string getOutputString(const std::shared_ptr<const HostCache> &) const
        {
            return "";
        }

        virtual size_t getMinWidth() const
        {
            return 0;
        }

        const NCursesInterface *const m_interface;
};

class NameColumn: public Column {
    public:
        explicit NameColumn(const NCursesInterface *const interface): Column(interface) {}
        virtual ~NameColumn() {}

        virtual std::string getHeader() const override
        {
            return "NAME";
        }

        virtual void output(int row, int column, int /* width */, const std::shared_ptr<const HostCache> &host) const override
        {
            move(row, column);
            {
                Attr attr(COLOR_PAIR(host->host->getColor()) | ( host->host->getNoRemote() ? A_UNDERLINE : 0 ));
                addstr(getOutputString(host).c_str());
            }
        }

        Compare get_compare() const override {
            return [](const HostCache &a, const HostCache &b) {
                return a.host->getName() < b.host->getName();
            };
        }

    protected:
        virtual std::string getOutputString(const std::shared_ptr<const HostCache> &host) const override
        {
            std::ostringstream ss;
            if (m_interface->get_anonymize()) {
                ss << std::hex;
                ss << "Host " << std::hash<std::string>{}(host->host->getName());
            } else {
                ss << host->host->getName();
            }
            return ss.str();
        }
};

class JobsColumn: public Column {
    public:
        explicit JobsColumn(const NCursesInterface *const interface): Column(interface) {}
        virtual ~JobsColumn() {}

        virtual std::pair<size_t, size_t> getWidthConstraint(HostCache::List const &hosts) const override
        {
            size_t min_width = getHeader().size();
            size_t desired_width = min_width;

            for (auto const h : hosts)
                desired_width = std::max(desired_width, static_cast<size_t>(h->host->getMaxJobs()) + 2);

            return std::pair<size_t, size_t>(min_width, desired_width);
        }

        virtual std::string getHeader() const override
        {
            return "JOBS";
        }

        virtual void output(int row, int column, int width, const std::shared_ptr<const HostCache> &host) const override
        {
            move(row, column);
            m_interface->print_job_graph(host->current_jobs, host->host->getMaxJobs(), width);
        }

        Compare get_compare() const override {
            return [](const HostCache &a, const HostCache &b) {
                return a.current_jobs.size() < b.current_jobs.size();
            };
        }
};

class SimpleColumn : public Column {
public:
    SimpleColumn(const NCursesInterface *nc,
                 const std::string& header,
                 size_t min_width,
                 std::function<void(std::ostream&, const HostCache&)> stream,
                 std::function<bool(const HostCache&, const HostCache&)> compare)
        : Column(nc), _header(header), _min_width(min_width), _stream(stream), _compare(compare) {}

    template <typename Extract>
    SimpleColumn(const NCursesInterface *nc,
                 const std::string& header,
                 size_t min_width,
                 const Extract& extract)
    : SimpleColumn(nc,
                   header,
                   min_width,
                   [=](std::ostream& ss, const HostCache& host) {
                       ss << extract(host);
                   },
                   [=](const HostCache& a, const HostCache& b) {
                       return extract(a) < extract(b);
                   }) {}

    std::string getHeader() const override { return _header; }

    Compare get_compare() const override {
        return _compare;
    }
protected:
    std::string getOutputString(const std::shared_ptr<const HostCache> &host) const override {
        std::ostringstream ss;
        ss << std::setprecision(6);
        _stream(ss, *host);
        return ss.str();
    }

    size_t getMinWidth() const override { return _min_width; }

    std::string _header;
    size_t _min_width;
    std::function<void(std::ostream&, const HostCache&)> _stream;
    std::function<bool(const HostCache&,const HostCache&)> _compare;
};

struct InJobsColumn : SimpleColumn {
    InJobsColumn(const NCursesInterface *nc)
        : SimpleColumn(nc, "IN", 5, [](const HostCache& hc) { return hc.host->total_in; }) {}
};

struct OutJobsColumn : SimpleColumn {
    OutJobsColumn(const NCursesInterface *nc)
        : SimpleColumn(nc, "OUT", 5, [](const HostCache& hc) { return hc.host->total_out; }) {}
};

struct ActiveJobsColumn : SimpleColumn {
    ActiveJobsColumn(const NCursesInterface *nc)
        : SimpleColumn(nc, "ACTIVE", 0, [](const HostCache& hc) { return hc.active_jobs.size(); }) {}
};

struct PendingJobsColumn : SimpleColumn {
    PendingJobsColumn(const NCursesInterface *nc)
        : SimpleColumn(nc, "PENDING", 0, [](const HostCache& hc) { return hc.pending_jobs.size(); }) {}
};

struct LocalJobsColumn : SimpleColumn {
    LocalJobsColumn(const NCursesInterface *nc)
        : SimpleColumn(nc, "LOCAL", 5, [](const HostCache& hc) { return hc.host->total_local; }) {}
};

struct CurrentJobsColumn : SimpleColumn {
    CurrentJobsColumn(const NCursesInterface *nc)
        : SimpleColumn(nc, "CUR", 0, [](const HostCache& hc) { return hc.current_jobs.size(); }) {}
};

struct MaxJobsColumn : SimpleColumn {
    MaxJobsColumn(const NCursesInterface *nc)
        : SimpleColumn(nc, "MAX", 0, [](const HostCache& hc) { return hc.host->getMaxJobs(); }) {}
};

struct IDColumn : SimpleColumn {
    IDColumn(const NCursesInterface *nc)
        : SimpleColumn(nc, "ID", 0, [](const HostCache& hc) { return hc.host->id; }) {}
};

class SpeedColumn : public SimpleColumn {
private:
    static auto extract(const HostCache& hc) { return hc.host->getSpeed(); }
public:
    // special formatting for speed so the decimal place lines up
    SpeedColumn(const NCursesInterface *nc)
        : SimpleColumn(nc, "SPEED", 8,
                       [](std::ostream& ss, const HostCache& hc) {
                           ss << std::fixed << std::setprecision(3) << std::setw(7) << extract(hc);
                       },
                       [](const HostCache& a, const HostCache& b) {
                           return extract(a) < extract(b);
                       }) {}
};

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

void NCursesInterface::suspend()
{
    clear();
    refresh();
    endwin();
    redraw_source.clear();
}

void NCursesInterface::resume()
{
    init();
}

void NCursesInterface::print_job_graph(Job::Map const &jobs, int max_host_jobs, int max_graph_width) const
{
    struct Bin {
        int color = 0;
        bool is_local = false;
        int num_jobs = 0;
        int num_slots = 0;
        int remainder = 0;

        Bin(int color, bool is_local, int num_jobs):
            color(color), is_local(is_local), num_jobs(num_jobs)
        {}
    };

    // Only compress the jobs into a smaller or equal number of slots. Don't
    // expand them
    int max_graph_jobs = std::min(max_graph_width - 2, max_host_jobs);
    bool is_scaled = max_graph_jobs < max_host_jobs;

    std::vector<Bin> bins;
    int total_active_jobs = 0;

    for (auto const j : jobs) {
        if (!j.second->active)
            continue;

        total_active_jobs++;

        int color = 0;
        auto const h = j.second->getClient();
        if (h)
            color = h->getColor();

        bool is_local = j.second->is_local;

        bool found_bin = false;
        for (auto& b : bins) {
            if (b.color == color && b.is_local == is_local) {
                b.num_jobs++;
                found_bin = true;
            }
        }

        if (!found_bin) {
            bins.emplace_back(color, is_local, 1);
        }
    }

    // If there are nodes that do not accept remote jobs but are performing
    // local compiles, it is possible that the number of active jobs exceeds
    // the number of host jobs (at least on the master job graph). In these
    // cases, use the total number of active jobs as the maximum instead of
    // provided maximum to ensure that the job graph is still scaled in the
    // event it would exceed the allocated space.
    max_host_jobs = std::max(total_active_jobs, max_host_jobs);

    int active_graph_slots = ceil(max_graph_jobs * total_active_jobs / (double)max_host_jobs);
    int used_graph_slots = 0;

    // Calculate the whole and remainder slots for each bin
    for (auto& b : bins) {
        b.num_slots = (b.num_jobs * active_graph_slots) / total_active_jobs;
        b.remainder = (b.num_jobs * active_graph_slots) % total_active_jobs;

        used_graph_slots += b.num_slots;
    }

    // Add a slot to the bin with the highest remainders until we run out of graph slots
    std::sort(bins.begin(), bins.end(), [](Bin const& a, Bin const& b) -> bool { return a.remainder > b.remainder; });
    for (auto& b : bins) {
        if (used_graph_slots == active_graph_slots || b.remainder == 0)
            break;

        b.num_slots++;
        used_graph_slots++;
    }

    assert(used_graph_slots == active_graph_slots);

    // Sort by color/local to keep the display ordering stable. Otherwise, it
    // jumps around unpleasantly.
    std::sort(bins.begin(), bins.end(), [](Bin const& a, Bin const& b) -> bool {
        if (a.is_local != b.is_local)
            return a.is_local > b.is_local;
        return a.color < b.color;
    });

    if (is_scaled)
        addch('{');
    else
        addch('[');

    int cnt = 0;

    for (auto const& b : bins) {
        Attr clr(COLOR_PAIR(b.color));

        char c = b.is_local ? '%' : '=';

        for (int i = 0; i < b.num_slots; i++)
            addch(c);

        cnt += b.num_slots;
    }

    for (int i = cnt; i < max_graph_jobs; ++i)
        addch(' ');

    if (is_scaled)
        addch('}');
    else
        addch(']');
}

gboolean NCursesInterface::on_idle_draw(gpointer user_data)
{
    auto *self = static_cast<NCursesInterface*>(user_data);
    self->doRedraw();
    self->idle_source.clear();
    return FALSE;
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

    if (!get_anonymize()) {
        {
            Attr bold(A_BOLD);
            addstr("Scheduler: ");
        }
        addstr(scheduler->getSchedulerName().c_str());
        addch(' ');
    }

    {
        Attr bold(A_BOLD);
        addstr("Netname: ");
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
        ss << "Maximum:" << total_job_slots << " Active:" << Job::activeJobs.size() <<
            " Local:" << Job::localJobs.size() << " Pending:" << Job::pendingJobs.size();
        addstr(ss.str().c_str());
    }
    next_row();

    move(row, 6);
    print_job_graph(Job::allJobs, total_job_slots, screen_cols - 6);
    next_row();
    next_row();

    struct ColumnView {
        size_t idx;
        int col;
        int width;
        int min_width;
        int desired_width;
        std::shared_ptr<Column> column;

        bool hasSlack() const
        {
            return desired_width != min_width;
        }
    };
    std::vector<ColumnView> views;

    move(row, 0);
    {
        Attr color(COLOR_PAIR(header_color));
        Attr highlight(COLOR_PAIR(highlight_color), false);

        add_wch(sort_reversed ? WACS_UARROW : WACS_DARROW);
        for (int i = 1; i < screen_cols; i++)
            addch(' ');

        int max_col = 2;
        int min_col = 2;
        int slack_cols = 0;

        for (size_t i = 0; i < columns.size(); i++) {
            auto &c = columns[i];
            auto width = c->getWidthConstraint(host_cache);
            ColumnView v;

            v.idx = i;
            v.col = max_col;
            v.width = width.second;
            v.min_width = width.first;
            v.desired_width = width.second;
            v.column = c;

            views.push_back(v);

            max_col += width.second + 1;
            min_col += width.first + 1;

            if (v.hasSlack())
                slack_cols++;
        }

        if (max_col > screen_cols && slack_cols) {
            // Resize columns
            int slack_per_col = (screen_cols - min_col) / slack_cols;
            int extra_slack = (screen_cols - min_col) % slack_cols;

            if (slack_per_col < 0) {
                slack_per_col = 0;
                extra_slack = 0;
            }

            for (auto& v : views) {
                if (v.hasSlack()) {
                    v.width = v.min_width + slack_per_col;
                    if (extra_slack > 0) {
                        v.width++;
                        extra_slack--;
                    }
                }
            }

            // Recalculate positions
            int col = 2;
            for (auto& v : views) {
                v.col = col;
                col += v.width + 1;
            }
        }

        // Draw headers
        for (auto const& v : views) {
            if (v.col + v.width <= screen_cols) {
                if (current_col == v.idx) {
                    color.off();
                    highlight.on();
                }

                mvprintw(row, v.col, "%-*s", v.width, v.column->getHeader().c_str());

                if (current_col == v.idx) {
                    highlight.off();
                    color.on();
                }
            }
        }
    }
    next_row();

    if (current_col < columns.size()) {
        auto raw_compare = columns[current_col]->get_compare();
        auto compare = [&](const std::shared_ptr<const HostCache>& a,
                           const std::shared_ptr<const HostCache>& b) {
            if (sort_reversed) {
                return raw_compare(*a, *b);
            } else {
                return raw_compare(*b, *a);
            }
        };
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

        for (auto const &v: views) {
            if (v.col + v.width <= screen_cols)
                v.column->output(row, v.col, v.width, cache);
        }

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
                    if (job->filename.empty()) {
                        addstr("<unknown>");
                    } else if (get_anonymize()) {
                        std::ostringstream ss;
                        ss << "Job " << std::hash<std::string>{}(job->filename);
                        addstr(ss.str().c_str());
                    } else {
                        addstr(job->filename.c_str());
                    }
                }
            }

            size_t width = 0;
            for (auto const &a : host->attr) {
                width = std::max(width, a.first.size());
            }

            for (auto const &a : host->attr) {
                if (get_anonymize() && (a.first == "Name" || a.first == "IP"))
                    continue;

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
    erase();
    doRender();
    refresh();
}

void NCursesInterface::triggerRedraw()
{
    if (!idle_source.get())
        idle_source.set(g_idle_add(reinterpret_cast<GSourceFunc>(on_idle_draw), this));
}

void NCursesInterface::init()
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

    Host::clearColors();
    Host::addColor(assign_color(COLOR_RED, -1));
    Host::addColor(assign_color(COLOR_YELLOW, -1));
    Host::addColor(assign_color(COLOR_BLUE, -1));
    Host::addColor(assign_color(COLOR_MAGENTA, -1));
    Host::addColor(assign_color(COLOR_CYAN, -1));
    Host::addColor(assign_color(COLOR_WHITE, -1));

    Host::setLocalhostColor(assign_color(COLOR_GREEN, -1));

    header_color = assign_color(COLOR_BLACK, COLOR_GREEN);
    expand_color = assign_color(COLOR_GREEN, -1);
    highlight_color = assign_color(COLOR_BLACK, COLOR_CYAN);

    redraw_source.set(g_timeout_add(1000, on_redraw_timer, this));

    triggerRedraw();
}

NCursesInterface::NCursesInterface() :
    UserInterface()
{
    init();

    columns.emplace_back(std::make_unique<IDColumn>(this));
    columns.emplace_back(std::make_unique<NameColumn>(this));
    columns.emplace_back(std::make_unique<InJobsColumn>(this));
    columns.emplace_back(std::make_unique<CurrentJobsColumn>(this));
    columns.emplace_back(std::make_unique<MaxJobsColumn>(this));
    columns.emplace_back(std::make_unique<JobsColumn>(this));
    columns.emplace_back(std::make_unique<OutJobsColumn>(this));
    columns.emplace_back(std::make_unique<LocalJobsColumn>(this));
    columns.emplace_back(std::make_unique<ActiveJobsColumn>(this));
    columns.emplace_back(std::make_unique<PendingJobsColumn>(this));
    columns.emplace_back(std::make_unique<SpeedColumn>(this));
}

NCursesInterface::~NCursesInterface()
{
    endwin();
}

std::unique_ptr<UserInterface> create_ncurses_interface()
{
    return std::make_unique<NCursesInterface>();
}
