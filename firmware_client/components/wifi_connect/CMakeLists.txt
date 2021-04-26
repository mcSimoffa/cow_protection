
set(COMPONENT_ADD_INCLUDEDIRS .)
set(COMPONENT_SRCS  "./wifi_connect.c")
set(COMPONENT_REQUIRES nvs_backup wpa_supplicant)
register_component()					