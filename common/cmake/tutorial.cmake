## ======================================================================== ##
## Copyright 2009-2016 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

# additional parameters (beyond the name) are treated as additional dependencies
# if ADDITIONAL_LIBRARIES is set these will be included during linking

MACRO (ADD_TUTORIAL TUTORIAL_NAME)

IF (TASKING_INTERNAL)
  ADD_DEFINITIONS(-DTASKING_INTERNAL)
ELSE()
  ADD_DEFINITIONS(-DTASKING_TBB)
ENDIF()

ADD_EXECUTABLE(${TUTORIAL_NAME} ${TUTORIAL_NAME}.cpp ${TUTORIAL_NAME}_device.cpp ${ARGN})
TARGET_LINK_LIBRARIES(${TUTORIAL_NAME} embree tutorial image transport tutorial_device ${ADDITIONAL_LIBRARIES})
SET_PROPERTY(TARGET ${TUTORIAL_NAME} PROPERTY FOLDER tutorials/single)
INSTALL(TARGETS ${TUTORIAL_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT examples)

SET(CPACK_NSIS_MENU_LINKS ${CPACK_NSIS_MENU_LINKS} "${CMAKE_INSTALL_BINDIR}/${TUTORIAL_NAME}" "${TUTORIAL_NAME}")

IF (ENABLE_ISPC_SUPPORT AND RTCORE_RAY_PACKETS)
  ADD_ISPC_EXECUTABLE(${TUTORIAL_NAME}_ispc ${TUTORIAL_NAME}.cpp ${TUTORIAL_NAME}_device.ispc)
  TARGET_LINK_LIBRARIES(${TUTORIAL_NAME}_ispc embree tutorial image transport tutorial_device_ispc)
  SET_PROPERTY(TARGET ${TUTORIAL_NAME}_ispc PROPERTY FOLDER tutorials/ispc)
  INSTALL(TARGETS ${TUTORIAL_NAME}_ispc DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT examples)
  SET(CPACK_NSIS_MENU_LINKS ${CPACK_NSIS_MENU_LINKS} "${CMAKE_INSTALL_BINDIR}/${TUTORIAL_NAME}_ispc" "${TUTORIAL_NAME}_ispc")
ENDIF()

SET(CPACK_NSIS_MENU_LINKS ${CPACK_NSIS_MENU_LINKS} PARENT_SCOPE)

ENDMACRO ()
