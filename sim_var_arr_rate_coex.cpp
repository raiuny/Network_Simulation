#include "simulator.hpp"
#include <thread>
#include <chrono>
#include <cstdio>

using namespace std::chrono_literals;

int main() {
    long num_mlds{20};
    long num_links{1};
    long init_w{16};
    long cut_stg{6};
    long suc_time{180};
    long col_time{175};
    // ArrivalType arr_type{DETERMINISTIC};

    // Simulator sim{num_mlds, num_links, arr_rate, init_w, cut_stg};
    // sim.run(1'000'000'000);
    // sim.show_results();
    std::vector<double> uni_arr_rates{};
    std::vector<double> arr_rates{};
    for (double r{0.02}; r < 1.01; r += 0.02) {
        uni_arr_rates.push_back(r);
        arr_rates.push_back(num_links * r / (num_mlds * 180.0));
    }
    long len = arr_rates.size();
    std::vector<std::vector<Result>> results(len);

    long num_thds = std::min(8l, len);
    std::vector<std::thread> thds{};
    thds.reserve(num_thds);
    for (long thd_idx{0}; thd_idx < num_thds; ++thd_idx) {
        thds.emplace_back([=, &results](){
            for (long i{thd_idx}; i < len; i += num_thds) {
                // std::random_device this_rd{};
                // double rnd_arr_rate{std::uniform_real_distribution<double>{0.95 * arr_rates[i], 1.05 * arr_rates[i]}(this_rd)};
                std::vector<ArrivalType> arr_types(num_mlds, BERNOULLI);
                arr_types[0] = DETERMINISTIC;
                Simulator sim{num_mlds, num_links, 
                              arr_types, 
                              std::vector<double>(num_mlds, arr_rates[i]), 
                              std::vector<long>(num_mlds, init_w), 
                              std::vector<long>(num_mlds, cut_stg),
                              std::vector<long>(num_mlds, suc_time),
                              std::vector<long>(num_mlds, col_time)};
                sim.run(200'000'000);
                results[i] = sim.get_per_device_results();
            }
        });
    }

    for (auto& thd : thds) {
        thd.join();
    }

    for (long i{0}; i < len; ++i) {
        printf("%.2f \t %.3f \t %10.4g \t %10.4g \t %10.4g \t %10.4g\n", 
               uni_arr_rates[i], results[i][0].suc_prob, 
               results[i][0].que_del, results[i][0].que_del_sqr, 
               results[i][0].ser_del, results[i][0].ser_del_sqr);
        // printf("  \t %10.4g \t %10.4g \t %10.4g \t %10.4g\n", 
        //        results[i][1].que_del, results[i][1].que_del_sqr, 
        //        results[i][1].ser_del, results[i][1].ser_del_sqr);
    }

    printf("\n\n");
    for (long i{0}; i < len; ++i) {
        printf("%.2f \t %.3f \t %10.4g \t %10.4g \t %10.4g \t %10.4g\n", 
               uni_arr_rates[i], results[i][1].suc_prob, 
               results[i][1].que_del, results[i][1].que_del_sqr, 
               results[i][1].ser_del, results[i][1].ser_del_sqr);
    }
    
    std::cout << "The End\n";
    return 0;
}   