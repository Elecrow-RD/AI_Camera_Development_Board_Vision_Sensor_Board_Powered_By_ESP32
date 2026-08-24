void InitIot() {

  auto& ai_vox_engine = ai_vox::Engine::GetInstance();

  // Speaker
  // 1.Define the properties for the speaker entity
    std::vector<ai_vox::iot::Property> speaker_properties({
        {
            "volume",                         
            "Current volume level",          
            ai_vox::iot::ValueType::kNumber   
        },
    });

    // 2. Define the speaker's function
    std::vector<ai_vox::iot::Function> speaker_functions({
        {"SetVolume",                                     
        "Set the volume of the device",                
        {
            {
                "volume",                                 
                "Integer value between 0 and 100",       
                ai_vox::iot::ValueType::kNumber,          
                true                                      
            },

        }},
    });

    // 3. Create speaker entity object
    g_speaker_iot_entity = std::make_shared<ai_vox::iot::Entity>(
        "Speaker",                          
        "A smart speaker device",           
        std::move(speaker_properties),      
        std::move(speaker_functions)        
    );

    // 4. Initialize default state values
    g_speaker_iot_entity->UpdateState("volume", g_audio_output_device->volume());

    // 5. Register to AI Vox Engine
    ai_vox_engine.RegisterIotEntity(g_speaker_iot_entity);

  // LED
  // 1.Define the properties for the LED entity
    std::vector<ai_vox::iot::Property> rgb_properties({
        {
            "state",                          // attribute name
            "RGB light switch state (true=on, false=off)", // describe
            ai_vox::iot::ValueType::kBool     // type (on/off)
        },
    });

    // 2. Define the properties for RGB Light functions
    std::vector<ai_vox::iot::Function> rgb_functions({
        {"TurnOn",                         
        "Turn on the RGB light",             
        {
            {
                "delay",                     
                "Delay time in seconds (optional)", 
                ai_vox::iot::ValueType::kNumber, 
                false                        
            }
        }},
        {"TurnOff",                          
        "Turn off the RGB light",         
        {
            {
                "delay",                  
                "Delay time in seconds (optional)", 
                ai_vox::iot::ValueType::kNumber,
                false
            }
        }}
    });

    g_rgb_iot_entity = std::make_shared<ai_vox::iot::Entity>(
        "RGB",                              
        "RGB lighting controller",          
        std::move(rgb_properties),          
        std::move(rgb_functions)            
    );


    g_rgb_iot_entity->UpdateState("state", false);


    ai_vox_engine.RegisterIotEntity(g_rgb_iot_entity);

  // Screen
  // 1.Define the properties for the screen entity
    std::vector<ai_vox::iot::Property> screen_properties({
        {
            "power",                           // 
            "Screen power status (on/off)",    // 
            ai_vox::iot::ValueType::kBool      // 
        },
        {
            "brightness",                      //
            "Current brightness percentage",   //
            ai_vox::iot::ValueType::kNumber    // 
        }
    });


    std::vector<ai_vox::iot::Function> screen_functions({
        {"TurnOn",
        "Turn on the screen display",
        {{
            "delay",
            "Optional delay in seconds before turning on",
            ai_vox::iot::ValueType::kNumber,
            false
        }}},
        {"TurnOff",
        "Turn off the screen display",
        {{
            "delay",
            "Optional delay in seconds before turning off",
            ai_vox::iot::ValueType::kNumber,
            false
        }}},
        {"SetBrightness",
        "Adjust screen brightness level",
        {{
            "brightness",
            "Integer from 1 to 100 representing brightness percent",
            ai_vox::iot::ValueType::kNumber,
            true
        }}}
    });


    g_screen_iot_entity = std::make_shared<ai_vox::iot::Entity>(
        "Screen",
        "Smart screen that supports power on/off and adjustable brightness",
        std::move(screen_properties),
        std::move(screen_functions)
    );


    g_screen_iot_entity->UpdateState("power", true); 
    g_screen_iot_entity->UpdateState("brightness", brightness_value);


    ai_vox_engine.RegisterIotEntity(g_screen_iot_entity);
    // LED

    std::vector<ai_vox::iot::Property> led_properties({
        {
            "state",                           // 
            "light switch state (on/off)", // 
            ai_vox::iot::ValueType::kBool
        }
    });

    // 2. 
    std::vector<ai_vox::iot::Function> led_functions({
        {"TurnOn",
        "Turn the  light on",
        {{
            "delay",
            "Optional delay in seconds",
            ai_vox::iot::ValueType::kNumber,
            false
        }}},
        {"TurnOff",
        "Turn the light off",
        {{
            "delay",
            "Optional delay in seconds",
            ai_vox::iot::ValueType::kNumber,
            false
        }}}
    });

    // 3. 
    g_led_iot_entity = std::make_shared<ai_vox::iot::Entity>(
        "Led",
        "General LED control interface",
        std::move(led_properties),
        std::move(led_functions)
    );

    // 4.
    g_led_iot_entity->UpdateState("state", false);

    // 5. 
    ai_vox_engine.RegisterIotEntity(g_led_iot_entity);
}
}

String getSerialNumber() {
  uint64_t chipid = ESP.getEfuseMac();
  char serial[32];
  sprintf(serial, "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
  return String(serial);
}

String GetMacAddress() {
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);

  char mac_str[18];
  sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  return String(mac_str);
}

String GetActivationPayload() {
  if (serial_number.length() == 0) {
    return "{}";
  }

  StaticJsonDocument<256> doc;
  doc["algorithm"] = "hmac-sha256";
  doc["serial_number"] = serial_number.c_str();
  doc["challenge"] = activation_challenge.c_str();
  doc["hmac"] = "";

  String json_str;
  serializeJson(doc, json_str);
  return json_str;
}

String GenerateUUIDSimple()
{
    uint8_t mac[6];
    if (esp_wifi_get_mac(WIFI_IF_STA, mac) != ESP_OK) {
        return "00000000-0000-0000-0000-000000000000";
    }

    char uuid[37]; 

    snprintf(uuid, sizeof(uuid),
             "%02X%02X%02X%02X-0000-1000-8000-%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3],
             mac[4], mac[5],
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return String(uuid);
}

