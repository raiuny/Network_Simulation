#pragma once

#include "mld.hpp"
#include "event.hpp"
#include "channel.hpp"

#include <string>
#include <string_view>

struct Result {
    double suc_prob{};
    double que_del{};
    double que_del_sqr{};
    double ser_del{};
    double ser_del_sqr{};
};

class Simulator {
    public:
    Simulator(long num_mlds_, long num_links_, ArrivalType arr_type_, double arr_rate_, 
              long init_w_, long cut_stg_, long suc_time_, long col_time_) 
              : num_mlds{num_mlds_}, 
                num_links{num_links_},
                channel{num_links_} {
        mlds.reserve(num_mlds);
        for (long i{0}; i < num_mlds; ++i) {
            mlds.emplace_back(num_links, arr_type_, arr_rate_, init_w_, cut_stg_, suc_time_, col_time_);
        }

        link_idxs.resize(num_links);
        std::iota(link_idxs.begin(), link_idxs.end(), 0);
    }

    Simulator(long num_mlds_, long num_links_, const std::vector<ArrivalType>& arr_types_,
              const std::vector<double>& arr_rates_, const std::vector<long>& init_ws_,
              const std::vector<long>& cut_stgs_, const std::vector<long>& suc_times_, 
              const std::vector<long>& col_times_)
              : num_mlds{num_mlds_},
                num_links{num_links_},
                channel{num_links_} {
        mlds.reserve(num_mlds);
        for (long i{0}; i < num_mlds; ++i) {
            mlds.emplace_back(num_links_, arr_types_[i], arr_rates_[i], init_ws_[i], cut_stgs_[i], suc_times_[i], col_times_[i]);
        }

        link_idxs.resize(num_links);
        std::iota(link_idxs.begin(), link_idxs.end(), 0);
    }

    void run(long sim_time);

    void on_arrival(const Event& eve);
    void on_start_tx(const Event& eve);
    void on_re_tx(const Event& eve);
    void on_try_count_down(const Event& eve);
    void on_detect(const Event& eve);
    void on_success(const Event& eve);
    void on_collision(const Event& eve);

    void show_results();
    // void save_results(std::string_view file_name);
    Result get_results();
    std::vector<Result> get_per_device_results();
    

    private:
    const long num_mlds;
    const long num_links;

    std::vector<Mld> mlds;
    EventQueue eve_q{};
    Channel channel;
    std::vector<long> link_idxs;
};

void Simulator::run(long sim_time) {
    std::mt19937_64 init_gen{std::random_device{}()};

    for (long n{0}; n < num_mlds; ++n) {
        eve_q.push({static_cast<long>(std::uniform_real_distribution<double>{0.0, 1.0 / mlds[n].arr_rate}(init_gen)), 
                    ARRIVAL, n, -1});
    }

    while (!eve_q.empty() && eve_q.top().time < sim_time) {
        Event eve = eve_q.top();
        eve_q.pop();
        switch(eve.type) {
            case ARRIVAL:
                on_arrival(eve);
                break;
            case START_TX:
                on_start_tx(eve);
                break;
            case RE_TX:
                on_re_tx(eve);
                break;
            case TRY_COUNT_DOWN:
                on_try_count_down(eve);
                break;
            case DETECT:
                on_detect(eve);
                break;
            case SUCCESS:
                on_success(eve);
                break;
            case COLLISION:
                on_collision(eve);
                break;
            default:
                std::cerr << "[ERROR]: Bad Event Type\n";
        }
    }
}

// NOTE: eve.link_idx isn't used in ARRIVAL
void Simulator::on_arrival(const Event& eve) {
    Mld& mld = mlds[eve.sta_idx];

    // Push the packet to the MLD's queue
    mld.queue.push({eve.time, -1, -1, 100});

    // The next arrival event
    eve_q.push({eve.time + static_cast<long>(mld.arr_itv()), ARRIVAL, eve.sta_idx, -1});

    if (mld.queue.size() == 1) {
        std::shuffle(link_idxs.begin(), link_idxs.end(), mld.link_gen);
        for (long l : link_idxs) {
            if (mld.on_tx_pkts[l].arr_time == -1) {
                mld.queue.front().ser_time = eve.time;
                break;
            }
        }
        for (long l : link_idxs) {
            if (channel.tx_sta_cnts[l] == 0 && mld.bocs[l] == 0 && mld.on_tx_pkts[l].arr_time == -1) {
                mld.on_tx_pkts[l] = mld.queue.front();
                mld.queue.pop();
                eve_q.push({eve.time + 1, START_TX, eve.sta_idx, l});
                return;
            }  
        }

        for (long l : link_idxs) {
            if (channel.tx_sta_cnts[l] > 0 && mld.bocs[l] == 0 && mld.on_tx_pkts[l].arr_time == -1) {
                mld.reset_boc(l);
                eve_q.push({eve.time + 1, TRY_COUNT_DOWN, eve.sta_idx, l});
            }
        }
    }

}

void Simulator::on_start_tx(const Event& eve) {
    Mld& mld = mlds[eve.sta_idx];
    long link_idx = eve.link_idx;

    if (mld.on_tx_pkts[link_idx].ser_time == -1) {
        std::cout << "Error: -1" << std::endl;
    }
    
    // mld.on_tx_pkts[link_idx] = mld.queue.front();
    // mld.queue.pop();

    // mld.on_tx_pkts[link_idx].ser_time = eve.time;
    // mld.t_que_del += (eve.time - mld.on_tx_pkts[link_idx].arr_time);

    ++channel.tx_sta_cnts[eve.link_idx];

    eve_q.push({eve.time + 1, DETECT, eve.sta_idx, eve.link_idx});
}

void Simulator::on_re_tx(const Event& eve) {
    ++channel.tx_sta_cnts[eve.link_idx];

    eve_q.push({eve.time + 1, DETECT, eve.sta_idx, eve.link_idx});
}

void Simulator::on_detect(const Event& eve) {
    Mld& mld = mlds[eve.sta_idx];

    if (channel.tx_sta_cnts[eve.link_idx] == 1) {
        eve_q.push({eve.time + mld.suc_time - 2, SUCCESS, eve.sta_idx, eve.link_idx});
    } 
    else {
        eve_q.push({eve.time + mld.col_time - 2, COLLISION, eve.sta_idx, eve.link_idx});
    }
}

void Simulator::on_success(const Event& eve) {
    Mld& mld = mlds[eve.sta_idx];
    long link_idx = eve.link_idx;

    if (mld.on_tx_pkts[link_idx].ser_time == -1) {
        std::cout << "Error: -1\n";
    }

    ++mld.pkt_cnt;
    ++mld.att_cnt;
    mld.t_que_del += (mld.on_tx_pkts[link_idx].ser_time - mld.on_tx_pkts[link_idx].arr_time);
    mld.t_que_del_sqr += (mld.on_tx_pkts[link_idx].ser_time - mld.on_tx_pkts[link_idx].arr_time)
                         * (mld.on_tx_pkts[link_idx].ser_time - mld.on_tx_pkts[link_idx].arr_time);
    mld.t_ser_del += (eve.time - mld.on_tx_pkts[link_idx].ser_time + 1);
    mld.t_ser_del_sqr += (eve.time - mld.on_tx_pkts[link_idx].ser_time + 1)
                         * (eve.time - mld.on_tx_pkts[link_idx].ser_time + 1);
    mld.on_tx_pkts[link_idx] = Pkt{};

    --channel.tx_sta_cnts[link_idx];

    if (!mld.queue.empty() && mld.queue.front().ser_time == -1) {
        mld.queue.front().ser_time = eve.time;
    }
    
    mld.reset_bow(link_idx, 0);
    mld.reset_boc(link_idx);
    if (mld.bocs[link_idx] == 0 && !mld.queue.empty()) {
        mld.on_tx_pkts[link_idx] = mld.queue.front();
        mld.queue.pop();
        eve_q.push({eve.time + 1, START_TX, eve.sta_idx, eve.link_idx});
        if (!mld.queue.empty()) {
            for (long l{0}; l < num_links; ++l) {
                if (mld.on_tx_pkts[l].arr_time == -1) {
                    mld.queue.front().ser_time = eve.time;
                    break;
                }
            }
        }
    }
    else if (mld.bocs[link_idx] != 0) {
        eve_q.push({eve.time + 1, TRY_COUNT_DOWN, eve.sta_idx, eve.link_idx});
    }

}

void Simulator::on_collision(const Event& eve) {
    Mld& mld = mlds[eve.sta_idx];
    long link_idx = eve.link_idx;

    ++mld.att_cnt;
    --channel.tx_sta_cnts[link_idx];
    
    mld.reset_bow(link_idx, 1);
    mld.reset_boc(link_idx);
    if (mld.bocs[link_idx] == 0) {
        eve_q.push({eve.time + 1, RE_TX, eve.sta_idx, eve.link_idx});
    }
    else if (mld.bocs[link_idx] != 0) {
        eve_q.push({eve.time + 1, TRY_COUNT_DOWN, eve.sta_idx, eve.link_idx});
    }
}

void Simulator::on_try_count_down(const Event& eve) {
    Mld& mld = mlds[eve.sta_idx];
    long link_idx = eve.link_idx;

    // If the channel is idle
    if (channel.tx_sta_cnts[link_idx] == 0) {
        if (mld.bocs[link_idx] > 0) {
            --mld.bocs[link_idx];
        }

        if (mld.bocs[link_idx] == 0) {
            if (mld.on_tx_pkts[link_idx].ser_time != -1) {
                eve_q.push({eve.time + 1, RE_TX, eve.sta_idx, eve.link_idx});
            }
            else if (!mld.queue.empty()) {
                mld.on_tx_pkts[link_idx] = mld.queue.front();
                mld.queue.pop();
                eve_q.push({eve.time + 1, START_TX, eve.sta_idx, eve.link_idx});
                if (!mld.queue.empty()) {
                    for (long l{0}; l < num_links; ++l) {
                        if (mld.on_tx_pkts[l].arr_time == -1) {
                            mld.queue.front().ser_time = eve.time;
                            break;
                        }
                    }
                }
            }
        }
        else {
            eve_q.push({eve.time + 1, TRY_COUNT_DOWN, eve.sta_idx, eve.link_idx});
        }
    }
    else {
        eve_q.push({eve.time + 1, TRY_COUNT_DOWN, eve.sta_idx, eve.link_idx});
    }

}

void Simulator::show_results() {
    long num_pkts{0};
    double total_que_del{0.0};
    double total_ser_del{0.0};
    for (const auto& mld : mlds) {
        num_pkts += mld.pkt_cnt;
        total_que_del += mld.t_que_del;
        total_ser_del += mld.t_ser_del;
    }
    std::cout << "Number of MLDs: " << num_mlds << "    "
              << "Number of links: " << num_links << "    "
              << "Arrival rate: " << mlds[0].arr_rate << "\n";
    std::cout << "Average queuing delay: " << total_que_del / num_pkts << "    "
              << "Average service delay: " << total_ser_del / num_pkts << "\n";
    std::cout << "Total count: " << num_pkts << "\n";
}

Result Simulator::get_results() {
    long num_pkts{0};
    long num_atts{0};
    double total_que_del{0.0};
    double total_que_del_sqr{0.0};
    double total_ser_del{0.0};
    double total_ser_del_sqr{0.0};
    for (const auto& mld : mlds) {
        num_pkts += mld.pkt_cnt;
        num_atts += mld.att_cnt;
        total_que_del += mld.t_que_del;
        total_que_del_sqr += mld.t_que_del_sqr;
        total_ser_del += mld.t_ser_del;
        total_ser_del_sqr += mld.t_ser_del_sqr;
    }
    return {static_cast<double>(num_pkts) / num_atts,
            total_que_del / num_pkts, total_que_del_sqr / num_pkts, 
            total_ser_del / num_pkts, total_ser_del_sqr / num_pkts};
}

std::vector<Result> Simulator::get_per_device_results() {
    std::vector<Result> res{};
    res.reserve(num_mlds);
    for (const auto& mld : mlds) {
        res.emplace_back(static_cast<double>(mld.pkt_cnt) / mld.att_cnt, 
                         mld.t_que_del / mld.pkt_cnt, mld.t_que_del_sqr / mld.pkt_cnt, 
                         mld.t_ser_del / mld.pkt_cnt, mld.t_ser_del_sqr / mld.pkt_cnt);
    }
    return res;
}

