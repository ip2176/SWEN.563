## Overview

Using the STM32 Discovery board, design and implement an embedded, bare-metal (no operating
system) program that will display a histogram of one thousand rising edge pulse inter-arrival times.
The inter-arrival time between pulses is expected to average around 1.0 millisecond, but the histogram
should represent the range of 101 “buckets” (one bucket per microsecond) between the default
values of 950 and 1050 microseconds. However, these upper and lower limits must be user
configurable via the virtual terminal.
