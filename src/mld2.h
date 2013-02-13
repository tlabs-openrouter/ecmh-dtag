/* The timeout for queries */
/* as per RFC3810 MLDv2 "9.2.  Query Interval" */
#define ECMH_SUBSCRIPTION_TIMEOUT	125

/* Robustness Factor, per RFC3810 MLDv2 "9.1.  Robustness Variable" */
#define ECMH_ROBUSTNESS_FACTOR		2

#define QUERY_INTERVAL 10
/* Multicast Address Listening Interval, 
 * per RFC3810 MLDv2 "9.1 Multicast Address Listening Interval 
 */
#define MALI (ECMH_SUBSCRIPTION_TIMEOUT*ECMH_ROBUSTNESS_FACTOR+QUERY_INTERVAL)

/* Last Listener Query Interval
 * per RFC3810 MLDv2 
 * (Note: per RFC, this should be one second. But we're
 * using 1-second-resolution time (the epoch), so to be safe, use two seconds.
 */
#define LLQI 2

