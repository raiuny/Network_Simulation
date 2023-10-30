#pragma once

#include <vector>

struct Channel {
    Channel(long num_links = 1) : tx_sta_cnts(num_links, 0) { }

    std::vector<long> tx_sta_cnts;
};