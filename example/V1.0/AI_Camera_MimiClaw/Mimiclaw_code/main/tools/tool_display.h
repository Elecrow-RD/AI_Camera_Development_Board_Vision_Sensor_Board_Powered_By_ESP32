#ifndef __TOOL_SCREEN_H__
#define __TOOL_SCREEN_H__

/** @brief Initialize the screen 
 * Call this before using any display-related tools. It sets up the LVGL library and prepares the canvas for drawing.
 * This is automatically called when the "control_led" tool is registered, so you don't need
 * to call this manually unless you want to reinitialize the screen after clearing it.
 * Note: Always call "clear_screen" at the start of a new drawing session to free
 * memory and prevent overlapping with old visuals.
*/
void tool_screen_init();
/**
 * @brief Agent Tool: Display text at specific coordinates
 * @param input_json JSON string, e.g., {"x": 100, "y": 200, "content": "Hello"}
 * @param output Result message buffer for the Agent
 * @param output_size Size of the output buffer
 * @return ESP_OK on success
 */
esp_err_t tool_display_text_execute(const char *input_json, char *output, size_t output_size);

// /**
//  * @brief Agent Tool: Draw a pixel-like point at specific coordinates
//  * @param input_json JSON string, e.g., {"x": 400, "y": 240}
//  * @param output Result message buffer for the Agent
//  * @param output_size Size of the output buffer
//  * @return ESP_OK on success
//  */
// esp_err_t tool_draw_pixel_execute(const char *input_json, char *output, size_t output_size);

/**
 * @brief Agent Tool: Draw multiple pixel-like points on the canvas
 * @param input_json JSON string, e.g., {"points": [{"x": 100, "y": 100}, {"x": 110, "y": 110}], "color": "blue"}
 * @param output Result message buffer for the Agent
 * @param output_size Size of the output buffer
 * @return ESP_OK on success
 */
esp_err_t tool_draw_points_execute(const char *input_json, char *output, size_t output_size);

/**
 * @brief Agent Tool: Draws multiple shapes (lines or polygons) with filling on a Canvas.
 * @param input_json JSON string containing "shapes" array.
 * @param output Buffer for status message.
 * @param output_size Size of the output buffer.
 * @return ESP_OK on success, error code otherwise.
 */
esp_err_t tool_draw_shapes_execute(const char *input_json, char *output, size_t output_size);

/**
 * @brief Agent Tool: Clears all objects and resets the 800x480 LCD screen
 * @param input_json JSON Data
 * @param output  Result message buffer for the Agent
 * @param output_size Size of the output buffer
 * @return ESP_OK on success
 */
esp_err_t tool_clear_screen_execute(const char *input_json, char *output, size_t output_size);


#endif