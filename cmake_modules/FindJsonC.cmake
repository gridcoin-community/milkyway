# Copyright 2010 Matthew Arsenault, Travis Desell, Dave Przybylo,
# Nathan Cole, Boleslaw Szymanski, Heidi Newberg, Carlos Varela, Malik
# Magdon-Ismail and Rensselaer Polytechnic Institute.

# This file is part of Milkway@Home.

# Milkyway@Home is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# Milkyway@Home is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with Milkyway@Home.  If not, see <http://www.gnu.org/licenses/>.
#

if(JSON_C_USE_STATIC)
  set(__old_cmake_find_lib_suffixes ${CMAKE_FIND_LIBRARY_SUFFIXES})
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

find_path(JSON_C_INCLUDE_DIR "json/json.h")
find_library(JSON_C_LIBRARY json)

if(JSON_C_USE_STATIC)
  set(__old_cmake_find_lib_suffixes ${CMAKE_FIND_LIBRARY_SUFFIXES})
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()


if(JSON_C_INCLUDE_DIR AND JSON_C_LIBRARY)
   set(JSON_C_FOUND TRUE)
endif()

if(JSON_C_FOUND)
   if(NOT Json_c_FIND_QUIETLY)
      message(STATUS "Found json-c Library: ${JSON_C_LIBRARY}")
   endif(NOT Json_c_FIND_QUIETLY)
else(JSON_C_FOUND)
   if(Json_c_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find json-c Library")
   endif(Json_c_FIND_REQUIRED)
endif()

