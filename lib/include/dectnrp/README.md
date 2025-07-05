# Latency Tuning

## Mutex or Spinlock

Mutexes are typically held for a very short period of time, so switching to a spinlock has little impact on CPU usage but may improve latency.

- [queue.hpp](/lib/include/dectnrp/application/queue/queue.hpp)
- [json_export.hpp](/lib/include/dectnrp/common/json/json_export.hpp)
- [irregular_queue.hpp](/lib/include/dectnrp/phy/pool/irregular_queue.hpp)
- [job_queue_mc.hpp](/lib/include/dectnrp/phy/pool/job_queue_mc.hpp)

## Condition Variable or Busy Waiting

Switching from a condition variable to busy waiting will lead to one or multiple CPU cores spinning at 100% usage.

- [application_client.hpp](/lib/include/dectnrp/application/application_client.hpp)
- [baton.hpp](/lib/include/dectnrp/phy/pool/baton.hpp)
- [job_queue_naive.hpp](/lib/include/dectnrp/phy/pool/job_queue_naive.hpp)
- [token.hpp](/lib/include/dectnrp/phy/pool/token.hpp)
- [buffer_rx.hpp](/lib/include/dectnrp/radio/buffer_rx.hpp)
- [buffer_tx.hpp](/lib/include/dectnrp/radio/buffer_tx.hpp)

### Special Case

- [watch.hpp](/lib/include/dectnrp/common/thread/watch.hpp)
