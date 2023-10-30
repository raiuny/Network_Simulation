#include "simulator.hpp"
#include <thread>
#include <chrono>
#include <cstdio>

using namespace std::chrono_literals;

int main() {
    long num_mlds{20};
    long num_links{1};
    long init_w{1024};
    long cut_stg{6};
    long suc_time{180};
    long col_time{175};
    ArrivalType arr_type{BERNOULLI};

    std::vector<double> uni_arr_rates{};
    std::vector<double> arr_rates{};
    for (double r{0.02}; r < 1.01; r += 0.02) {
        uni_arr_rates.push_back(r);
        arr_rates.push_back(num_links * r / (num_mlds * suc_time));
    }
    long len = arr_rates.size();
    std::vector<Result> results(len);

    long num_thds = std::min(8l, len);
    std::vector<std::thread> thds{};
    thds.reserve(num_thds);
    for (long thd_idx{0}; thd_idx < num_thds; ++thd_idx) {
        thds.emplace_back([=, &results](){
            for (long i{thd_idx}; i < len; i += num_thds) {
                Simulator sim{num_mlds, num_links, arr_type, arr_rates[i], init_w, cut_stg, suc_time, col_time};
                sim.run(100'000'000);
                results[i] = sim.get_results();
            }
        });
    }

    for (auto& thd : thds) {
        thd.join();
    }

    for (long i{0}; i < len; ++i) {
        printf("%.2f \t %.3f \t %10.4g \t %10.4g \t %10.4g \t %10.4g\n", 
               uni_arr_rates[i], results[i].suc_prob, results[i].que_del, 
               results[i].que_del_sqr, results[i].ser_del, results[i].ser_del_sqr);
        // double scl1 = 9.0 / 1000.0;
        // double scl2 = scl1 * scl1;
        // printf("%.2f \t %.3f \t %10.4g \t %10.4g \t %10.4g \t %10.4g\n", 
        //        uni_arr_rates[i], results[i].suc_prob, results[i].que_del * scl1, 
        //        results[i].que_del_sqr * scl2, results[i].ser_del * scl1, results[i].ser_del_sqr * scl2);
    }

    
    std::cout << "The End\n";
    return 0;
}   