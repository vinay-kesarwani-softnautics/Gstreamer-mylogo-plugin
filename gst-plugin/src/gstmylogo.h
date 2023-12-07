#ifndef __GST_MYLOGO_H__
#define __GST_MYLOGO_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_MYLOGO (gst_mylogo_get_type())
G_DECLARE_FINAL_TYPE (Gstmylogo, gst_mylogo,
    GST, MYLOGO, GstElement)

struct _Gstmylogo
{
  GstElement element;
  GstPad *sinkpad;
  GstPad *srcpad;
  gchar *logo_file;
  gint logo_x;
  gint logo_y;
  gint rotation_direction;
  gdouble rotation_angle;
  GstClockTime rotation_speed, start_time;
  gdouble alpha;
  gint scroll_up;
  gint scroll_down;
  gint scroll_left;
  gint scroll_right;
  gchar *scroll_direction;
  gint scroll_speed;
};

G_END_DECLS

#endif /* __GST_MYLOGO_H__ */
