if(NOT GTK3_FOUND)
  return()
endif()

# STATIC, not OBJECT: an OBJECT library's objects do not propagate
# transitively through target_link_libraries() when the consumer is
# itself an OBJECT library (sm_engine, under WITH_TESTS) — CMake only
# pulls OBJECT-library objects into "real" binary targets. That gap left
# LoadingWindow_Gtk::LoadingWindow_Gtk() undefined when linking sm_tests
# on Linux. A STATIC library resolves normally through any number of
# target_link_libraries() hops. See DocsAgents/adr/0006-test-harness.md.
add_library("LoadingWindowGtk"
            STATIC
            "arch/LoadingWindow/LoadingWindow_Gtk.cpp"
            "arch/LoadingWindow/LoadingWindow_Gtk.h")

set_property(TARGET "LoadingWindowGtk" PROPERTY FOLDER "Internal Libraries")
set_property(TARGET "LoadingWindowGtk" PROPERTY CXX_STANDARD 11)
set_property(TARGET "LoadingWindowGtk" PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET "LoadingWindowGtk" PROPERTY CXX_EXTENSIONS ON)

target_include_directories("LoadingWindowGtk" PRIVATE "${SM_SRC_DIR}"
                                                      "${SM_GENERATED_SRC_DIR}"
                                                      "${SM_SRC_DIR}/arch/LoadingWindow"
                                                      "${GTK3_INCLUDE_DIRS}")

target_link_libraries("LoadingWindowGtk" ${GTK3_LIBRARIES})

list(APPEND SMDATA_LINK_LIB "LoadingWindowGtk")
list(APPEND SMDATA_ARCH_LOADING_HPP
            "arch/LoadingWindow/LoadingWindow_Gtk.h")
