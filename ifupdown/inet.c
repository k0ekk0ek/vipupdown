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
static int dhcp_up(interface_defn *ifd, execfn *exec) {
return 1;
}
static int dhcp_down(interface_defn *ifd, execfn *exec) {
return 1;
}
static int bootp_up(interface_defn *ifd, execfn *exec) {
return 1;
}
static int bootp_down(interface_defn *ifd, execfn *exec) {
return 1;
}
static int ppp_up(interface_defn *ifd, execfn *exec) {
return 1;
}
static int ppp_down(interface_defn *ifd, execfn *exec) {
return 1;
}
static int wvdial_up(interface_defn *ifd, execfn *exec) {
return 1;
}
static int wvdial_down(interface_defn *ifd, execfn *exec) {
return 1;
}
static method methods[] = {
        {
                "wvdial",
                wvdial_up, wvdial_down,
        },
        {
                "manual",
                manual_up, manual_down,
        },
        {
                "ppp",
                ppp_up, ppp_down,
        },
        {
                "static",
                static_up, static_down,
        },
        {
                "bootp",
                bootp_up, bootp_down,
        },
        {
                "dhcp",
                dhcp_up, dhcp_down,
        },
        {
                "loopback",
                loopback_up, loopback_down,
        },
};

address_family addr_inet = {
        "inet",
        sizeof(methods)/sizeof(struct method),
        methods
};
