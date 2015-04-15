#include <linux/version.h>

#ifdef RHEL_RELEASE_CODE

  #if RHEL_RELEASE_CODE == RHEL_RELEASE_VERSION(6,0)
    #include "hcd.h.rhel60"
  #else
    #include "hcd.h.vanilla"
  #endif

#else
  #include "hcd.h.vanilla"
#endif
