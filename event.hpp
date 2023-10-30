#pragma once

#include <queue>
#include <cstdlib>
#include <iostream>

enum EventType {
    START_TX = 0,
    RE_TX,
    TRY_COUNT_DOWN,
    DETECT,
    SUCCESS,
    COLLISION,
    ARRIVAL
};

struct Event {
    long time;
    EventType type;
    long sta_idx;
    long link_idx;
    
    friend bool operator>(const Event& a, const Event& b) {
        if (a.time != b.time) {
            return a.time > b.time;
        }
        else {
            return static_cast<long>(a.type) > static_cast<long>(b.type);
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const Event& event) {
        os << "Time: " << event.time << "; " << "Type: " << event.type << "\n";
        return os;
    }
};

class EventQueue {
    public:
    const Event& top() {
        return m_queue.top();
    }

    void pop() {
        m_queue.pop();
    }

    void push(const Event& value) {
        m_queue.push(value);
    }

    bool empty() {
        return m_queue.empty();
    }

    size_t size() {
        return m_queue.size();
    }

    private:
    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> m_queue{};
};


