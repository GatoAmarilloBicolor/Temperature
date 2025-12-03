#ifndef __TEMPERATURE_DEFS__
#define __TEMPERATURE_DEFS__

enum {
	M_DEVICE_CHANGED			= 'idev',
	M_SCALE_CHANGED				= 'k   ',
	M_STARTED_RUNNING			= 'strt',
	M_STOPPED_RUNNING			= 'stop',
	M_REFRESH_RATE				= 'rate',
	M_TEMPERATURE_REQUESTED		= 'temp',
	M_TEMPERATURE_REPLY,
	M_GRAPHVIEW_PAUSE			= 'paus',
	M_GRAPHVIEW_WATERMARK		= 'wter',
	M_GRAPHVIEW_COLOR_CHANGED	= 'PSTE',
	M_RESTORE_DEFAULTS			= 'rstr'
};

/* Application identity */
#define kAppName "Temperature"
#define kTemperatureSuiteMime "suite/vnd.Loa-Temperature"
#define kTemperatureConfigFile kAppName ".settings"
#define kTemperatureMime "application/x-vnd.Loa-Temperature"

/* Configuration names */
#define kConfigDevicePath   "device"
#define kConfigTempScale    "scale"
#define kConfigBaseWnd      "window:"
#define kConfigBaseGraph    "graph:"
#define kConfigWndFrame     kConfigBaseWnd   "frame"
#define kConfigWndPulse     kConfigBaseWnd   "pulse"
#define kConfigGraphRun     kConfigBaseGraph "running"
#define kConfigGraphWMark   kConfigBaseGraph "watermark"
#define kConfigGraphLColor  kConfigBaseGraph "line_color"

#endif /* __TEMPERATURE_DEFS__ */
