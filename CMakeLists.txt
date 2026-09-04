cmake_minimum_required(VERSION 3.20)

project(StepMania)

include(StepmaniaCore.cmake)

# The external libraries need to be included.
add_subdirectory(extern)

# The internal libraries and eventual executable to be used.
add_subdirectory(src)

# Unit tests (Catch2 + the sm_engine OBJECT library). OFF by default;
# a dedicated CI job turns WITH_TESTS ON. See
# DocsAgents/adr/0006-test-harness.md.
if(WITH_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
