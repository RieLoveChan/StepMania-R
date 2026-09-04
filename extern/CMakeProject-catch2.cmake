# Catch2 v3.16.0 — vendored amalgamated distribution.
#
# Source: extras/catch_amalgamated.{hpp,cpp} from the upstream v3.16.0
# release (github.com/catchorg/Catch2). Bumping = replace those two files
# plus LICENSE.txt and update the version here, in
# DocsAgents/adr/0006-test-harness.md, and in DocsAgents/baseline.md.
#
# Only pulled in when WITH_TESTS is ON (see extern/CMakeLists.txt). The
# unit-test target links this; nothing in the shipped engine does.

set(CATCH2_SRC "Catch2/catch_amalgamated.cpp")
set(CATCH2_HPP "Catch2/catch_amalgamated.hpp")

source_group("" FILES ${CATCH2_SRC} ${CATCH2_HPP})

add_library("Catch2" STATIC ${CATCH2_SRC} ${CATCH2_HPP})

set_property(TARGET "Catch2" PROPERTY FOLDER "External Libraries")
set_property(TARGET "Catch2" PROPERTY CXX_STANDARD 17)
set_property(TARGET "Catch2" PROPERTY CXX_STANDARD_REQUIRED ON)

disable_project_warnings("Catch2")

target_include_directories("Catch2" PUBLIC "Catch2")
