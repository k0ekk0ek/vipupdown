#include "header.h"


#include "archlinux.h"


static int loopback_up(interface_defn *ifd, execfn *exec) {
return 1;
}
static int loopback_down(interface_defn *ifd, execfn *exec) {
return 1;
}
static int static_up(interface_defn *ifd, execfn *exec) {
return 1;
}
static int static_down(interface_defn *ifd, execfn *exec) {
return 1;
}
static int manual_up(interface_defn *ifd, execfn *exec) {
return 1;
}
static int manual_down(interface_defn *ifd, execfn *exec) {
return 1;
}
static int v4tunnel_up(interface_defn *ifd, execfn *exec) {
return 1;
}
static int v4tunnel_down(interface_defn *ifd, execfn *exec) {
return 1;
}
static method methods[] = {
        {
                "v4tunnel",
                v4tunnel_up, v4tunnel_down,
        },
        {
                "manual",
                manual_up, manual_down,
        },
        {
                "static",
                static_up, static_down,
        },
        {
                "loopback",
                loopback_up, loopback_down,
        },
};

address_family addr_inet6 = {
        "inet6",
        sizeof(methods)/sizeof(struct method),
        methods
};
