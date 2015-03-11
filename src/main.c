#include <pebble.h>

/*
 *  the stroke sizes control how thick the ring is
 *  the cutout size controls how big the black hole in the middle is
 *  the rings' diameter is controlled by how big the black hole is
 *
 *  to get colors go here http://developer.getpebble.com/tools/color-picker/
 */
  
// some geometry constants
static int angle_90 = TRIG_MAX_ANGLE / 4;
static int angle_180 = TRIG_MAX_ANGLE / 2;
static int angle_270 = 3 * TRIG_MAX_ANGLE / 4;
  
// my window
Window *my_window;

// my layers
Layer *innerRingLayer;
#define INNERRING_STROKE 10
#define INNERRING_COLOR GColorElectricBlue
  
Layer *middleRingLayer;
#define MIDDLERING_STROKE 10
#define MIDDLERING_COLOR GColorBrightGreen
  
Layer *outerRingLayer;
#define OUTERRING_STROKE 10
#define OUTERRING_COLOR  GColorFolly
  
Layer *cutoutLayer;
#define CUTOUT_SIZE 80
TextLayer *text_layer;
#define TEXT_COLOR GColorElectricBlue
  
// the ring's completion percentages
int innerPct = 45;
int middlePct = 66;
int outerPct = 32;

// app sync object
static AppSync s_sync;
static uint8_t s_sync_buffer[32];
#define KEY_COUNT 5
  

/*\
|*| DrawArc function thanks to Cameron MacFarland (http://forums.getpebble.com/profile/12561/Cameron%20MacFarland)
\*/
static void graphics_draw_arc(GContext *ctx, GPoint center, int radius, int thickness, int start_angle, int end_angle, GColor c) {
	int32_t xmin = 65535000, xmax = -65535000, ymin = 65535000, ymax = -65535000;
	int32_t cosStart, sinStart, cosEnd, sinEnd;
	int32_t r, t;
	
	while (start_angle < 0) start_angle += TRIG_MAX_ANGLE;
	while (end_angle < 0) end_angle += TRIG_MAX_ANGLE;

	start_angle %= TRIG_MAX_ANGLE;
	end_angle %= TRIG_MAX_ANGLE;
	
	if (end_angle == 0) end_angle = TRIG_MAX_ANGLE;
	
	if (start_angle > end_angle) {
		graphics_draw_arc(ctx, center, radius, thickness, start_angle, TRIG_MAX_ANGLE, c);
		graphics_draw_arc(ctx, center, radius, thickness, 0, end_angle, c);
	} else {
		// Calculate bounding box for the arc to be drawn
		cosStart = cos_lookup(start_angle);
		sinStart = sin_lookup(start_angle);
		cosEnd = cos_lookup(end_angle);
		sinEnd = sin_lookup(end_angle);
		
		r = radius;
		// Point 1: radius & start_angle
		t = r * cosStart;
		if (t < xmin) xmin = t;
		if (t > xmax) xmax = t;
		t = r * sinStart;
		if (t < ymin) ymin = t;
		if (t > ymax) ymax = t;

		// Point 2: radius & end_angle
		t = r * cosEnd;
		if (t < xmin) xmin = t;
		if (t > xmax) xmax = t;
		t = r * sinEnd;
		if (t < ymin) ymin = t;
		if (t > ymax) ymax = t;
		
		r = radius - thickness;
		// Point 3: radius-thickness & start_angle
		t = r * cosStart;
		if (t < xmin) xmin = t;
		if (t > xmax) xmax = t;
		t = r * sinStart;
		if (t < ymin) ymin = t;
		if (t > ymax) ymax = t;

		// Point 4: radius-thickness & end_angle
		t = r * cosEnd;
		if (t < xmin) xmin = t;
		if (t > xmax) xmax = t;
		t = r * sinEnd;
		if (t < ymin) ymin = t;
		if (t > ymax) ymax = t;
		
		// Normalization
		xmin /= TRIG_MAX_RATIO;
		xmax /= TRIG_MAX_RATIO;
		ymin /= TRIG_MAX_RATIO;
		ymax /= TRIG_MAX_RATIO;
				
		// Corrections if arc crosses X or Y axis
		if ((start_angle < angle_90) && (end_angle > angle_90)) {
			ymax = radius;
		}
		
		if ((start_angle < angle_180) && (end_angle > angle_180)) {
			xmin = -radius;
		}
		
		if ((start_angle < angle_270) && (end_angle > angle_270)) {
			ymin = -radius;
		}
		
		// Slopes for the two sides of the arc
		float sslope = (float)cosStart/ (float)sinStart;
		float eslope = (float)cosEnd / (float)sinEnd;
	 
		if (end_angle == TRIG_MAX_ANGLE) eslope = -1000000;
	 
		int ir2 = (radius - thickness) * (radius - thickness);
		int or2 = radius * radius;
	 
		graphics_context_set_stroke_color(ctx, c);

		for (int x = xmin; x <= xmax; x++) {
			for (int y = ymin; y <= ymax; y++)
			{
				int x2 = x * x;
				int y2 = y * y;
	 
				if (
					(x2 + y2 < or2 && x2 + y2 >= ir2) && (
						(y > 0 && start_angle < angle_180 && x <= y * sslope) ||
						(y < 0 && start_angle > angle_180 && x >= y * sslope) ||
						(y < 0 && start_angle <= angle_180) ||
						(y == 0 && start_angle <= angle_180 && x < 0) ||
						(y == 0 && start_angle == 0 && x > 0)
					) && (
						(y > 0 && end_angle < angle_180 && x >= y * eslope) ||
						(y < 0 && end_angle > angle_180 && x <= y * eslope) ||
						(y > 0 && end_angle >= angle_180) ||
						(y == 0 && end_angle >= angle_180 && x < 0) ||
						(y == 0 && start_angle == 0 && x > 0)
					)
				)
				graphics_draw_pixel(ctx, GPoint(center.x+x, center.y+y));
			}
		}
	}
}

static void sync_changed_handler(const uint32_t key, const Tuple *new_tuple, const Tuple *old_tuple, void *context) {
  // Update UI here
  APP_LOG(APP_LOG_LEVEL_INFO, "Recieved new information.");
}

static void sync_error_handler(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  // An error occured!
  APP_LOG(APP_LOG_LEVEL_ERROR, "ERROR OCCURRED IN SYNC.");
}
  
void innerRing_update_callback(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  // turn the percent into an angle
  int angle = innerPct / 100.0 * TRIG_MAX_ANGLE;
  // -1 to prevent the little bit of clipping
  const int16_t half_h = (bounds.size.h-1) / 2;
  #ifdef PBL_COLOR
    graphics_draw_arc(ctx, GPoint(half_h, half_h), half_h, INNERRING_STROKE, angle_270, angle_270 + angle, INNERRING_COLOR);
  #else
    graphics_draw_arc(ctx, GPoint(half_h, half_h), half_h, INNERRING_STROKE, angle_270, angle_270 + angle, GColorWhite);
  #endif
}

void middleRing_update_callback(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  // turn the percent into an angle
  int angle = middlePct / 100.0 * TRIG_MAX_ANGLE;
  // -1 to prevent the little bit of clipping
  const int16_t half_h = (bounds.size.h-1) / 2;
  #ifdef PBL_COLOR
    graphics_draw_arc(ctx, GPoint(half_h, half_h), half_h, MIDDLERING_STROKE, angle_270, angle_270 + angle, MIDDLERING_COLOR);
  #else
    graphics_draw_arc(ctx, GPoint(half_h, half_h), half_h, MIDDLERING_STROKE, angle_270, angle_270 + angle, GColorWhite);
  #endif
}

void outerRing_update_callback(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  // turn the percent into an angle
  int angle = outerPct / 100.0 * TRIG_MAX_ANGLE;
  // -1 to prevent the little bit of clipping
  const int16_t half_h = (bounds.size.h-1) / 2;
  #ifdef PBL_COLOR
    graphics_draw_arc(ctx, GPoint(half_h, half_h), half_h, OUTERRING_STROKE, angle_270, angle_270 + angle, OUTERRING_COLOR);
  #else
    graphics_draw_arc(ctx, GPoint(half_h, half_h), half_h, OUTERRING_STROKE, angle_270, angle_270 + angle, GColorWhite);
  #endif
}

void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  
  text_layer = text_layer_create(GRect(0, 0, 144, 20));
  
  #ifdef PBL_COLOR 
    text_layer_set_text_color(text_layer, TEXT_COLOR);
  #else
    text_layer_set_text_color(text_layer, GColorWhite);
  #endif
  text_layer_set_background_color(text_layer, GColorBlack);
  
  text_layer_set_text(text_layer, "Activity");
  layer_add_child(window_layer, text_layer_get_layer(text_layer));

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
  layer_destroy(innerRingLayer);
  layer_destroy(outerRingLayer);
  layer_destroy(middleRingLayer);
}

void handle_init(void) {
  my_window = window_create();
  window_set_background_color(my_window, GColorBlack);
  window_set_window_handlers(my_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  
  window_stack_push(my_window, true);
  
  
  // start up app sync
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  // Setup initial values
  Tuplet initial_values[] = {
    TupletInteger(KEY_COUNT, 0),
  };

  // Begin using AppSync
  app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer), initial_values, ARRAY_LENGTH(initial_values), sync_changed_handler, sync_error_handler, NULL);
}

void handle_deinit(void) {
  window_destroy(my_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
