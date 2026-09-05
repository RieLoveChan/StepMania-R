# sm_tests — unit / characterization tests.
#
# Links the sm_engine OBJECT library (built when WITH_TESTS is ON) plus
# vendored Catch2. sm_engine PUBLIC-propagates the engine include dirs,
# compile definitions, and third-party link libraries, so this target
# only adds what is test-specific. See
# DocsAgents/adr/0006-test-harness.md.

set(SM_TEST_SRC
    test_RageUtil.cpp
    test_RageMath.cpp
    test_TimingData.cpp
    test_NoteData.cpp
    test_NoteDataUtil.cpp)

source_group("" FILES ${SM_TEST_SRC})

add_executable(sm_tests ${SM_TEST_SRC})

set_property(TARGET sm_tests PROPERTY CXX_STANDARD 17)
set_property(TARGET sm_tests PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET sm_tests PROPERTY CXX_EXTENSIONS ON)
set_property(TARGET sm_tests PROPERTY FOLDER "Tests")

target_link_libraries(sm_tests PRIVATE sm_engine Catch2)

if(MSVC)
  target_compile_options(sm_tests PRIVATE /utf-8 /MP2)
  # The engine objects are built /SAFESEH-less and (Debug) against the
  # debug CRT; match the executable's link settings. Console subsystem
  # (the default) — sm_tests must NOT inherit the exe's /SUBSYSTEM:WINDOWS.
  target_link_directories(sm_tests PRIVATE
                          "${SM_FFMPEG_W32_DIR}/${SM_WIN32_ARCH}")
  set_target_properties(sm_tests PROPERTIES LINK_FLAGS "/SAFESEH:NO")
  set_target_properties(sm_tests
                        PROPERTIES LINK_FLAGS_DEBUG "/NODEFAULTLIB:msvcrt.lib")
endif()

if(WIN32)
  # Run from Program/ so the ffmpeg / io DLLs resolve if a static
  # initializer in the linked engine objects needs them.
  set_target_properties(sm_tests
                        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${SM_PROGRAM_DIR}"
                                   RUNTIME_OUTPUT_DIRECTORY_RELEASE "${SM_PROGRAM_DIR}"
                                   RUNTIME_OUTPUT_DIRECTORY_DEBUG "${SM_PROGRAM_DIR}"
                                   RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${SM_PROGRAM_DIR}"
                                   RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${SM_PROGRAM_DIR}")
endif()

# One ctest entry that runs the whole Catch2 binary. Per-test-case
# discovery (catch_discover_tests) needs Catch2's extras/Catch.cmake,
# which is not vendored yet; the binary's own exit code is enough for CI.
add_test(NAME sm_tests COMMAND sm_tests)
