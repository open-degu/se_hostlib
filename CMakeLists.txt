if(CONFIG_SE_HOSTLIB)
zephyr_interface_library_named(SE_HOSTLIB)

target_include_directories(SE_HOSTLIB INTERFACE
  inc
  common
  platform
  wrapper
  )

add_definitions(
  -DZEPHYR
  -DFTR_FILE_SYSTEM
  -DTGT_A71CH
  -DI2C
  -DMBEDTLS
  -DNO_SECURE_CHANNEL_SUPPORT
  -DSCP_MODE=NO_C_MAC_NO_C_ENC_NO_R_MAC_NO_R_ENC
  )
if (${CMAKE_BUILD_TYPE} MATCHES "[Dd]ebug")
  add_definitions(-DDEBUG)
endif()

zephyr_library()
zephyr_library_sources(
  ax/a71ch_com.c
  ax/a71ch_crypto_derive.c
  ax/a71ch_crypto_ecc.c
  ax/a71ch_module.c
  ax/a71ch_sst.c
  ax/ax_scp.c
  common/a71_debug.c
  common/global_platf.c
  common/nxLog.c
  common/sci2c.c
  common/scp.c
  common/sm_apdu.c
  common/sm_errors.c
  common/sm_printf.c
  common/smCom.c
  common/smComSCI2C.c
  platform/i2c_a7.c
  platform/sm_timer.c
  wrapper/libA71CH_api.c
  )

zephyr_library_link_libraries(SE_HOSTLIB)
target_link_libraries(SE_HOSTLIB INTERFACE zephyr_interface)
endif()
