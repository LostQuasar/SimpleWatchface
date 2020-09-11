#include <pebble.h>

static Window *s_main_window;
static Layer *s_battery_layer;
static TextLayer *s_time_layer;
static GFont s_time_font;
static int s_battery_level;
static bool s_battery_charging;

static void battery_callback(BatteryChargeState state) {
  s_battery_level = state.charge_percent;
  s_battery_charging = state.is_charging;
  layer_mark_dirty(s_battery_layer);
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Find the width of the bar
  int width = (s_battery_level * bounds.size.w) / 100;

  // Draw the background
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Draw the bar
  graphics_context_set_fill_color(ctx, s_battery_charging ? GColorChromeYellow : GColorGreen);
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LEXEND_DECA_48));
  s_battery_layer = layer_create(GRect(0,0, bounds.size.w, bounds.size.h));
  layer_set_update_proc(s_battery_layer, battery_update_proc);

  layer_add_child(window_get_root_layer(window), s_battery_layer);

  s_time_layer = text_layer_create (
    GRect(0, PBL_IF_ROUND_ELSE(58,52), bounds.size.w, 50)
  );

  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_unload(Window *window) {
  fonts_unload_custom_font(s_time_font);
}

static void init() {
  s_main_window = window_create();
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_callback);

  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
  update_time();
  battery_callback(battery_state_service_peek());
}

static void deinit() {
  text_layer_destroy(s_time_layer);
  window_destroy(s_main_window);
  layer_destroy(s_battery_layer);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
