# SPDX-License-Identifier: GPL-3.0-or-later
find_package(Qt6 REQUIRED COMPONENTS Test)

foreach(name scheme composition practicestats typingtracker visibilitypolicy)
    add_executable(test_${name} test_${name}.cpp)
    target_link_libraries(test_${name} PRIVATE shuangpin-core Qt6::Test)
    add_test(NAME ${name} COMMAND test_${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endforeach()
