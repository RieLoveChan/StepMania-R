#ifndef STEPMANIA_VER_H
#define STEPMANIA_VER_H

extern char const * const product_version;

// XXX: These names are misnomers. This is actually the BUILD number, time and date.
// Defined in the generated verstub.cpp (configure_file'd from
// src/verstub.in.cpp -- see StepmaniaCore.cmake).
extern unsigned long const version_num;
extern char const * const version_time;
extern char const * const version_date;
extern char const * const sm_version_git_hash;

#endif
