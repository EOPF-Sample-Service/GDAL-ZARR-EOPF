# Add test executable
add_executable(test_eopf_driver test_eopf_driver.cpp)

# Link against GDAL
target_link_libraries(test_eopf_driver ${GDAL_LIBRARIES})

# Add test
add_test(NAME test_eopf_driver COMMAND test_eopf_driver)