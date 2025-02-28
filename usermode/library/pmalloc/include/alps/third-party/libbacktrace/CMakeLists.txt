#    CMakeLists.txt -- libbacktrace CMake build script
#    Contributed by Alexander Monakov, ISP RAS
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     (1) Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#
#     (2) Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in
#     the documentation and/or other materials provided with the
#     distribution.
#
#     (3) The name of the author may not be used to
#     endorse or promote products derived from this software without
#     specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.  */

cmake_minimum_required (VERSION 2.8)

project (libbacktrace)

set (BACKTRACE_SUPPORTED 1)

if(BUILD_SHARED_LIBS)
    add_definitions (-D_DYN_LINK_FLAG) 
    add_definitions (-DBOOST_LOG_DYN_LINK)
else()
    add_definitions (-static -static-libgcc)
endif()

include (CheckSymbolExists)
check_symbol_exists (_Unwind_Backtrace unwind.h HAVE_BACKTRACE)
check_symbol_exists (_Unwind_GetIPInfo unwind.h HAVE_GETIPINFO)

if (HAVE_BACKTRACE)
    set (BACKTRACE_FILE backtrace.c simple.c)
else ()
    set (BACKTRACE_FILE nounwind.c)
    set (BACKTRACE_SUPPORTED 0)
    message (STATUS "libunwind not found. Disabling Backtrace support.")
endif ()

include (CheckCCompilerFlag)
check_c_compiler_flag ("-funwind-tables" FLAG_UNWIND_TABLES)
if (FLAG_UNWIND_TABLES)
    add_definitions ("-funwind-tables")
endif ()

# Adjust warnings
if (CMAKE_COMPILER_IS_GNUCC)
    add_definitions ("-Wno-switch -Wno-enum-compare")
endif ()

check_c_source_compiles (
    "int i;
    int main() {
    __sync_bool_compare_and_swap (&i, i, i);
    __sync_lock_test_and_set (&i, 1);
    __sync_lock_release (&i);}"
    HAVE_SYNC_FUNCTIONS)

if (HAVE_SYNC_FUNCTIONS)
    set (BACKTRACE_SUPPORTS_THREADS 1)
else ()
    set (BACKTRACE_SUPPORTS_THREADS 0)
endif ()

if (CMAKE_EXECUTABLE_FORMAT STREQUAL "ELF")
    set (FORMAT_FILE elf.c dwarf.c)
    math (EXPR BACKTRACE_ELF_SIZE 8*${CMAKE_C_SIZEOF_DATA_PTR})
else ()
    set (FORMAT_FILE unknown.c)
    set (BACKTRACE_SUPPORTED 0)
    message (STATUS "Executable format is not ELF. Disabling Backtrace support.")
endif ()

check_symbol_exists (mmap sys/mman.h HAVE_MMAP)

if (HAVE_MMAP)
    set (VIEW_FILE mmapio.c)
    check_symbol_exists (MAP_ANONYMOUS sys/mman.h HAVE_MMAP_ANONYMOUS)
    check_symbol_exists (MAP_ANON sys/mman.h HAVE_MMAP_ANON)
    if (HAVE_MMAP_ANONYMOUS AND HAVE_MMAP_ANON)
	set (ALLOC_FILE mmap.c)
    else ()
	set (ALLOC_FILE alloc.c)
    endif ()
else ()
    set (VIEW_FILE read.c)
    set (ALLOC_FILE alloc.c)
endif ()

if (ALLOC_FILE STREQUAL "alloc.c")
    set (BACKTRACE_USES_MALLOC 1)
else ()
    set (BACKTRACE_USES_MALLOC 0)
endif ()

add_definitions (-D_GNU_SOURCE)

set (CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS} -D_GNU_SOURCE)
check_symbol_exists (dl_iterate_phdr link.h HAVE_DL_ITERATE_PHDR)

include (CheckFunctionExists)
check_function_exists (fcntl HAVE_FCNTL)

check_function_exists (strnlen HAVE_DECL_STRNLEN)

check_function_exists (getexecname HAVE_GETEXECNAME)

include (CheckIncludeFile)
check_include_file (dwarf.h HAVE_DWARF_H)
check_include_file (libdwarf/dwarf.h HAVE_LIBDWARF_DWARF_H)
if (NOT HAVE_DWARF_H AND NOT HAVE_LIBDWARF_DWARF_H)
    set (FORMAT_FILE unknown.c)
    set (BACKTRACE_SUPPORTED 0)
    message (STATUS "libdwarf not found. Disabling Backtrace support.")
endif ()

configure_file (backtrace-supported.h.in backtrace-supported.h)

configure_file (config.h.in.cmake config.h)

include_directories (BEFORE
    ${CMAKE_CURRENT_BINARY_DIR}
)
include_directories (
    auxincl
)
add_library (backtrace
    ${BACKTRACE_FILE}
    ${FORMAT_FILE}
    ${VIEW_FILE}
    ${ALLOC_FILE}
    atomic.c
    fileline.c
    posix.c
    print.c
    sort.c
    state.c
)

set (ENABLE_LIBBACKTRACE_TEST false CACHE BOOL "Enable libbacktrace testing")
if (ENABLE_LIBBACKTRACE_TEST)
    enable_testing ()
    add_executable (btest btest.c)
    set_property (SOURCE btest.c PROPERTY COMPILE_FLAGS "-g")
    target_link_libraries (btest backtrace)
    add_test (test-libbacktrace btest)
endif ()
