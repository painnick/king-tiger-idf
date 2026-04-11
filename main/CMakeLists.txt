idf_component_register(SRCS "app_main.c" 
                            "motor_control.c" 
                            "servo_led_control.c" 
                            "sound_player.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES bluepad32 nvs_flash driver)
