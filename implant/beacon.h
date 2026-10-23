#ifndef S0P_BEACON_H
#define S0P_BEACON_H

int beacon_loop(const char *controller_host, int controller_port,
                int interval, int jitter);

#endif
