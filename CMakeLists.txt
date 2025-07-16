if ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)
endif()
