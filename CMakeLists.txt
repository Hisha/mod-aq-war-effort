# The core discovers sources and Addmod_aq_war_effortScripts automatically.
# Match current sibling modules without using the deprecated script-loader API.
AC_ADD_SCRIPT("${CMAKE_CURRENT_LIST_DIR}/src/mod_aq_war_effort.cpp")
AC_ADD_SCRIPT("${CMAKE_CURRENT_LIST_DIR}/src/AQScarabWall.cpp")
AC_ADD_SCRIPT("${CMAKE_CURRENT_LIST_DIR}/src/AQScarabGong.cpp")
AC_ADD_SCRIPT("${CMAKE_CURRENT_LIST_DIR}/src/AQWarContent.cpp")
AC_ADD_SCRIPT("${CMAKE_CURRENT_LIST_DIR}/src/AQBattlefrontData.cpp")
AC_ADD_SCRIPT("${CMAKE_CURRENT_LIST_DIR}/src/mod_aq_war_effort_loader.cpp")

message("-- MOD AQ WAR EFFORT CONFIGURED --")
