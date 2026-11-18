// Unit tests for parse_args(), no framework: ctest --test-dir build

#include "config.h"

#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

#define CHECK(expr)                                                         \
    do {                                                                    \
        if (!(expr)) {                                                      \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: "  \
                      << #expr << "\n";                                     \
            ++g_failures;                                                   \
        }                                                                   \
    } while (0)

// getopt may reorder argv, so it needs writable copies
Config parse(std::initializer_list<const char*> args)
{
    std::vector<std::string> storage{"udp_sender"};
    storage.insert(storage.end(), args.begin(), args.end());

    std::vector<char*> argv;
    for (auto& s : storage) {
        argv.push_back(s.data());
    }
    argv.push_back(nullptr);

    return parse_args(static_cast<int>(storage.size()), argv.data());
}

void check_throws(std::initializer_list<const char*> args, const std::string& expected, int line)
{
    try {
        parse(args);
        std::cerr << __FILE__ << ":" << line << ": expected exception containing '"
                  << expected << "', nothing thrown\n";
        ++g_failures;
    } catch (const std::invalid_argument& e) {
        if (std::string(e.what()).find(expected) == std::string::npos) {
            std::cerr << __FILE__ << ":" << line << ": wrong message: '" << e.what()
                      << "', expected to contain '" << expected << "'\n";
            ++g_failures;
        }
    }
}

#define CHECK_THROWS(expected, ...) check_throws({__VA_ARGS__}, expected, __LINE__)

void test_defaults()
{
    const Config c = parse({});
    const Config def;
    CHECK(c.dest == def.dest);
    CHECK(c.port == def.port);
    CHECK(c.interface.empty());
    CHECK(!c.no_listener);
    CHECK(c.size == def.size);
    CHECK(c.count == def.count);
    CHECK(c.interval_us == def.interval_us);
    CHECK(c.jitter_us == def.jitter_us);
    CHECK(c.warmup == def.warmup);
    CHECK(!c.realtime);
}

void test_short_options()
{
    const Config c = parse({"-d", "192.168.0.89", "-p", "6000", "-I", "en7", "-L", "-n", "500",
                            "-i", "2000", "-j", "100", "-w", "5", "-R"});
    CHECK(c.dest == "192.168.0.89");
    CHECK(c.port == 6000);
    CHECK(c.interface == "en7");
    CHECK(c.no_listener);
    CHECK(c.count == 500);
    CHECK(c.interval_us == 2000);
    CHECK(c.jitter_us == 100);
    CHECK(c.warmup == 5);
    CHECK(c.realtime);
}

void test_long_options()
{
    const Config c = parse({"--dest=10.0.0.3", "--port", "7000", "--no-listener",
                            "--count", "100", "--interval-us", "500", "--jitter-us", "50",
                            "--warmup", "0", "--realtime"});
    CHECK(c.dest == "10.0.0.3");
    CHECK(c.port == 7000);
    CHECK(c.no_listener);
    CHECK(c.count == 100);
    CHECK(c.interval_us == 500);
    CHECK(c.jitter_us == 50);
    CHECK(c.warmup == 0);
    CHECK(c.realtime);
}

void test_size()
{
    CHECK(parse({"-s", "16"}).size == 16);
    CHECK(parse({"--size", "1472"}).size == 1472);
    CHECK(parse({"--size=200"}).size == 200);
}

void test_cross_checks()
{
    // jitter must stay below half the interval, whatever the option order
    CHECK(parse({"-j", "499"}).jitter_us == 499);
    CHECK_THROWS("--jitter-us", "-j", "500");
    CHECK_THROWS("--jitter-us", "-j", "100", "-i", "200");
    CHECK(parse({"-j", "99", "-i", "200"}).jitter_us == 99);

    CHECK(parse({"-n", "10", "-w", "10"}).warmup == 10);
    CHECK_THROWS("--warmup", "-n", "10", "-w", "11");
}

void test_invalid_values()
{
    CHECK_THROWS("invalid IPv4", "-d", "10.0.0");
    CHECK_THROWS("invalid IPv4", "-d", "pi.local");
    CHECK_THROWS("--port", "-p", "0");
    CHECK_THROWS("--port", "-p", "65536");
    CHECK_THROWS("--count", "-n", "0");
    CHECK_THROWS("--interval-us", "-i", "0");
    CHECK_THROWS("invalid value '12x'", "-n", "12x");
    CHECK_THROWS("--interface", "-I", "");
    CHECK_THROWS("--size", "-s", "15");
    CHECK_THROWS("--size", "-s", "1473");
}

void test_bad_usage()
{
    CHECK_THROWS("missing value", "-d");
    CHECK_THROWS("unknown option: --foo", "--foo");
    CHECK_THROWS("unknown option: -x", "-x");
    CHECK_THROWS("unexpected argument: extra", "extra");
}

void test_repeated_calls()
{
    CHECK(parse({"-p", "6000"}).port == 6000);
    CHECK(parse({}).port == Config{}.port);
}

} // namespace

int main()
{
    test_defaults();
    test_short_options();
    test_long_options();
    test_size();
    test_cross_checks();
    test_invalid_values();
    test_bad_usage();
    test_repeated_calls();

    if (g_failures != 0) {
        std::cerr << g_failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "all config tests passed\n";
    return 0;
}
