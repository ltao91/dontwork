#pragma once

#include <cstdint>

#include "atomic_tool.h"
#include "procedure.h"
#include "random.h"
#include "tsc.h"

inline static bool chkClkSpan(const uint64_t start, const uint64_t stop, const uint64_t threshold) {
    uint64_t diff = 0;
    diff = stop - start;
    if (diff > threshold) {
        return true;
    } else {
        return false;
    }
}

bool chkEpochLoaded() {
    uint64_t nowepo = atomicLoadGE();
    // leader_workを実行しているのはthid:0だからforは1から回している？
    for (unsigned int i = 1; i < THREAD_NUM; i++) {
        if (__atomic_load_n(&(ThLocalEpoch[i]), __ATOMIC_ACQUIRE) != nowepo) return false;
    }
    return true;
}

void leaderWork(uint64_t &epoch_timer_start, uint64_t &epoch_timer_stop) {
    epoch_timer_stop = rdtscp();
    if (chkClkSpan(epoch_timer_start, epoch_timer_stop, EPOCH_TIME * CLOCKS_PER_US * 1000) && chkEpochLoaded()) {
        atomicAddGE();
        epoch_timer_start = epoch_timer_stop;
    }
}

void ecall_initDB() {
    // init Table
    for (int i = 0; i < TUPLE_NUM; i++) {
        Tuple *tmp;
        tmp = &Table[i];
        tmp->tidword_.epoch = 1;
        tmp->tidword_.latest = 1;
        tmp->tidword_.lock = 0;
        tmp->key_ = i;
        tmp->val_ = 0;
    }

    for (int i = 0; i < THREAD_NUM; i++) {
        ThLocalEpoch[i] = 0;
        CTIDW[i] = ~(uint64_t)0;
    }

    for (int i = 0; i < LOGGER_NUM; i++) {
        ThLocalDurableEpoch[i] = 0;
    }
    DurableEpoch = 0;
}

void makeProcedure(std::vector<Procedure> &pro, Xoroshiro128Plus &rnd) {
    pro.clear();
    for (int i = 0; i < MAX_OPE; i++) {
        uint64_t tmpkey, tmpope;
        tmpkey = rnd.next() % TUPLE_NUM;
        if ((rnd.next() % 100) < RRAITO) {
            pro.emplace_back(Ope::READ, tmpkey);
        } else {
            pro.emplace_back(Ope::WRITE, tmpkey);
        }
    }
}