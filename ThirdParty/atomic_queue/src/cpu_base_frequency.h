/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*- */
#ifndef CPU_BASE_FREQUENCY_H_INCLUDED
#define CPU_BASE_FREQUENCY_H_INCLUDED

// Copyright (c) 2019 Maxim Egorushkin. MIT License. See the full licence in file LICENSE.

#include <vector>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace atomic_queue
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

double cpu_base_frequency();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct CpuTopologyInfo
{
	unsigned socket_id;
	unsigned core_id;
	unsigned hw_thread_id;
};
std::vector<CpuTopologyInfo> get_cpu_topology_info();

std::vector<CpuTopologyInfo> sort_by_core_id(std::vector<CpuTopologyInfo> const&);
std::vector<CpuTopologyInfo> sort_by_hw_thread_id(std::vector<CpuTopologyInfo> const&);

std::vector<unsigned> hw_thread_id(std::vector<CpuTopologyInfo> const&);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void set_thread_affinity(unsigned hw_thread_id);
void reset_thread_affinity();

void set_default_thread_affinity(unsigned hw_thread_id);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace atomic_queue

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif // QUORUM_CPU_BASE_FREQUENCY_H_INCLUDED
