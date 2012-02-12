
#include <stdlib.h>
#include "header.h"


extern address_family addr_inet;

extern address_family addr_inet6;

extern address_family addr_ipx;


address_family *addr_fams[] = {
	

&addr_inet, 

&addr_inet6,

&addr_ipx,

	NULL
};
