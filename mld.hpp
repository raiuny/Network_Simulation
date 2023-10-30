#pragma once

#include <queue>
#include <random>
#include <algorithm>

// enum class TxStatus {SUCCESS, COLLISION};

enum ArrivalType {
    POISSON = 0,
    BERNOULLI,
    DETERMINISTIC,
    UNIFORM
};

struct Pkt {
    long arr_time{-1};
    long ser_time{-1};
    long dep_time{-1};
    long len{-1};
};

struct Mld {
    public:

    Mld(long num_links_ = 1, ArrivalType arr_type_ = POISSON, double arr_rate_ = 1.0, 
        long init_w_ = 16, long cut_stg_ = 6, long suc_time_ = 180, long col_time_ = 175)
        : num_links{num_links_},
          arr_type{arr_type_},
          arr_rate{arr_rate_},
          init_w{init_w_},
          cut_stg{cut_stg_},
          max_w{init_w_ * static_cast<long>(std::pow(2, cut_stg_))},
          suc_time{suc_time_},
          col_time{col_time_} {

        arr_gen.seed(rd());
        link_gen.seed(rd());

        bows.resize(num_links);
        bocs.resize(num_links);
        boc_gens.resize(num_links);
        for (long i{0}; i < num_links; ++i) {
            boc_gens[i].seed(rd());
            bows[i] = init_w;
            bocs[i] = 0;
            // reset_boc(i);
        }

        on_tx_pkts.resize(num_links);
    }

    void reset_bow(long link_idx, long flag) {
        if (flag == 0) {
            bows[link_idx] = init_w;
        }
        else {
            bows[link_idx] = std::min<long>(bows[link_idx] * 2, max_w);
        }
    }

    void reset_boc(long link_idx) {
        std::uniform_int_distribution<long> uni_dist{0, bows[link_idx] - 1};

        bocs[link_idx] = uni_dist(boc_gens[link_idx]);
    }

    double arr_itv() {
        switch (arr_type) {
            case POISSON:
            case BERNOULLI:
                return std::exponential_distribution<double>{arr_rate}(arr_gen);
            case DETERMINISTIC:
                return 1.0 / arr_rate;
            case UNIFORM:
                return std::uniform_real_distribution<double>{0.95 / arr_rate, 1.05 / arr_rate}(arr_gen);
            default:
                return 1.0;
        }
    }

    const long num_links;

    const ArrivalType arr_type;
    const double arr_rate;

    const long init_w;
    const long cut_stg;
    const long max_w;

    const long suc_time;
    const long col_time;

    std::vector<long> bows;
    std::vector<long> bocs;

    inline static std::random_device rd{};
    std::mt19937_64 arr_gen;
    std::mt19937_64 link_gen;
    std::vector<std::mt19937_64> boc_gens;

    std::queue<Pkt> queue{};

    std::vector<Pkt> on_tx_pkts;

    long pkt_cnt{0};
    long att_cnt{0};
    double t_que_del{0.0};
    double t_que_del_sqr{0.0};
    double t_ser_del{0.0};
    double t_ser_del_sqr{0.0};
};

