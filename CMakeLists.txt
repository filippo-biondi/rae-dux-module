

if ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  zephyr_library_sources(src/behavior_auto_cap.c src/behavior_magic_key.c src/bluetooth_log.c)

  zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)

endif()
