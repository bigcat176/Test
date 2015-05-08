#ifndef STRUCTURES_H
#define STRUCTURES_H

#include "includes.h"
#include "keymap.h"

// Profile ID structure
typedef struct {
    char guid[16];
    char modified[4];
} usbid;

// Used to manipulate key bitfields
// The do-while business is a hack to make statements like "if(a) SET_KEYBIT(...); else CLEAR_KEYBIT(...);" work
#define SET_KEYBIT(array, index) do { (array)[(index) / 8] |= 1 << ((index) % 8); } while(0)
#define CLEAR_KEYBIT(array, index) do { (array)[(index) / 8] &= ~(1 << ((index) % 8)); } while(0)

// Indicator LEDs
#define I_NUM       1
#define I_CAPS      2
#define I_SCROLL    4

// Maximum number of notification nodes
#define OUTFIFO_MAX 10

// Action triggered when activating a macro
typedef struct {
    short scan;         // Key scancode, OR
    short rel_x, rel_y; // Mouse movement
    char down;          // 0 for keyup, 1 for keydown (ignored if rel_x != 0 || rel_y != 0)
} macroaction;

// Key macro
typedef struct {
    macroaction* actions;
    int actioncount;
    uchar combo[N_KEYBYTES_INPUT];
    char triggered;
} keymacro;

// Key bindings for a mode (keyboard + mouse)
typedef struct {
    // Base bindings
    int base[N_KEYS_INPUT];
    // Macros
    keymacro* macros;
    int macrocount;
    int macrocap;
} binding;
#define MACRO_MAX   1024

// Keyboard/mouse input tracking
typedef struct {
    uchar keys[N_KEYBYTES_INPUT];
    uchar prevkeys[N_KEYBYTES_INPUT];
    short rel_x, rel_y;
} usbinput;

// Lighting structure for a mode
typedef struct {
    uchar r[N_KEYS_KB + N_MOUSE_ZONES_EXTENDED];
    uchar g[N_KEYS_KB + N_MOUSE_ZONES_EXTENDED];
    uchar b[N_KEYS_KB + N_MOUSE_ZONES_EXTENDED];
} lighting;

// Native mode structure
#define MD_NAME_LEN 16
typedef struct {
    lighting light;
    binding bind;
    // Name and UUID
    usbid id;
    ushort name[MD_NAME_LEN];
    // Key notification settings (bitfield - 0: off, 1: on)
    uchar notify[OUTFIFO_MAX][N_KEYBYTES_INPUT];
    // Indicators permanently off/on
    uchar ioff, ion;
    // Notify mask for indicator LEDs
    uchar inotify[OUTFIFO_MAX];
} usbmode;

// Native profile structure
#define PR_NAME_LEN 16
#define MODE_COUNT  6
typedef struct {
    // Modes
    usbmode mode[MODE_COUNT];
    // Currently-selected mode
    usbmode* currentmode;
    // Last RGB data sent to the device
    lighting lastlight;
    // Profile name and UUID
    ushort name[PR_NAME_LEN];
    usbid id;
} usbprofile;

// Hardware profile structure
#define HWMODE_K70 1
#define HWMODE_K95 3
#define HWMODE_MAX 3
typedef struct {
    // RGB settings
    lighting light[HWMODE_MAX];
    // Mode/profile IDs
    usbid id[HWMODE_MAX + 1];
    // Mode/profile names
    ushort name[HWMODE_MAX + 1][MD_NAME_LEN];
} hwprofile;

// vtables for keyboards/mice (see command.h)
extern const union devcmd vtable_keyboard;
extern const union devcmd vtable_keyboard_nonrgb;
extern const union devcmd vtable_mouse;

// Device features
#define FEAT_RGB        0x01    // RGB backlighting?
#define FEAT_POLLRATE   0x02    // Known poll rate?
#define FEAT_BIND       0x04    // Rebindable keys?
#define FEAT_NOTIFY     0x08    // Key notifications?
#define FEAT_FWVERSION  0x10    // Has firmware version?
#define FEAT_FWUPDATE   0x20    // Has firmware update?

#define FEAT_ANSI       0x40    // ANSI/ISO layout toggle (Mac only - not needed on Linux)
#define FEAT_ISO        0x80

// Standard feature sets
#define FEAT_COMMON     (FEAT_BIND | FEAT_NOTIFY | FEAT_FWVERSION)
#define FEAT_STD_RGB    (FEAT_COMMON | FEAT_RGB | FEAT_POLLRATE | FEAT_FWUPDATE)
#define FEAT_STD_NRGB   (FEAT_COMMON)
#define FEAT_LMASK      (FEAT_ANSI | FEAT_ISO)

// Feature test (usbdevice* kb, int feat)
#define HAS_FEATURES(kb, feat)      (((kb)->features & (feat)) == (feat))
#define HAS_ANY_FEATURE(kb, feat)   (!!((kb)->features & (feat)))

// Bricked firmware?
#define NEEDS_FW_UPDATE(kb) ((kb)->fwversion == 0 && HAS_FEATURES((kb), FEAT_FWUPDATE | FEAT_FWVERSION))

// Structure for tracking keyboard/mouse devices
#define KB_NAME_LEN 34
#define SERIAL_LEN  34
#define MSG_SIZE    64
typedef struct {
    // Function table (see command.h)
    const union devcmd* vtable;
    // I/O devices
#ifdef OS_LINUX
    struct udev_device* udev;
    int handle;
    int uinput;
    int event;
#else
    uchar urbinput[8 + 21 + MSG_SIZE];
    struct timespec keyrepeat;
    IOHIDDeviceRef handle;
    IOHIDDeviceRef handles[4];
    io_connect_t event;
    IOReturn lastError;
    IOOptionBits modifiers;
    short lastkeypress;
#endif
    // Thread used for USB/devnode communication. To close: lock mutexes, set handle to zero, unlock, then wait for thread to stop
    pthread_t thread;
    // Thread for device input. Will close on its own when the device is terminated.
    pthread_t inputthread;
    // Keyboard settings
    usbprofile* profile;
    // Hardware modes. Null if not read yet
    hwprofile* hw;
    // Command FIFO
    int infifo;
    // Notification FIFOs, or zero if a FIFO is closed
    int outfifo[OUTFIFO_MAX];
    // Features (see F_ macros)
    char features;
    // Whether the keyboard is being actively controlled by the driver
    char active;
    // Device name
    char name[KB_NAME_LEN];
    // Device serial number
    char serial[SERIAL_LEN];
    // USB vendor and product IDs
    short vendor, product;
    // Firmware version
    ushort fwversion;
    // Poll rate (ns), or -1 if unsupported
    int pollrate;
    // Current input state
    usbinput input;
    // Indicator LED state
    uchar ileds;
} usbdevice;

#endif  // STRUCTURES_H
