#include <assert.h>
#include <limits.h>

#include <sys/ioctl.h>
#if HAVE_SYS_SYSCTL_H
#ifdef __FreeBSD__
#include <sys/types.h>
#endif
#include <sys/sysctl.h>
#endif
#include <stdio.h>
#include <stdbool.h>

#include "xf86drmMode.h"
#include "xf86drm.h"
#include <drm.h>
#include <panfrost_drm.h>

#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include "stdio.h"
#include <fcntl.h>

static __u64                                                              
panfrost_query_raw(int fd, enum drm_panfrost_param param, bool required,  
                   unsigned default_value)                                
{                                                                         
   struct drm_panfrost_get_param get_param = {                            
      0,                                                                  
   };                                                                     
   int ret;                                                      
                                                                          
   get_param.param = param;                                               
   ret = drmIoctl(fd, DRM_IOCTL_PANFROST_GET_PARAM, &get_param);          
                                                                          
   if (ret) {                                                             
      assert(!required);                                                  
      return default_value;                                               
   }
   return get_param.value;                                              
}

int main(int argc, char** argv) {
  char* fn = "/dev/dri/renderD128";
  if(argc > 1)
    fn = argv[1];
  int fd = open(fn, O_RDWR);
  assert(fd != -1);

  #define PRINT_PARAM(x) {						\
    unsigned gpu_prod_id = panfrost_query_raw(fd, x, false, -1);	\
    printf("%-48s: 0x%08x %u\n", #x, gpu_prod_id, gpu_prod_id);		\
  }
  
  PRINT_PARAM(DRM_PANFROST_PARAM_GPU_PROD_ID);
  PRINT_PARAM(DRM_PANFROST_PARAM_GPU_REVISION);
  PRINT_PARAM(DRM_PANFROST_PARAM_SHADER_PRESENT);
  PRINT_PARAM(DRM_PANFROST_PARAM_TILER_PRESENT);
  PRINT_PARAM(DRM_PANFROST_PARAM_L2_PRESENT);
  PRINT_PARAM(DRM_PANFROST_PARAM_STACK_PRESENT);
  PRINT_PARAM(DRM_PANFROST_PARAM_AS_PRESENT);
  PRINT_PARAM(DRM_PANFROST_PARAM_JS_PRESENT);
  PRINT_PARAM(DRM_PANFROST_PARAM_L2_FEATURES);
  PRINT_PARAM(DRM_PANFROST_PARAM_CORE_FEATURES);
  PRINT_PARAM(DRM_PANFROST_PARAM_TILER_FEATURES);
  PRINT_PARAM(DRM_PANFROST_PARAM_MEM_FEATURES);
  PRINT_PARAM(DRM_PANFROST_PARAM_MMU_FEATURES);
  PRINT_PARAM(DRM_PANFROST_PARAM_THREAD_FEATURES);
  PRINT_PARAM(DRM_PANFROST_PARAM_MAX_THREADS);
  PRINT_PARAM(DRM_PANFROST_PARAM_THREAD_MAX_WORKGROUP_SZ);
  PRINT_PARAM(DRM_PANFROST_PARAM_THREAD_MAX_BARRIER_SZ);
  PRINT_PARAM(DRM_PANFROST_PARAM_COHERENCY_FEATURES);
  PRINT_PARAM(DRM_PANFROST_PARAM_TEXTURE_FEATURES0);
  PRINT_PARAM(DRM_PANFROST_PARAM_TEXTURE_FEATURES1);
  PRINT_PARAM(DRM_PANFROST_PARAM_TEXTURE_FEATURES2);
  PRINT_PARAM(DRM_PANFROST_PARAM_TEXTURE_FEATURES3);
  PRINT_PARAM(DRM_PANFROST_PARAM_JS_FEATURES0);
  PRINT_PARAM(DRM_PANFROST_PARAM_JS_FEATURES1);
  PRINT_PARAM(DRM_PANFROST_PARAM_JS_FEATURES2);
  PRINT_PARAM(DRM_PANFROST_PARAM_JS_FEATURES3);
  PRINT_PARAM(DRM_PANFROST_PARAM_JS_FEATURES4);
  PRINT_PARAM(DRM_PANFROST_PARAM_JS_FEATURES5);
  PRINT_PARAM(DRM_PANFROST_PARAM_JS_FEATURES6);
  PRINT_PARAM(DRM_PANFROST_PARAM_JS_FEATURES7);
  PRINT_PARAM(DRM_PANFROST_PARAM_JS_FEATURES8);
  PRINT_PARAM(DRM_PANFROST_PARAM_JS_FEATURES9);
  PRINT_PARAM(DRM_PANFROST_PARAM_JS_FEATURES10);
  PRINT_PARAM(DRM_PANFROST_PARAM_JS_FEATURES11);
  PRINT_PARAM(DRM_PANFROST_PARAM_JS_FEATURES12);
  PRINT_PARAM(DRM_PANFROST_PARAM_JS_FEATURES13);
  PRINT_PARAM(DRM_PANFROST_PARAM_JS_FEATURES14);
  PRINT_PARAM(DRM_PANFROST_PARAM_JS_FEATURES15);
  PRINT_PARAM(DRM_PANFROST_PARAM_NR_CORE_GROUPS);
  PRINT_PARAM(DRM_PANFROST_PARAM_THREAD_TLS_ALLOC);
  PRINT_PARAM(DRM_PANFROST_PARAM_AFBC_FEATURES);
	  
  drmClose(fd);
  return 0;
}
