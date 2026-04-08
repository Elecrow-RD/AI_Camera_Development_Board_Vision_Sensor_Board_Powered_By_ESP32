#include "drivers/ST7789.h"
#include "drivers/aw9523.h"
#include "cJSON.h"
#include "esp_log.h"
#include "lvgl.h"



static lv_obj_t *global_canvas = NULL;
static lv_color_t *canvas_buf = NULL;
/**
 * @brief Helper: Convert color string to lv_color_t.
 * @param color_str Color name (e.g., "red", "pink").
 * @return lv_color_t Corresponding LVGL color object.
 */
static lv_color_t get_lv_color_from_str(const char *color_str)
{
    if (!color_str)
        return lv_color_white();
    if (strcmp(color_str, "red") == 0)
        return lv_palette_main(LV_PALETTE_RED);
    if (strcmp(color_str, "green") == 0)
        return lv_palette_main(LV_PALETTE_GREEN);
    if (strcmp(color_str, "blue") == 0)
        return lv_palette_main(LV_PALETTE_BLUE);
    if (strcmp(color_str, "yellow") == 0)
        return lv_palette_main(LV_PALETTE_YELLOW);
    if (strcmp(color_str, "pink") == 0)
        return lv_color_make(0xFF, 0xC0, 0xCB);
    if (strcmp(color_str, "black") == 0)
        return lv_color_black();
    return lv_color_white();
}

static inline int16_t clamp_i16(int16_t val, int16_t min, int16_t max)
{
    if (val < min)
        return min;
    if (val > max)
        return max;
    return val;
}

void tool_screen_init()
{
    screen_init();
    lcd_backlight_ctrl(true);
}



/**
 * @brief Agent Tool: Display text at specific coordinates
 * @param input_json JSON string, e.g., {"x": 100, "y": 200, "content": "Hello"}
 * @param output Result message buffer for the Agent
 * @param output_size Size of the output buffer
 * @return ESP_OK on success
 */
esp_err_t tool_display_text_execute(const char *input_json, char *output, size_t output_size)
{
    cJSON *root = cJSON_Parse(input_json);
    if (!root)
    {
        snprintf(output, output_size, "Error: Invalid JSON format");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *x_obj = cJSON_GetObjectItem(root, "x");
    cJSON *y_obj = cJSON_GetObjectItem(root, "y");
    cJSON *content_obj = cJSON_GetObjectItem(root, "content");

    if (!cJSON_IsNumber(x_obj) || !cJSON_IsNumber(y_obj) || !cJSON_IsString(content_obj))
    {
        snprintf(output, output_size, "Error: Missing x, y, or content");
        cJSON_Delete(root);
        return ESP_ERR_INVALID_ARG;
    }

    int x = clamp_i16(x_obj->valueint, 0, 239);
    int y = clamp_i16(y_obj->valueint, 0, 283);
    const char *content = content_obj->valuestring;

    if (lvgl_port_lock(0))
    {
        lv_obj_t *label = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_20, LV_PART_MAIN);

        lv_obj_set_width(label, 240 - x);
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);

        lv_label_set_text(label, content);
        lv_obj_set_pos(label, x, y);
        lvgl_port_unlock();
        snprintf(output, output_size, "Success: Displayed text at (%d, %d)", x, y);
    }
    else
    {
        snprintf(output, output_size, "Error: LVGL system busy");
    }

    cJSON_Delete(root);
    return ESP_OK;
}

/**
 * @brief Agent Tool: Draw multiple pixel-like points on the canvas
 * @param input_json JSON string, e.g., {"points": [{"x": 100, "y": 100}, {"x": 110, "y": 110}], "color": "blue"}
 * @param output Result message buffer for the Agent
 * @param output_size Size of the output buffer
 * @return ESP_OK on success
 */
esp_err_t tool_draw_points_execute(const char *input_json, char *output, size_t output_size)
{
    cJSON *root = cJSON_Parse(input_json);
    if (!root)
    {
        snprintf(output, output_size, "Error: Invalid JSON format");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *pts_arr = cJSON_GetObjectItem(root, "points");
    if (!cJSON_IsArray(pts_arr))
    {
        snprintf(output, output_size, "Error: 'points' array required.");
        cJSON_Delete(root);
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *color_obj = cJSON_GetObjectItem(root, "color");
    const char *color_str = cJSON_IsString(color_obj) ? color_obj->valuestring : "white";
    lv_color_t pt_color = get_lv_color_from_str(color_str);

    if (lvgl_port_lock(0))
    {
        if (global_canvas == NULL)
        {
            size_t buf_size = 240 * 284 * sizeof(lv_color_t);
            canvas_buf = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
            if (!canvas_buf)
            {
                lvgl_port_unlock();
                cJSON_Delete(root);
                snprintf(output, output_size, "Error: PSRAM alloc failed for points canvas.");
                return ESP_ERR_NO_MEM;
            }
            global_canvas = lv_canvas_create(lv_scr_act());
            lv_canvas_set_buffer(global_canvas, canvas_buf, 240, 284, LV_IMG_CF_TRUE_COLOR);
            lv_canvas_fill_bg(global_canvas, lv_color_black(), LV_OPA_TRANSP);
            lv_obj_move_background(global_canvas);
        }

        lv_draw_rect_dsc_t pt_dsc;
        lv_draw_rect_dsc_init(&pt_dsc);
        pt_dsc.bg_color = pt_color;
        pt_dsc.bg_opa = LV_OPA_COVER;
        pt_dsc.border_width = 0;

        int pt_cnt = cJSON_GetArraySize(pts_arr);
        for (int i = 0; i < pt_cnt; i++)
        {
            cJSON *p = cJSON_GetArrayItem(pts_arr, i);
            cJSON *x_obj = cJSON_GetObjectItem(p, "x");
            cJSON *y_obj = cJSON_GetObjectItem(p, "y");

            if (cJSON_IsNumber(x_obj) && cJSON_IsNumber(y_obj))
            {
                int x = clamp_i16(x_obj->valueint, 0, 238);
                int y = clamp_i16(y_obj->valueint, 0, 282);

                lv_canvas_draw_rect(global_canvas, x, y, 2, 2, &pt_dsc);
            }
        }
        lvgl_port_unlock();
        snprintf(output, output_size, "Success: %d points drawn in %s", pt_cnt, color_str);
    }
    else
    {
        snprintf(output, output_size, "Error: LVGL system busy.");
    }

    cJSON_Delete(root);
    return ESP_OK;
}

/**
 * @brief Agent Tool: Draws multiple shapes (lines or polygons) with filling on a Canvas.
 * @param input_json JSON string containing "shapes" array.
 * @param output Buffer for status message.
 * @param output_size Size of the output buffer.
 * @return ESP_OK on success, error code otherwise.
 */
esp_err_t tool_draw_shapes_execute(const char *input_json, char *output, size_t output_size)
{
    cJSON *root = cJSON_Parse(input_json);
    if (!root)
    {
        snprintf(output, output_size, "Error: Invalid JSON format");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *shapes_arr = cJSON_GetObjectItem(root, "shapes");
    if (!cJSON_IsArray(shapes_arr))
    {
        snprintf(output, output_size, "Error: 'shapes' array required.");
        cJSON_Delete(root);
        return ESP_ERR_INVALID_ARG;
    }

    if (lvgl_port_lock(0))
    {
        if (global_canvas == NULL)
        {
            canvas_buf = (lv_color_t *)heap_caps_malloc(240 * 284 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
            if (!canvas_buf)
            {
                snprintf(output, output_size, " Error: PSRAM alloc failed. Check if PSRAM is enabled.");
                lvgl_port_unlock();
                cJSON_Delete(root);
                return ESP_ERR_NO_MEM;
            }
            global_canvas = lv_canvas_create(lv_scr_act());
            lv_canvas_set_buffer(global_canvas, canvas_buf, 240, 284, LV_IMG_CF_TRUE_COLOR);
            lv_canvas_fill_bg(global_canvas, lv_color_black(), LV_OPA_TRANSP);
            lv_obj_move_background(global_canvas);
        }

        cJSON *shape_item;
        cJSON_ArrayForEach(shape_item, shapes_arr)
        {
            cJSON *pts_arr = cJSON_GetObjectItem(shape_item, "points");
            if (!cJSON_IsArray(pts_arr))
            {
                continue;
            }
            int pt_cnt = cJSON_GetArraySize(pts_arr);
            if (pt_cnt < 2)
            {
                continue;
            }
            lv_point_t *pts = (lv_point_t *)lv_mem_alloc(sizeof(lv_point_t) * pt_cnt);
            if (!pts)
            {
                continue;
            }

            bool parse_success = true;
            for (int i = 0; i < pt_cnt; i++)
            {
                cJSON *p = cJSON_GetArrayItem(pts_arr, i);
                if (!cJSON_IsObject(p))
                {
                    parse_success = false;
                    break;
                }

                cJSON *x_obj = cJSON_GetObjectItem(p, "x");
                cJSON *y_obj = cJSON_GetObjectItem(p, "y");
                if (!cJSON_IsNumber(x_obj) || !cJSON_IsNumber(y_obj))
                {
                    parse_success = false;
                    break;
                }

                pts[i].x = clamp_i16(x_obj->valueint, 0, 239);
                pts[i].y = clamp_i16(y_obj->valueint, 0, 283);
            }

            if (!parse_success)
            {
                lv_mem_free(pts);
                continue;
            }

            cJSON *fill_obj = cJSON_GetObjectItem(shape_item, "fill_color");
            cJSON *line_obj = cJSON_GetObjectItem(shape_item, "outline_color");
            bool is_closed = (pts[0].x == pts[pt_cnt - 1].x && pts[0].y == pts[pt_cnt - 1].y);
            const char *outline_str = cJSON_IsString(line_obj) ? line_obj->valuestring : "white";
            lv_color_t color_outline = get_lv_color_from_str(outline_str);

            if (pt_cnt >= 3 && is_closed && cJSON_IsString(fill_obj))
            {
                lv_draw_rect_dsc_t fill_dsc;
                lv_draw_rect_dsc_init(&fill_dsc);
                fill_dsc.bg_color = get_lv_color_from_str(fill_obj->valuestring);
                fill_dsc.bg_opa = LV_OPA_COVER;
                fill_dsc.border_width = 0;
                lv_canvas_draw_polygon(global_canvas, pts, pt_cnt, &fill_dsc);
            }

            lv_draw_line_dsc_t stroke_dsc;
            lv_draw_line_dsc_init(&stroke_dsc);
            stroke_dsc.color = color_outline;
            stroke_dsc.width = 2;
            stroke_dsc.round_start = true;
            stroke_dsc.round_end = true;
            for (int i = 0; i < pt_cnt - 1; i++)
            {
                lv_point_t segment[2];
                segment[0] = pts[i];
                segment[1] = pts[i + 1];
                lv_canvas_draw_line(global_canvas, segment, 2, &stroke_dsc);
            }

            lv_mem_free(pts);
        }
        lvgl_port_unlock();
        snprintf(output, output_size, "Success: Shapes drawn on canvas.");
    }
    else
    {
        snprintf(output, output_size, "Error: LVGL system busy.");
    }
    cJSON_Delete(root);
    return ESP_OK;
}

/**
 * @brief Agent Tool: Clears all objects and resets the 240x284 LCD screen
 * @param input_json JSON Data
 * @param output  Result message buffer for the Agent
 * @param output_size Size of the output buffer
 * @return ESP_OK on success
 */
esp_err_t tool_clear_screen_execute(const char *input_json, char *output, size_t output_size)
{
    cJSON *root = cJSON_Parse(input_json);
    if (!root)
    {
        snprintf(output, output_size, "Error: Invalid JSON format");
        return ESP_ERR_INVALID_ARG;
    }

    if (lvgl_port_lock(0))
    {
        lv_obj_clean(lv_scr_act());
        global_canvas = NULL;
        if (canvas_buf != NULL)
        {
            free(canvas_buf);
            canvas_buf = NULL;
        }
        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
        lvgl_port_unlock();
        snprintf(output, output_size, "Success: Screen cleared, canvas reset.");
    }
    else
    {
        snprintf(output, output_size, "Error: LVGL system busy.");
    }

    cJSON_Delete(root);
    return ESP_OK;
}