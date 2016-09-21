add_definitions(
  -D__USE_GNU
  -D_GNU_SOURCE
)

set( LIB_PREFIX 64)

########### next target ###############

SET(fsalmem_LIB_SRCS
   memfs_handle.c
   memfs_methods.h
   memfs_main.c
   memfs_export.c
)

add_library(fsalmem SHARED ${fsalmem_LIB_SRCS})

target_link_libraries(fsalmem ${SYSTEM_LIBRARIES})

set_target_properties(fsalmem PROPERTIES VERSION 4.2.0 SOVERSION 4)
install(TARGETS fsalmem COMPONENT fsal DESTINATION ${FSAL_DESTINATION} )

########### install files ###############
