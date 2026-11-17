#ifndef UDP_SENDER_STOP_SIGNAL_H
#define UDP_SENDER_STOP_SIGNAL_H

#include <csignal>

/// Set to 1 by SIGINT/SIGTERM; the send loop polls it.
/// sig_atomic_t: written atomically; volatile: re-read on every check.
extern volatile std::sig_atomic_t g_stop;

/// Installs the SIGINT/SIGTERM handler that sets g_stop.
/// Without SA_RESTART, so mach_wait_until() returns early; a second SIGINT kills.
void install_stop_handler();

#endif // UDP_SENDER_STOP_SIGNAL_H
