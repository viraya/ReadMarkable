#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "gesture.h"

#define XOVI_START_PATH "/home/root/xovi/start"
#define LOCK_PATH "/run/xovi-trigger.lock"
#define TOP_STRIP_PERCENT 15
#define TAP_MAX_DURATION_MS 500

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x) - 1) / BITS_PER_LONG) + 1)
#define TEST_BIT(bit, array) ((array[(bit) / BITS_PER_LONG] >> ((bit) % BITS_PER_LONG)) & 1)

typedef struct {
    int fd;
    int has_touch;
    int touch_active;
    int touch_in_zone;
    uint64_t touch_down_ms;
    int32_t y_max;
    int32_t y_threshold;
    gesture_t gesture;
} touch_source_t;

typedef struct {
    int fd;
    int has_power;
    int power_down;
    uint64_t power_down_ms;
    gesture_t gesture;
} power_source_t;

static uint64_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)(ts.tv_nsec / 1000000);
}

static int read_capabilities(int fd, unsigned long *keys, unsigned long *abs_bits) {
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(*keys) * NBITS(KEY_MAX)), keys) < 0) {
        return -1;
    }
    if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(*abs_bits) * NBITS(ABS_MAX)), abs_bits) < 0) {
        return -1;
    }
    return 0;
}

static int open_input_sources(touch_source_t *touch, power_source_t *power) {
    DIR *d = opendir("/dev/input");
    if (!d) {
        perror("opendir /dev/input");
        return -1;
    }
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (strncmp(de->d_name, "event", 5) != 0) continue;
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/%s", de->d_name);
        int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) continue;
        unsigned long keys[NBITS(KEY_MAX)] = {0};
        unsigned long abs_bits[NBITS(ABS_MAX)] = {0};
        if (read_capabilities(fd, keys, abs_bits) < 0) {
            close(fd);
            continue;
        }
        if (!touch->has_touch && TEST_BIT(BTN_TOUCH, keys) && TEST_BIT(ABS_MT_POSITION_Y, abs_bits)) {
            struct input_absinfo info;
            if (ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &info) == 0) {
                touch->fd = fd;
                touch->has_touch = 1;
                touch->y_max = info.maximum;
                touch->y_threshold = (info.maximum * TOP_STRIP_PERCENT) / 100;
                gesture_reset(&touch->gesture);
                continue;
            }
        }
        if (!power->has_power && TEST_BIT(KEY_POWER, keys)) {
            power->fd = fd;
            power->has_power = 1;
            gesture_reset(&power->gesture);
            continue;
        }
        close(fd);
    }
    closedir(d);
    return 0;
}

static void write_lock(void) {
    int fd = open(LOCK_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) close(fd);
}

static int lock_exists(void) {
    struct stat st;
    return stat(LOCK_PATH, &st) == 0;
}

static void activate(void) {
    write_lock();
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        int devnull = open("/dev/null", O_RDONLY);
        if (devnull >= 0) {
            dup2(devnull, 0);
            close(devnull);
        }
        execl(XOVI_START_PATH, XOVI_START_PATH, (char *)NULL);
        perror("execl " XOVI_START_PATH);
        _exit(127);
    }
    int status = 0;
    waitpid(pid, &status, 0);
}

static void handle_touch(touch_source_t *touch, const struct input_event *ev) {
    if (ev->type == EV_ABS && ev->code == ABS_MT_POSITION_Y) {
        if (!touch->touch_active) {
            touch->touch_down_ms = now_ms();
            touch->touch_active = 1;
            touch->touch_in_zone = (ev->value <= touch->y_threshold);
        }
    } else if (ev->type == EV_KEY && ev->code == BTN_TOUCH) {
        if (ev->value == 1) {
            if (!touch->touch_active) {
                touch->touch_active = 1;
                touch->touch_down_ms = now_ms();
                touch->touch_in_zone = 0;
            }
        } else {
            uint64_t t = now_ms();
            if (touch->touch_active && touch->touch_in_zone &&
                (t - touch->touch_down_ms) <= TAP_MAX_DURATION_MS) {
                if (gesture_record(&touch->gesture, t)) {
                    activate();
                    gesture_disarm(&touch->gesture);
                }
            }
            touch->touch_active = 0;
            touch->touch_in_zone = 0;
        }
    }
}

static void handle_power(power_source_t *power, const struct input_event *ev) {
    if (ev->type != EV_KEY || ev->code != KEY_POWER) return;
    if (ev->value == 1) {
        power->power_down = 1;
        power->power_down_ms = now_ms();
    } else if (ev->value == 0) {
        uint64_t t = now_ms();
        if (power->power_down && (t - power->power_down_ms) <= TAP_MAX_DURATION_MS) {
            if (gesture_record(&power->gesture, t)) {
                activate();
                gesture_disarm(&power->gesture);
            }
        }
        power->power_down = 0;
    }
}

static int drain(int fd, void (*handler)(void *, const struct input_event *), void *ctx) {
    struct input_event ev;
    ssize_t n;
    while ((n = read(fd, &ev, sizeof(ev))) == sizeof(ev)) {
        handler(ctx, &ev);
    }
    if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        return -1;
    }
    return 0;
}

static void touch_handler(void *ctx, const struct input_event *ev) {
    handle_touch((touch_source_t *)ctx, ev);
}

static void power_handler(void *ctx, const struct input_event *ev) {
    handle_power((power_source_t *)ctx, ev);
}

int main(void) {
    if (lock_exists()) {
        return 0;
    }
    touch_source_t touch = {0};
    power_source_t power = {0};
    touch.fd = -1;
    power.fd = -1;
    if (open_input_sources(&touch, &power) < 0) {
        return 1;
    }
    if (!touch.has_touch && !power.has_power) {
        fprintf(stderr, "xovi-trigger: no touch or power evdev found\n");
        return 1;
    }

    struct pollfd fds[2];
    int nfds = 0;
    if (touch.has_touch) {
        fds[nfds].fd = touch.fd;
        fds[nfds].events = POLLIN;
        nfds++;
    }
    if (power.has_power) {
        fds[nfds].fd = power.fd;
        fds[nfds].events = POLLIN;
        nfds++;
    }

    for (;;) {
        int r = poll(fds, nfds, -1);
        if (r < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            return 1;
        }
        for (int i = 0; i < nfds; i++) {
            if (!(fds[i].revents & POLLIN)) continue;
            if (touch.has_touch && fds[i].fd == touch.fd) {
                drain(touch.fd, touch_handler, &touch);
            } else if (power.has_power && fds[i].fd == power.fd) {
                drain(power.fd, power_handler, &power);
            }
        }
    }
}
