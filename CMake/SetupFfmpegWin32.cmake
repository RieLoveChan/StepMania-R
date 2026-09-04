# Downloads the prebuilt Windows FFmpeg DLLs + MSVC import libs into
# extern/ffmpeg-w32-prebuilt/. These are built by
# .github/workflows/build-ffmpeg-win32.yml (mingw-w64 cross-compile on
# ubuntu-latest) from the pinned extern/ffmpeg submodule commit, replacing
# the extern/ffmpeg-w32/ blob that used to be committed here (backlog
# item 7 / ADR 0001 §8) with something built from a version we control.
#
# A separate directory (not extern/ffmpeg-w32/) is used deliberately, so a
# downloaded copy never collides with — or gets silently overwritten
# alongside — a git-tracked one during the transition; it also means a
# `git status` here is never dirtied by a download.
#
# Kept offline on repeat configures: the SHA-256 of the pinned zip is
# recorded next to the extracted files, and re-download/re-extract is
# skipped once it matches.

set(SM_FFMPEG_W32_URL
    "https://github.com/RieLoveChan/StepMania-R/releases/download/ffmpeg-w32-19feb712f5/ffmpeg-w32-package.zip"
    CACHE STRING "URL of the prebuilt Windows FFmpeg package")
set(SM_FFMPEG_W32_SHA256
    "7d7d9077bf1bf3d7c6f385716bf6cc57c130b3c484a7774ac7ff68776a866afe"
    CACHE STRING "Expected SHA-256 of SM_FFMPEG_W32_URL")
mark_as_advanced(SM_FFMPEG_W32_URL SM_FFMPEG_W32_SHA256)

set(SM_FFMPEG_W32_DIR "${SM_EXTERN_DIR}/ffmpeg-w32-prebuilt")
set(SM_FFMPEG_W32_STAMP "${SM_FFMPEG_W32_DIR}/.sha256")

set(_sm_ffmpeg_w32_need_fetch TRUE)
if(EXISTS "${SM_FFMPEG_W32_STAMP}")
  file(READ "${SM_FFMPEG_W32_STAMP}" _sm_ffmpeg_w32_have_hash)
  string(STRIP "${_sm_ffmpeg_w32_have_hash}" _sm_ffmpeg_w32_have_hash)
  if(_sm_ffmpeg_w32_have_hash STREQUAL SM_FFMPEG_W32_SHA256)
    set(_sm_ffmpeg_w32_need_fetch FALSE)
  endif()
endif()

if(_sm_ffmpeg_w32_need_fetch)
  message(STATUS "Fetching prebuilt Windows FFmpeg (extern/ffmpeg @ 19feb712f5)...")

  set(_sm_ffmpeg_w32_zip "${CMAKE_BINARY_DIR}/ffmpeg-w32-package.zip")
  file(DOWNLOAD "${SM_FFMPEG_W32_URL}" "${_sm_ffmpeg_w32_zip}"
       EXPECTED_HASH "SHA256=${SM_FFMPEG_W32_SHA256}"
       STATUS _sm_ffmpeg_w32_dl_status
       SHOW_PROGRESS)
  list(GET _sm_ffmpeg_w32_dl_status 0 _sm_ffmpeg_w32_dl_code)
  if(NOT _sm_ffmpeg_w32_dl_code EQUAL 0)
    list(GET _sm_ffmpeg_w32_dl_status 1 _sm_ffmpeg_w32_dl_msg)
    message(FATAL_ERROR "Failed to download prebuilt Windows FFmpeg from "
                        "${SM_FFMPEG_W32_URL}: ${_sm_ffmpeg_w32_dl_msg}")
  endif()

  set(_sm_ffmpeg_w32_extract "${CMAKE_BINARY_DIR}/ffmpeg-w32-extract")
  file(REMOVE_RECURSE "${_sm_ffmpeg_w32_extract}")
  file(MAKE_DIRECTORY "${_sm_ffmpeg_w32_extract}")
  file(ARCHIVE_EXTRACT INPUT "${_sm_ffmpeg_w32_zip}" DESTINATION "${_sm_ffmpeg_w32_extract}")

  file(REMOVE_RECURSE "${SM_FFMPEG_W32_DIR}")
  file(MAKE_DIRECTORY "${SM_FFMPEG_W32_DIR}")
  file(COPY "${_sm_ffmpeg_w32_extract}/ffmpeg-w32-package/"
       DESTINATION "${SM_FFMPEG_W32_DIR}")
  file(REMOVE_RECURSE "${_sm_ffmpeg_w32_extract}")
  file(REMOVE "${_sm_ffmpeg_w32_zip}")

  file(WRITE "${SM_FFMPEG_W32_STAMP}" "${SM_FFMPEG_W32_SHA256}")
endif()

unset(_sm_ffmpeg_w32_need_fetch)
