#include <gst/gst.h>
#include <gst/video/video.h>
#include <cairo.h>
#include "gstmylogo.h"
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
// #include <stdio.h>

GST_DEBUG_CATEGORY_STATIC(gst_mylogo_debug);
#define GST_CAT_DEFAULT gst_mylogo_debug

enum
{
    PROP_0,
    PROP_LOGO_FILE,
    PROP_LOGO_X,
    PROP_LOGO_Y,
    PROP_ROTATION_ANGLE,
    PROP_ROTATION_DIRECTION,
    PROP_ROTATION_SPEED,
    PROP_ALPHA,
    PROP_SCROLL_DIRECTION,
    PROP_SCROLL_SPEED,
};

static GstStaticPadTemplate
    sink_factory = GST_STATIC_PAD_TEMPLATE("sink",
                                           GST_PAD_SINK,
                                           GST_PAD_ALWAYS,
                                           GST_STATIC_CAPS("video/x-raw, format=(string)NV12, width=(int)[1, 1920], height=(int)[1, 1080], framerate=(fraction)[0/1, 60/1]"));

static GstStaticPadTemplate
    src_factory = GST_STATIC_PAD_TEMPLATE("src",
                                          GST_PAD_SRC,
                                          GST_PAD_ALWAYS,
                                          GST_STATIC_CAPS("video/x-raw, format=(string)NV12, width=(int)[1, 1920], height=(int)[1, 1080], framerate=(fraction)[0/1, 60/1]"));

G_DEFINE_TYPE(Gstmylogo, gst_mylogo, GST_TYPE_ELEMENT);
GST_ELEMENT_REGISTER_DEFINE(mylogo, "mylogo", GST_RANK_NONE, GST_TYPE_MYLOGO);

#define DEFAULT_LOGO_FILE "logo4.png"
#define DEFAULT_LOGO_X 100
#define DEFAULT_LOGO_Y 100
#define DEFAULT_ROTATION_ANGLE 0
#define DEFAULT_ROTATION_DIRECTION 1
#define DEFAULT_ROTATION_SPEED "medium"
#define DEFAULT_ALPHA 1.0
#define DEFAULT_SCROLL_SPEED 0

#define ROTATION_SPEED_SLOW 50
#define ROTATION_SPEED_MEDIUM 90
#define ROTATION_SPEED_FAST 140

int frame_count = 0;
int current_scroll_position = 0;

static void gst_mylogo_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_mylogo_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

static gboolean gst_mylogo_sink_event(GstPad *pad, GstObject *parent, GstEvent *event);
static GstFlowReturn gst_mylogo_chain(GstPad *pad, GstObject *parent, GstBuffer *buf);

static void gst_mylogo_class_init(GstmylogoClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstElementClass *gstelement_class = GST_ELEMENT_CLASS(klass);

    gobject_class->set_property = gst_mylogo_set_property;
    gobject_class->get_property = gst_mylogo_get_property;

    g_object_class_install_property(gobject_class, PROP_LOGO_FILE,
                                    g_param_spec_string("logo_file", "Logo File",
                                                        "The path to the logo image file (PNG) with width <=260 and height <= 260",
                                                        DEFAULT_LOGO_FILE, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_LOGO_X,
                                    g_param_spec_int("logo_x", "X Position", "X position for logo overlay",
                                                     0, G_MAXINT, DEFAULT_LOGO_X, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_LOGO_Y,
                                    g_param_spec_int("logo_y", "Y Position", "Y position for logo overlay",
                                                     0, G_MAXINT, DEFAULT_LOGO_Y, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_ROTATION_ANGLE,
                                    g_param_spec_int("rotation_angle", "Rotation Angle",
                                                     "Rotation angle for the logo (clockwise)",
                                                     -360, 360, DEFAULT_ROTATION_ANGLE, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_ROTATION_DIRECTION,
                                    g_param_spec_int("rotation_direction", "Rotation Direction",
                                                     "Rotation direction for the logo (1 for clockwise, -1 for counterclockwise)",
                                                     -1, 1, DEFAULT_ROTATION_DIRECTION, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_ROTATION_SPEED,
                                    g_param_spec_string("rotation_speed", "Rotation Speed",
                                                        "Rotation speed for the logo ('slow', 'medium', 'fast', etc.)",
                                                        DEFAULT_ROTATION_SPEED, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_ALPHA,
                                    g_param_spec_double("alpha", "Alpha",
                                                        "Alpha blending level for the logo (0.0 - 1.0)",
                                                        0.0, 1.0, DEFAULT_ALPHA, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_SCROLL_DIRECTION,
                                    g_param_spec_string("scroll_direction", "Scroll Direction",
                                                        "Scroll direction for the logo ('up', 'down', 'left', 'right')",
                                                        NULL, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_SCROLL_SPEED,
                                    g_param_spec_int("scroll_speed", "Scroll Speed",
                                                     "Scroll speed for the logo",
                                                     -0, G_MAXINT, DEFAULT_SCROLL_SPEED, G_PARAM_READWRITE));

    gst_element_class_set_details_simple(gstelement_class, "mylogo", "Filter/Effect/Video",
                                         "Imposes a logo on video", "Your Name <youremail@domain.com>");

    gst_element_class_add_pad_template(gstelement_class, gst_static_pad_template_get(&src_factory));
    gst_element_class_add_pad_template(gstelement_class, gst_static_pad_template_get(&sink_factory));
}

static void gst_mylogo_init(Gstmylogo *filter)
{
    const char *current_dir = g_get_current_dir();
    gchar *absolute_path = g_build_filename(current_dir, DEFAULT_LOGO_FILE, NULL);

    filter->logo_file = g_strdup(absolute_path);
    g_free(absolute_path);
    if (filter->logo_file == NULL)
    {
        g_critical("Failed to allocate memory for logo_file.");
    }

    filter->logo_x = DEFAULT_LOGO_X;
    filter->logo_y = DEFAULT_LOGO_Y;
    filter->rotation_angle = DEFAULT_ROTATION_ANGLE;
    filter->rotation_direction = DEFAULT_ROTATION_DIRECTION;
    filter->rotation_speed = g_strdup(DEFAULT_ROTATION_SPEED);
    filter->alpha = DEFAULT_ALPHA;
    if (filter->rotation_speed == NULL)
    {
        g_critical("Failed to allocate memory for rotation_speed.");
    }
    filter->scroll_direction = NULL;
    filter->scroll_speed = DEFAULT_SCROLL_SPEED;
    filter->start_time = 0;

    filter->sinkpad = gst_pad_new_from_static_template(&sink_factory, "sink");
    gst_pad_set_event_function(filter->sinkpad, GST_DEBUG_FUNCPTR(gst_mylogo_sink_event));
    gst_pad_set_chain_function(filter->sinkpad, GST_DEBUG_FUNCPTR(gst_mylogo_chain));
    GST_PAD_SET_PROXY_CAPS(filter->sinkpad);
    gst_element_add_pad(GST_ELEMENT(filter), filter->sinkpad);

    filter->srcpad = gst_pad_new_from_static_template(&src_factory, "src");
    GST_PAD_SET_PROXY_CAPS(filter->srcpad);
    gst_element_add_pad(GST_ELEMENT(filter), filter->srcpad);
}

static void gst_mylogo_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    Gstmylogo *filter = GST_MYLOGO(object);

    switch (prop_id)
    {
    case PROP_LOGO_FILE:
        g_free(filter->logo_file);
        filter->logo_file = g_value_dup_string(value);
        if (filter->logo_file == NULL)
        {
            g_critical("Failed to allocate memory for logo_file.");
        }
        break;
    case PROP_LOGO_X:
        filter->logo_x = g_value_get_int(value);
        break;
    case PROP_LOGO_Y:
        filter->logo_y = g_value_get_int(value);
        break;
    case PROP_ROTATION_ANGLE:
        filter->rotation_angle = g_value_get_int(value);
        break;
    case PROP_ROTATION_DIRECTION:
        filter->rotation_direction = g_value_get_int(value);
        break;
    case PROP_ROTATION_SPEED:
        g_free(filter->rotation_speed);
        filter->rotation_speed = g_value_dup_string(value);
        if (filter->rotation_speed == NULL)
        {
            g_critical("Failed to allocate memory for rotation_speed.");
        }
        break;
    case PROP_ALPHA:
        filter->alpha = g_value_get_double(value);
        break;
    case PROP_SCROLL_SPEED:
        filter->scroll_speed = g_value_get_int(value);
        break;
    case PROP_SCROLL_DIRECTION:
        g_free(filter->scroll_direction);
        filter->scroll_direction = g_value_dup_string(value);
        if (filter->scroll_direction == NULL)
        {
            g_critical("Failed to allocate memory for scroll_direction.");
        }
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void gst_mylogo_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    Gstmylogo *filter = GST_MYLOGO(object);

    switch (prop_id)
    {
    case PROP_LOGO_FILE:
        g_value_set_string(value, filter->logo_file);
        break;
    case PROP_LOGO_X:
        g_value_set_int(value, filter->logo_x);
        break;
    case PROP_LOGO_Y:
        g_value_set_int(value, filter->logo_y);
        break;
    case PROP_ROTATION_ANGLE:
        g_value_set_int(value, filter->rotation_angle);
        break;
    case PROP_ROTATION_DIRECTION:
        g_value_set_int(value, filter->rotation_direction);
        break;
    case PROP_ROTATION_SPEED:
        g_value_set_string(value, filter->rotation_speed);
        break;
    case PROP_ALPHA:
        g_value_set_double(value, filter->alpha);
        break;
    case PROP_SCROLL_SPEED:
        g_value_set_int(value, filter->scroll_speed);
        break;
    case PROP_SCROLL_DIRECTION:
        g_value_set_string(value, filter->scroll_direction);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static gboolean gst_mylogo_sink_event(GstPad *pad, GstObject *parent, GstEvent *event)
{
    gboolean ret;

    switch (GST_EVENT_TYPE(event))
    {
    case GST_EVENT_CAPS:
    {
        GstCaps *caps;
        gst_event_parse_caps(event, &caps);
        // Forward the event
        ret = gst_pad_event_default(pad, parent, event);
        break;
    }
    default:
        ret = gst_pad_event_default(pad, parent, event);
        break;
    }
    return ret;
}

static gboolean rotation_done = FALSE;

static gboolean gst_mylogo_chain(GstPad *pad, GstObject *parent, GstBuffer *buf)
{
    Gstmylogo *filter = GST_MYLOGO(parent);

    // Check if the logo_file property is set and valid
    if (filter->logo_file == NULL || filter->logo_file[0] == '\0')
    {
        g_warning("Logo file not set. Please set the 'logo_file' property.");
        return GST_FLOW_ERROR;
    }

    // Load the logo image using Cairo
    cairo_surface_t *image = cairo_image_surface_create_from_png(filter->logo_file);

    // Check if the image loaded successfully
    if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS)
    {
        g_warning("Failed to load logo image from %s", filter->logo_file);
        return GST_FLOW_ERROR;
    }

    // Get the video frame's properties from the buffer's caps
    GstCaps *caps = gst_pad_get_current_caps(pad);
    GstVideoInfo info;
    GstVideoFrame frame;

    if (!gst_video_info_from_caps(&info, caps))
    {
        g_warning("Failed to get video info from buffer caps.");
        gst_caps_unref(caps);
        return GST_FLOW_ERROR;
    }

    if (!gst_video_frame_map(&frame, &info, buf, GST_MAP_READWRITE))
    {
        g_warning("Failed to map the video buffer.");
        return GST_FLOW_ERROR;
    }

    // Get y and uv plane information
    guint8 *y_plane = GST_VIDEO_FRAME_PLANE_DATA(&frame, 0);
    guint8 *uv_plane = GST_VIDEO_FRAME_PLANE_DATA(&frame, 1);

    gint image_x = filter->logo_x;
    gint image_y = filter->logo_y;

    // Get image width and height
    gint image_width = cairo_image_surface_get_width(image);
    gint image_height = cairo_image_surface_get_height(image);

    if (image_width <= 0 || image_width > 260 || image_height <= 0 || image_height > 260)
    {
        g_warning("Image dimensions are more than required.\n");
        return GST_FLOW_ERROR;
    }

    // Get rotation direction and speed from properties
    gint rotation_direction = filter->rotation_direction;
    const gchar *rotation_speed = filter->rotation_speed;

    // Calculate rotation angle based on speed
    GstClockTime now = gst_clock_get_time(gst_element_get_clock(GST_ELEMENT(parent)));
    GstClockTime elapsed_time = now - filter->start_time;

    gint rotation_angle = filter->rotation_angle;

    if (!rotation_done)
    {
        if (g_strcmp0(rotation_speed, "slow") == 0)
        {
            rotation_angle += rotation_direction * ROTATION_SPEED_SLOW * (now / GST_SECOND);
        }
        else if (g_strcmp0(rotation_speed, "medium") == 0)
        {
            rotation_angle += rotation_direction * ROTATION_SPEED_MEDIUM * (now / GST_SECOND);
        }
        else if (g_strcmp0(rotation_speed, "fast") == 0)
        {
            rotation_angle += rotation_direction * ROTATION_SPEED_FAST * (now / GST_SECOND);
        }

        // Mark rotation as done
        rotation_done = TRUE;
    }

    // Convert rotation angle to radians
    double rotation_angle_rad = rotation_angle * G_PI / 180.0;

    gdouble alpha = filter->alpha;

    const gchar *scroll_direction = filter->scroll_direction;

    // Get the frame number or timestamp from the buffer
    guint64 frame_number = GST_BUFFER_PTS(buf) / GST_SECOND;

    if (g_strcmp0(scroll_direction, "up") == 0)
    {
        if (frame_number < 1)
        {
            image_y = (filter->logo_y - filter->scroll_speed);
            current_scroll_position = image_y;
        }
        else
        {

            current_scroll_position = current_scroll_position - filter->scroll_speed;
            if (current_scroll_position < 0)
            {
                current_scroll_position = info.height - image_height;
            }
            image_y = current_scroll_position;
        }
    }
    else if (g_strcmp0(scroll_direction, "down") == 0)
    {
        if (frame_number < 1)
        {
            image_y = (filter->logo_y - filter->scroll_speed);
            current_scroll_position = image_y;
        }
        else
        {
            current_scroll_position = current_scroll_position + filter->scroll_speed;
            if (current_scroll_position > info.height - image_height)
            {
                current_scroll_position = 0;
            }
            image_y = current_scroll_position;
        }
    }
    else if (g_strcmp0(scroll_direction, "left") == 0)
    {
        if (frame_number < 1)
        {
            image_x = (filter->logo_x - filter->scroll_speed);
            current_scroll_position = image_x;
        }
        else
        {
            current_scroll_position = current_scroll_position - filter->scroll_speed;
            if (current_scroll_position < 0)
            {
                current_scroll_position = info.width - image_width;
            }
            image_x = current_scroll_position;
        }
    }
    else if (g_strcmp0(scroll_direction, "right") == 0)
    {
        if (frame_number < 1)
        {
            image_x = (filter->logo_x - filter->scroll_speed);
            current_scroll_position = image_x;
        }
        else
        {
            current_scroll_position = current_scroll_position + filter->scroll_speed;
            if (current_scroll_position > info.width - image_width)
            {
                current_scroll_position = 0;
            }
            image_x = current_scroll_position;
        }
    }

    // Iterate over each pixel of the image and overlay it onto the video frame
    for (gint y = 0; y < image_height; y++)
    {
        for (gint x = 0; x < image_width; x++)
        {
            // Calculate rotated coordinates around the center
            gint src_x = x - image_width / 2;
            gint src_y = y - image_height / 2;

            // Apply rotation to the coordinates
            gint rotated_x = cos(rotation_direction * rotation_angle_rad) * src_x - sin(rotation_direction * rotation_angle_rad) * src_y + image_width / 2;
            gint rotated_y = sin(rotation_direction * rotation_angle_rad) * src_x + cos(rotation_direction * rotation_angle_rad) * src_y + image_height / 2;

            // Apply scrolling
            rotated_x += image_x;
            rotated_y += image_y;

            gint dst_x = rotated_x;
            gint dst_y = rotated_y;

            if (dst_x >= 0 && dst_x < info.width && dst_y >= 0 && dst_y < info.height)
            {
                guint8 *y_ptr = y_plane + dst_y * info.stride[0] + dst_x;
                guint8 *uv_ptr = uv_plane + (dst_y / 2) * info.stride[1] + 2 * (dst_x / 2);

                uint8_t *data = cairo_image_surface_get_data(image);
                int stride = cairo_image_surface_get_stride(image);

                if (x >= 0 && x < image_width && y >= 0 && y < image_height)
                {
                    uint8_t *pixel = data + y * stride + x * 4;
                    uint8_t r = pixel[2];
                    uint8_t g = pixel[1];
                    uint8_t b = pixel[0];

                    // Apply alpha blending
                    guint8 y_value = (1 - alpha) * (*y_ptr) + alpha * (0.299 * r + 0.587 * g + 0.114 * b);
                    guint8 u_value = 0.492 * (b - y_value) + 128;
                    guint8 v_value = 0.877 * (r - y_value) + 128;

                    *y_ptr = y_value;
                    uv_ptr[0] = u_value;
                    uv_ptr[1] = v_value;
                }
            }
            else
            {
                g_warning("Overlay position is out of bounds.\n");
                return GST_FLOW_ERROR;
            }
        }
    }

    // Push the processed video buffer to the srcpad
    GstFlowReturn ret = gst_pad_push(filter->srcpad, buf);

    // Unmap the video buffer
    gst_video_frame_unmap(&frame);

    // Clean up Cairo resources
    cairo_surface_destroy(image);

    return ret;
}

static gboolean mylogo_init(GstPlugin *mylogo)
{
    GST_DEBUG_CATEGORY_INIT(gst_mylogo_debug, "mylogo",
                            0, "Template mylogo");

    return GST_ELEMENT_REGISTER(mylogo, mylogo);
}

#ifndef PACKAGE
#define PACKAGE "mylogo"
#endif

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  mylogo,
                  "Imposes a logo on video",
                  mylogo_init,
                  "1.0",
                  "LGPL",
                  "mylogo",
                  "Vinay Kesarwani <vinay.kesarwani@softnautics.com>");