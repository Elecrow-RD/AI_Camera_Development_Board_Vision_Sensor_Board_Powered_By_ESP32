idf_component_register(SRCS "main.cpp" 
                       INCLUDE_DIRS ""
                       REQUIRES ai_vox
                       PRIV_REQUIRES "esp_wifi" "nvs_flash")