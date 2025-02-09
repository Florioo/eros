
idf_component_register(SRCS "eros_c/src/eros_endpoint.c"
                            "eros_c/src/eros_package.c"
                            "eros_c/src/eros_router.c"
                            "eros_c/src/eros_worker.c"
                            INCLUDE_DIRS "eros_c/src"
                            REQUIRES freertos)