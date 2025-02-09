
idf_component_register(SRCS "eros_s/src/eros_endpoint.c"
                            "eros_s/src/eros_package.c"
                            "eros_s/src/eros_router.c"
                            "eros_s/src/eros_worker.c"
                            INCLUDE_DIRS "eros_c/src"
                            REQUIRES freertos)