/* //device/system/rild/rild.c
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <telephony/ril.h>
#define LOG_TAG "RILD"
#include <utils/Log.h>
#include <cutils/properties.h>
#include <cutils/sockets.h>
#include <linux/capability.h>
#include <linux/prctl.h>

#include <private/android_filesystem_config.h>
#include "hardware/qemu_pipe.h"

#define LIB_PATH_PROPERTY   "rild.libpath"
#define LIB_ARGS_PROPERTY   "rild.libargs"
#define MAX_LIB_ARGS        16
#define NUM_CLIENTS 2

static void usage(const char *argv0)
{
    fprintf(stderr, "Usage: %s -l <ril impl library> [-- <args for impl library>]\n", argv0);
    exit(-1);
}

extern void RIL_register (const RIL_RadioFunctions *callbacks, int client_id);

extern void RIL_onRequestComplete(RIL_Token t, RIL_Errno e,
                           void *response, size_t responselen);
//In case of DSDS two unsol functions are needed, corresponding to each of the commands interface.
extern void RIL_onUnsolicitedResponse(int unsolResponse, const void *data,
                                size_t datalen);

extern void RIL_requestTimedCallback (RIL_TimedCallback callback,
                               void *param, const struct timeval *relativeTime);

extern void RIL_setMaxNumClients(int num_clients);

static int isMultiSimEnabled();
static int isMultiRild();

static struct RIL_Env s_rilEnv = {
    RIL_onRequestComplete,
    RIL_onUnsolicitedResponse,
    RIL_requestTimedCallback
};

static struct RIL_Env s_rilEnv2 = {
    RIL_onRequestComplete,
    RIL_onUnsolicitedResponse2,
    RIL_requestTimedCallback
};

extern void RIL_startEventLoop();

static int make_argv(char * args, char ** argv)
{
    // Note: reserve argv[0]
    int count = 1;
    char * tok;
    char * s = args;

    while ((tok = strtok(s, " \0"))) {
        argv[count] = tok;
        s = NULL;
        count++;
    }
    return count;
}

/*
 * switchUser - Switches UID to radio, preserving CAP_NET_ADMIN capabilities.
 * Our group, cache, was set by init.
 */
void switchUser() {
    prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);
    setuid(AID_RADIO);

    struct __user_cap_header_struct header;
    struct __user_cap_data_struct cap;
    header.version = _LINUX_CAPABILITY_VERSION;
    header.pid = 0;
    cap.effective = cap.permitted = (1 << CAP_NET_ADMIN) | (1 << CAP_NET_RAW);
    cap.inheritable = 0;
    capset(&header, &cap);
}

int main(int argc, char **argv)
{
    const char * rilLibPath = NULL;
    char **rilArgv;
    static char * s_argv[MAX_LIB_ARGS];
    void *dlHandle;
    const RIL_RadioFunctions *(*rilInit)(const struct RIL_Env *, int, char **);
    const RIL_RadioFunctions *funcs_inst[NUM_CLIENTS] = {NULL, NULL};
    char libPath[PROPERTY_VALUE_MAX];
    unsigned char hasLibArgs = 0;
    int j = 0;
    int i;
    static char client[3] = {'0'};
    int numClients = 1;

    ALOGE("**RIL Daemon Started**");
    ALOGE("**RILd param count=%d**", argc);
    memset(s_argv, 0, MAX_LIB_ARGS*sizeof(char));

    s_argv[0] = argv[0];

    umask(S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
    for (i = 1, j = 1; i < argc ;) {
        if (0 == strcmp(argv[i], "-l") && (argc - i > 1)) {
            rilLibPath = argv[i + 1];
            i += 2;
        } else if (0 == strcmp(argv[i], "-c") && (argc - i > 1)) {
            strncpy(client, argv[i+1], strlen(client));
            i += 2;
        } else if (0 == strcmp(argv[i], "--")) {
            i++;
            hasLibArgs = 1;
            memcpy(&s_argv[j], &argv[i], argc-i);
            break;
        } else {
            usage(argv[0]);
        }
    }

    if (strcmp(client, "0") == 0) {
        RIL_setRilSocketName("rild");
    } else if (strcmp(client, "1") == 0) {
        RIL_setRilSocketName("rild1");
    }

    if (rilLibPath == NULL) {
        if ( 0 == property_get(LIB_PATH_PROPERTY, libPath, NULL)) {
            // No lib sepcified on the command line, and nothing set in props.
            // Assume "no-ril" case.
            goto done;
        } else {
            rilLibPath = libPath;
        }
    }

    /* special override when in the emulator */
#if 1
    {
        static char   arg_device[32];
        int           done = 0;

#define  REFERENCE_RIL_PATH  "/system/lib/libreference-ril.so"

        /* first, read /proc/cmdline into memory */
        char          buffer[1024], *p, *q;
        int           len;
        int           fd = open("/proc/cmdline",O_RDONLY);

        if (fd < 0) {
            ALOGD("could not open /proc/cmdline:%s", strerror(errno));
            goto OpenLib;
        }

        do {
            len = read(fd,buffer,sizeof(buffer)); }
        while (len == -1 && errno == EINTR);

        if (len < 0) {
            ALOGD("could not read /proc/cmdline:%s", strerror(errno));
            close(fd);
            goto OpenLib;
        }
        close(fd);

        if (strstr(buffer, "android.qemud=") != NULL)
        {
            /* the qemud daemon is launched after rild, so
            * give it some time to create its GSM socket
            */
            int  tries = 5;
#define  QEMUD_SOCKET_NAME    "qemud"

            while (1) {
                int  fd;

                sleep(1);

                fd = qemu_pipe_open("qemud:gsm");
                if (fd < 0) {
                    fd = socket_local_client(
                                QEMUD_SOCKET_NAME,
                                ANDROID_SOCKET_NAMESPACE_RESERVED,
                                SOCK_STREAM );
                }
                if (fd >= 0) {
                    close(fd);
                    snprintf( arg_device, sizeof(arg_device), "%s/%s",
                                ANDROID_SOCKET_DIR, QEMUD_SOCKET_NAME );

                    memset(s_argv, 0, sizeof(s_argv));
                    s_argv[1] = "-s";
                    s_argv[2] = arg_device;
                    done = 1;
                    break;
                }
                ALOGD("could not connect to %s socket: %s",
                    QEMUD_SOCKET_NAME, strerror(errno));
                if (--tries == 0)
                    break;
            }
            if (!done) {
                ALOGE("could not connect to %s socket (giving up): %s",
                    QEMUD_SOCKET_NAME, strerror(errno));
                while(1)
                    sleep(0x00ffffff);
            }
        }

        /* otherwise, try to see if we passed a device name from the kernel */
        if (!done) do {
#define  KERNEL_OPTION  "android.ril="
#define  DEV_PREFIX     "/dev/"

            p = strstr( buffer, KERNEL_OPTION );
            if (p == NULL)
                break;

            p += sizeof(KERNEL_OPTION)-1;
            q  = strpbrk( p, " \t\n\r" );
            if (q != NULL)
                *q = 0;

            snprintf( arg_device, sizeof(arg_device), DEV_PREFIX "%s", p );
            arg_device[sizeof(arg_device)-1] = 0;
            memset(s_argv, 0, sizeof(s_argv));
            s_argv[1] = "-d";
            s_argv[2] = arg_device;
            done = 1;

        } while (0);

        if (done) {
            argc = 3;
            i    = 1;
            hasLibArgs = 1;
            rilLibPath = REFERENCE_RIL_PATH;
            ALOGD("overriding with %s %s", s_argv[1], s_argv[2]);
        }
    }
OpenLib:
#endif
    switchUser();

    dlHandle = dlopen(rilLibPath, RTLD_NOW);

    if (dlHandle == NULL) {
        ALOGE("**dl open failed **");
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        exit(-1);
    }

    RIL_startEventLoop();

    rilInit = (const RIL_RadioFunctions *(*)(const struct RIL_Env *, int, char **))dlsym(dlHandle, "RIL_Init");

    if (rilInit == NULL) {
        fprintf(stderr, "RIL_Init not defined or exported in %s\n", rilLibPath);
        exit(-1);
    }

    if (hasLibArgs) {
        argc = argc-i+1;
    } else {
        static char * newArgv[MAX_LIB_ARGS];
        static char args[PROPERTY_VALUE_MAX];
        property_get(LIB_ARGS_PROPERTY, args, "");
        argc = make_argv(args, s_argv);
    }

    // Make sure there's a reasonable argv[0]
    s_argv[0] = argv[0];

    s_argv[argc++] = "-c";
    s_argv[argc++] = client;

    ALOGE("RIL_Init argc = %d client = %s",argc, s_argv[argc-1]);

    funcs_inst[0] = rilInit(&s_rilEnv, argc, s_argv);

    if (isMultiSimEnabled() && !isMultiRild()) {
        s_argv[argc-1] = "1";  //client id incase of single rild managing two instances of RIL
        ALOGE("RIL_Init argc = %d client = %s",argc, s_argv[argc-1]);
        funcs_inst[1] = rilInit(&s_rilEnv2, argc, s_argv);
        numClients++;
    }

    RIL_setMaxNumClients(numClients);

    ALOGD("Register the callbacks func received from RIL Init");
    for (i = 0; i < numClients; i++) {
        RIL_register(funcs_inst[i], i);
    }

done:

    while(1) {
        // sleep(UINT32_MAX) seems to return immediately on bionic
        sleep(0x00ffffff);
    }
}

static int isMultiSimEnabled()
{
    int enabled = 0;
    char prop_val[PROPERTY_VALUE_MAX];
    if (property_get("persist.multisim.config", prop_val, "0") > 0)
    {
        if ((strncmp(prop_val, "dsds", 4) == 0) || (strncmp(prop_val, "dsda", 4) == 0)) {
            enabled = 1;
        }
        ALOGE("isMultiSimEnabled: prop_val = %s enabled = %d", prop_val, enabled);
    }
    return enabled;
}

static int isMultiRild()
{
    int enabled = 0;
    char prop_val[PROPERTY_VALUE_MAX];
    if (property_get("ro.multi.rild", prop_val, "0") > 0)
    {
        if (strncmp(prop_val, "true", 4) == 0) {
            enabled = 1;
        }
        ALOGD("isMultiRild: prop_val = %s enabled = %d", prop_val, enabled);
    }
    return enabled;
}
