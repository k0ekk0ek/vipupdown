#include "header.h"


#include "archlinux.h"


static int static_up(interface_defn *ifd, execfn *exec) {
return 1;
}
static int static_down(interface_defn *ifd, execfn *exec) {
return 1;
}
static int dynamic_up(interface_defn *ifd, execfn *exec) {
return 1;
}
static int dynamic_down(interface_defn *ifd, execfn *exec) {
return 1;
}
static method methods[] = {
        {
                "dynamic",
                dynamic_up, dynamic_down,
        },
        {
                "static",
                static_up, static_down,
        },
};

address_family addr_ipx = {
        "ipx",
        sizeof(methods)/sizeof(struct method),
        methods
};
