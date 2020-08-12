# 
# Unit testing for the Hux kernel.
# 

project(hux-unit-test C)

set_source_files_properties(calculator.c
                            allocate_module.c
                            assert_module.c
                            PROPERTIES
                                COMPILE_DEFINITIONS
                                    UNIT_TESTING=1)

# The most simple test
add_cmocka_test(simple_test
                SOURCES simple_test.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS}
                LINK_LIBRARIES ${CMOCKA_SHARED_LIBRARY})
add_cmocka_test_environment(simple_test)
