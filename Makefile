NOW := $(shell date +"%c" | tr ' :' '__')

run_scratch: 
	../waf --run ERGCSimulator.cc

run_scratch_with_log:
	../waf --run ERGCSimulator.cc > log.out 2>&1

run_scratch_enable_debug_log:
	NS_LOG="ERGCApplication=level_debug|*:ERGCRouting=level_debug|*" ../waf --run ERGCSimulator.cc > log_$(NOW).out 2>&1