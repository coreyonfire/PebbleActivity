#include <pebble.h>
#include "arcDraw.h"

/*
 *  the stroke sizes control how thick the ring is
 *  the cutout size controls how big the black hole in the middle is
 *  the rings' diameter is controlled by how big the black hole is
 *
 *  to get colors go here http://developer.getpebble.com/tools/color-picker/
 *  there is already a stand up app, called Stand Up
 */
  
  
// my window
Window *my_window;

// my layers
Layer *innerRingLayer;
#define INNERRING_STROKE 10
#define INNERRING_COLOR GColorElectricBlue
#define INNERRING_UNSELECTED_COLOR GColorTiffanyBlue
  
Layer *middleRingLayer;
#define MIDDLERING_STROKE 10
#define MIDDLERING_COLOR GColorBrightGreen
#define MIDDLERING_UNSELECTED_COLOR GColorIslamicGreen
  
Layer *outerRingLayer;
#define OUTERRING_STROKE 10
#define OUTERRING_COLOR  GColorFolly
#define OUTERRING_UNSELECTED_COLOR GColorDarkCandyAppleRed
  
Layer *cutoutLayer;
#define CUTOUT_SIZE 80
  
TextLayer *text_layer;
#define TEXT_COLOR GColorWhite
TextLayer *center_text;
#define CENTER_COLOR GColorWhite
  
// selected ring index
static int selectedRing = 3;

// app sync object
static AppSync s_sync;
static uint8_t s_sync_buffer[100];
enum DataKey {
  STEP_COUNT = 0x2,
  WALK_RUN_COUNT = 0x1,
  FLOOR_COUNT = 0x0
};
  
// animation objects
#define ANIM_DURATION 1000
#define ANIM_DELAY 0
PropertyAnimation *innerRingAnimation;
AnimationImplementation innerAnimImpl;
PropertyAnimation *middleRingAnimation;
AnimationImplementation middleAnimImpl;
PropertyAnimation *outerRingAnimation;
AnimationImplementation outerAnimImpl;

// the ring's completion percentages
int innerPct = 0;
int middlePct = 0;
int outerPct = 0;
// these three will represent the current number of actions taken
int innerPctMax = 0;
int middlePctMax = 0;
int outerPctMax = 0;
// these three are your goals
int innerGoal = 10;
int middleGoal = 1000;
int outerGoal = 1000;

void animate_rings();

static void sync_changed_handler(const uint32_t key, const Tuple *new_tuple, const Tuple *old_tuple, void *context) {
  // Update UI here
  APP_LOG(APP_LOG_LEVEL_INFO, "Recieved new information.");
  
  switch (key) {
    case STEP_COUNT:
      innerPctMax = new_tuple->value->int32;
      APP_LOG(APP_LOG_LEVEL_INFO, "Setting innerPct to %d", innerPctMax);
      layer_mark_dirty(innerRingLayer);
      break;
    case WALK_RUN_COUNT:
      middlePctMax = new_tuple->value->int32;
      APP_LOG(APP_LOG_LEVEL_INFO, "Setting middlePct to %d", middlePctMax);
      layer_mark_dirty(middleRingLayer);
      break;
    case FLOOR_COUNT:
      outerPctMax = new_tuple->value->int32;
      APP_LOG(APP_LOG_LEVEL_INFO, "Setting outerPct to %d", outerPctMax);
      layer_mark_dirty(outerRingLayer);
      break;
  }
  char buf[10];
  snprintf(buf, 10, "IN: %d", (int)key);
  text_layer_set_text(center_text, buf);
  animate_rings();
}

static void sync_error_handler(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}



void inner_anim_update(struct Animation *anim, const uint32_t time) {
  uint32_t time_ms = time * 1000 / ANIMATION_NORMALIZED_MAX;
  innerPct = ((time_ms*1.0 / ANIM_DURATION  * (innerPctMax*1.0/innerGoal)) > 100 ? 100 :  (time_ms*1.0 / ANIM_DURATION  * (innerPctMax*1.0/innerGoal)));
  //APP_LOG(APP_LOG_LEVEL_INFO, "pct: %d, pctMax: %d, endPct: %d", (int) (time_ms*1.0 / ANIM_DURATION * 100), innerPctMax, innerPct);
  layer_mark_dirty(innerRingLayer);
}

void middle_anim_update(struct Animation *anim, const uint32_t time) {
  uint32_t time_ms = time * 1000 / ANIMATION_NORMALIZED_MAX;
  middlePct = ((time_ms*1.0 / ANIM_DURATION  * (middlePctMax*1.0/middleGoal)) > 100 ? 100 :  (time_ms*1.0 / ANIM_DURATION  * (middlePctMax*1.0/middleGoal)));
  //APP_LOG(APP_LOG_LEVEL_INFO, "pct: %d, pctMax: %d, endPct: %d", (int) (time_ms*1.0 / ANIM_DURATION * 100), innerPctMax, innerPct);
  layer_mark_dirty(middleRingLayer);
}

void outer_anim_update(struct Animation *anim, const uint32_t time) {
  uint32_t time_ms = time * 1000 / ANIMATION_NORMALIZED_MAX;
  outerPct = ((time_ms*1.0 / ANIM_DURATION  * (outerPctMax*1.0/outerGoal)) > 100 ? 100 :  (time_ms*1.0 / ANIM_DURATION  * (outerPctMax*1.0/outerGoal)));
  //APP_LOG(APP_LOG_LEVEL_INFO, "pct: %d, pctMax: %d, endPct: %d", (int) (time_ms*1.0 / ANIM_DURATION * 100), innerPctMax, innerPct);
  layer_mark_dirty(outerRingLayer);
}
  
void innerRing_update_callback(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  // turn the percent into an angle
  int angle = innerPct / 100.0 * TRIG_MAX_ANGLE;
  angle = angle >= 100.0 ? angle - 1 : angle;
  APP_LOG(APP_LOG_LEVEL_INFO, "Updating inner ring to %d", angle);
  // -1 to prevent the little bit of clipping
  const int16_t half_h = (bounds.size.h-1) / 2;
  #ifdef PBL_COLOR
    graphics_draw_arc(ctx, GPoint(half_h, half_h), half_h, INNERRING_STROKE, angle_270, angle_270 + angle, (selectedRing == 2 || selectedRing == 3) ? INNERRING_COLOR : INNERRING_UNSELECTED_COLOR);
  #else
    graphics_draw_arc(ctx, GPoint(half_h, half_h), half_h, INNERRING_STROKE, angle_270, angle_270 + angle, GColorWhite);
  #endif
}

void middleRing_update_callback(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  // turn the percent into an angle
  int angle = middlePct / 100.0 * TRIG_MAX_ANGLE;
  angle = angle >= 100.0 ? angle - 1 : angle;
  // -1 to prevent the little bit of clipping
  const int16_t half_h = (bounds.size.h-1) / 2;
  #ifdef PBL_COLOR
    graphics_draw_arc(ctx, GPoint(half_h, half_h), half_h, MIDDLERING_STROKE, angle_270, angle_270 + angle, (selectedRing == 1 || selectedRing == 3) ? MIDDLERING_COLOR : MIDDLERING_UNSELECTED_COLOR);
  #else
    graphics_draw_arc(ctx, GPoint(half_h, half_h), half_h, MIDDLERING_STROKE, angle_270, angle_270 + angle, GColorWhite);
  #endif
}

void outerRing_update_callback(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  // turn the percent into an angle
  int angle = outerPct / 100.0 * TRIG_MAX_ANGLE;
  angle = angle >= 100.0 ? angle - 1 : angle;
  // -1 to prevent the little bit of clipping
  const int16_t half_h = (bounds.size.h-1) / 2;
  #ifdef PBL_COLOR
    graphics_draw_arc(ctx, GPoint(half_h, half_h), half_h, OUTERRING_STROKE, angle_270, angle_270 + angle, (selectedRing == 0 || selectedRing == 3) ? OUTERRING_COLOR : OUTERRING_UNSELECTED_COLOR);
  #else
    graphics_draw_arc(ctx, GPoint(half_h, half_h), half_h, OUTERRING_STROKE, angle_270, angle_270 + angle, GColorWhite);
  #endif
}

void update_selection() {
  char *textChars;
  char centerText[50];
  switch (selectedRing) {
    case 0: textChars = "Steps"; snprintf(centerText, 50, "%d/%d", outerPct, outerGoal); break;
    case 1: textChars = "Walk/Run"; snprintf(centerText, 50, "%d/%d", middlePct, middleGoal); break;
    case 2: textChars = "Floors"; snprintf(centerText, 50, "%d/%d", innerPct, innerGoal); break;
    default : textChars = "Activity"; snprintf(centerText, 50, "\n"); 
  };
  #ifdef PBL_COLOR
    GColor textColor;
    switch (selectedRing) {
      case 0: textColor = OUTERRING_COLOR; 
              break;
      case 1: textColor = MIDDLERING_COLOR; 
              break;
      case 2: textColor = INNERRING_COLOR; 
              break;
      default : textColor = TEXT_COLOR;
    };
    text_layer_set_text_color(text_layer, textColor);
    text_layer_set_text_color(center_text, textColor);
  #endif
    
  text_layer_set_text(text_layer, textChars);
  text_layer_set_text(center_text, centerText);
}

// the way this works: clicking up and down cycles rings. clicking center un-selects.
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  selectedRing = (selectedRing - 1) % 3 >= 0 ? (selectedRing - 1) % 3 : 2;
  update_selection();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  selectedRing = 3;
  update_selection();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  // to make it go to outer ring instead of middle ring after setting selectedRing to 3
  selectedRing = ((selectedRing < 3 ? selectedRing : 2) + 1) % 3;
  update_selection();
}

static void click_config_provider(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  
  
  
  text_layer = text_layer_create(GRect(0, 0, 144, 20));
  GSize center_size = graphics_text_layout_get_content_size("10,000\n/10,000", fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(0,0, CUTOUT_SIZE/2, CUTOUT_SIZE/2), GTextOverflowModeWordWrap, GTextAlignmentCenter);
  center_text = text_layer_create(GRect((bounds.size.w-(center_size.w+5))/2, (bounds.size.w-(center_size.h+5))/2+10, center_size.w+5, center_size.h+5));
  
  #ifdef PBL_COLOR 
    text_layer_set_text_color(text_layer, TEXT_COLOR);
    text_layer_set_text_color(center_text, CENTER_COLOR);
  #else
    text_layer_set_text_color(text_layer, GColorWhite);
    text_layer_set_text_color(center_text, GColorWhite);
  #endif
  text_layer_set_background_color(text_layer, GColorClear);
  text_layer_set_background_color(center_text, GColorClear);
  
  text_layer_set_text_alignment(center_text, GTextAlignmentCenter);
  
  text_layer_set_text(text_layer, "Activity");
  text_layer_set_text(center_text, "");
  
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
  layer_insert_below_sibling(text_layer_get_layer(center_text), text_layer_get_layer(text_layer));
  

  // create any layers i'll need and insert them
  int innerSize = CUTOUT_SIZE + INNERRING_STROKE;
  innerRingLayer = layer_create((GRect((bounds.size.w-innerSize)/2, (bounds.size.w-innerSize)/2+10, innerSize, innerSize)));
  layer_set_update_proc(innerRingLayer, innerRing_update_callback);
  layer_add_child(window_layer, innerRingLayer);
  
  // times 2 because inner ring is 20 wide
  int middleSize = INNERRING_STROKE*2 + CUTOUT_SIZE + MIDDLERING_STROKE+3;
  middleRingLayer = layer_create((GRect((bounds.size.w-middleSize)/2, (bounds.size.w-middleSize)/2+10, middleSize, middleSize)));
  layer_set_update_proc(middleRingLayer, middleRing_update_callback);
  layer_insert_below_sibling(middleRingLayer, innerRingLayer);
  
  // outer
  int outerSize = INNERRING_STROKE*2 + CUTOUT_SIZE + MIDDLERING_STROKE*2 + OUTERRING_STROKE + 8;
  outerRingLayer = layer_create((GRect((bounds.size.w-outerSize)/2, (bounds.size.w-outerSize)/2+10, outerSize, outerSize)));
  layer_set_update_proc(outerRingLayer, outerRing_update_callback);
  layer_insert_below_sibling(outerRingLayer, middleRingLayer);
}

void main_window_unload(Window *window) {
  // destroy any layers that the window contains
  text_layer_destroy(text_layer);
  text_layer_destroy(center_text);
  layer_destroy(innerRingLayer);
  layer_destroy(outerRingLayer);
  layer_destroy(middleRingLayer);
}

void animate_rings() {
  // kick off animations
  // inner
  GRect pos = layer_get_frame(innerRingLayer);
  innerRingAnimation = property_animation_create_layer_frame(innerRingLayer, &pos, &pos);
  animation_set_duration((Animation*)innerRingAnimation, ANIM_DURATION);
  animation_set_curve((Animation*)innerRingAnimation, AnimationCurveEaseInOut);
  innerAnimImpl.update = *inner_anim_update;
  animation_set_implementation(property_animation_get_animation(innerRingAnimation), &innerAnimImpl);
  animation_schedule((Animation*)innerRingAnimation);
  
  // middle
  pos = layer_get_frame(middleRingLayer);
  middleRingAnimation = property_animation_create_layer_frame(middleRingLayer, &pos, &pos);
  animation_set_duration((Animation*)middleRingAnimation, ANIM_DURATION);
  animation_set_curve((Animation*)middleRingAnimation, AnimationCurveEaseInOut);
  middleAnimImpl.update = *middle_anim_update;
  animation_set_implementation(property_animation_get_animation(middleRingAnimation), &middleAnimImpl);
  animation_schedule((Animation*)middleRingAnimation);
  
  // outer
  pos = layer_get_frame(outerRingLayer);
  outerRingAnimation = property_animation_create_layer_frame(outerRingLayer, &pos, &pos);
  animation_set_duration((Animation*)outerRingAnimation, ANIM_DURATION);
  animation_set_curve((Animation*)outerRingAnimation, AnimationCurveEaseInOut);
  outerAnimImpl.update = *outer_anim_update;
  animation_set_implementation(property_animation_get_animation(outerRingAnimation), &outerAnimImpl);
  animation_schedule((Animation*)outerRingAnimation);
}

void handle_init(void) {
  my_window = window_create();
  window_set_background_color(my_window, GColorBlack);
  window_set_window_handlers(my_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  
  window_stack_push(my_window, true);
  
  //set up ring select click actions
  window_set_click_config_provider(my_window, click_config_provider);
  
  animate_rings();
  
  // start up app sync
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  // Setup initial values
  Tuplet initial_values[] = {
    TupletInteger(STEP_COUNT, 0),
    TupletInteger(WALK_RUN_COUNT, 0),
    TupletInteger(FLOOR_COUNT, 0),
  };

  // Begin using AppSync
  app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer), initial_values, ARRAY_LENGTH(initial_values), sync_changed_handler, sync_error_handler, NULL);
}

void handle_deinit(void) {
  animation_unschedule_all();
  window_destroy(my_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}

