cmake_minimum_required(VERSION 3.16)
project(autosar_classic_bsw_rte_pedal_voter VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -ffunction-sections -fdata-sections)
endif()

add_library(autosar_pedal_ecu STATIC
    src/Mcal_Adc_Dio.c
    src/Bsw_Dem_WdgM_E2E.c
    src/Swc_PedalVoter.c
)

target_include_directories(autosar_pedal_ecu PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

enable_testing()

add_executable(test_autosar_pedal_voter tests/test_autosar_pedal_voter.c)
target_link_libraries(test_autosar_pedal_voter PRIVATE autosar_pedal_ecu)
add_test(NAME AutosarAsilDVerificationSuite COMMAND test_autosar_pedal_voter)
