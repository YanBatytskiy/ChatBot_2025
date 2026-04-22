cmake_minimum_required(VERSION 3.16)
project(Shark_ui LANGUAGES CXX)

# Определяем, собирается ли UI автономно
set(SHARK_UI_STANDALONE OFF CACHE BOOL "UI standalone build")
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(SHARK_UI_STANDALONE ON CACHE BOOL "UI standalone build" FORCE)
endif()

# Пины Homebrew и поиск зависимостей только при автономной сборке на macOS
if(SHARK_UI_STANDALONE AND APPLE)
  set(PostgreSQL_ROOT "/opt/homebrew/opt/libpq" CACHE PATH "")
  set(PostgreSQL_INCLUDE_DIR "/opt/homebrew/opt/libpq/include" CACHE PATH "")
  set(PostgreSQL_LIBRARY "/opt/homebrew/opt/libpq/lib/libpq.dylib" CACHE FILEPATH "")
endif()

# Если нас запустили напрямую — подтянуть корневой проект один раз
if(SHARK_UI_STANDALONE AND NOT TARGET dto)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_BINARY_DIR}/_root_build)
endif()

# Ищем Qt
if(SHARK_UI_STANDALONE)
  find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
else()
  if(NOT TARGET Qt6::Widgets)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)
  find_package(Qt6 REQUIRED COMPONENTS Core)
endif()
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
qt_standard_project_setup()

qt_add_executable(Shark_ui
  main.cpp

  screen_main_window.h  screen_main_window.cpp  screen_main_window.ui
  screen_login.h        screen_login.cpp        screen_login.ui
  screen_register.h     screen_register.cpp     screen_register.ui
  screen_main_work.h    screen_main_work.cpp    screen_main_work.ui
  screen_chat_list.h    screen_chat_list.cpp    screen_chat_list.ui
  screen_chatting.h     screen_chatting.cpp     screen_chatting.ui

  # models
  models/model_chat_list.h            models/model_chat_list.cpp
  models/model_chat_list_delegate.h   models/model_chat_list_delegate.cpp
  models/model_user_list.h            models/model_user_list.cpp
  models/model_user_list_delegate.h   models/model_user_list_delegate.cpp
  models/model_chat_messages.h        models/model_chat_messages.cpp
  models/model_chat_mess_delegate.h   models/model_chat_mess_delegate.cpp

  # exceptions_qt
  exceptions_qt/errorbus.h
  exceptions_qt/exception_login.h
  exceptions_qt/exception_network.h
  exceptions_qt/exception_router.h
  exceptions_qt/exception_sql.h
  exceptions_qt/exception_valid.h

  nw_connection_monitor.h  nw_connection_monitor.cpp
  resources.qrc
  screen_user_data.h screen_user_data.cpp screen_user_data.ui
  screen_user_profile.h screen_user_profile.cpp screen_user_profile.ui
  screen_new_chat_participants.h screen_new_chat_participants.cpp screen_new_chat_participants.ui
  logger.h logger.cpp
  screen_logger_form.h screen_logger_form.cpp screen_logger_form.ui
)

# Инклуды проекта
target_include_directories(Shark_ui PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..           # доступ к src/*
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/client
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/core
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/dto
  ${CMAKE_CURRENT_SOURCE_DIR}/models
  ${CMAKE_CURRENT_SOURCE_DIR}/exceptions_qt

  ${CMAKE_CURRENT_BINARY_DIR}/Shark_ui_autogen/include
)

# Линки
target_link_libraries(Shark_ui PRIVATE
  Qt6::Core
  Qt6::Widgets
  clientlib
  core
  dto
)
target_link_libraries(Shark_ui PRIVATE Qt6::Core)
