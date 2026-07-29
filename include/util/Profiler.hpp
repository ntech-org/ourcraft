#pragma once

// Thin Tracy wrapper. Zero-cost when TRACY_ENABLE is not defined.
//
// Build with profiling:
//   xmake f -m debug --tracy=y && xmake
// Then run Tracy GUI and connect to the game (default localhost).
//
// Usage:
//   OC_ZONE_SCOPED;                 // name = current function
//   OC_ZONE_SCOPED_N("MyLabel");    // custom name
//   OC_FRAME_MARK;                  // end of frame
//   OC_FRAME_MARK_NAMED("Server");  // named frame plot
//   OC_PLOT("Queue", value);        // numeric plot
//   OC_MESSAGE("text");             // log message
//   OC_THREAD_NAME("MeshWorker");   // name current thread

#if defined(TRACY_ENABLE)
#  include <tracy/Tracy.hpp>
#  define OC_ZONE_SCOPED              ZoneScoped
#  define OC_ZONE_SCOPED_N(name)      ZoneScopedN(name)
#  define OC_ZONE_NAMED_N(var, name)  ZoneNamedN(var, name, true)
#  define OC_FRAME_MARK               FrameMark
#  define OC_FRAME_MARK_NAMED(name)   FrameMarkNamed(name)
#  define OC_PLOT(name, val)          TracyPlot(name, static_cast<int64_t>(val))
#  define OC_PLOT_F(name, val)        TracyPlot(name, static_cast<double>(val))
#  define OC_MESSAGE(text)            TracyMessageL(text)
#  define OC_THREAD_NAME(name)        tracy::SetThreadName(name)
#  define OC_ALLOC(ptr, size)         TracyAlloc(ptr, size)
#  define OC_FREE(ptr)                TracyFree(ptr)
#else
#  define OC_ZONE_SCOPED              ((void)0)
#  define OC_ZONE_SCOPED_N(name)      ((void)0)
#  define OC_ZONE_NAMED_N(var, name)  ((void)0)
#  define OC_FRAME_MARK               ((void)0)
#  define OC_FRAME_MARK_NAMED(name)   ((void)0)
#  define OC_PLOT(name, val)          ((void)0)
#  define OC_PLOT_F(name, val)        ((void)0)
#  define OC_MESSAGE(text)            ((void)0)
#  define OC_THREAD_NAME(name)        ((void)0)
#  define OC_ALLOC(ptr, size)         ((void)0)
#  define OC_FREE(ptr)                ((void)0)
#endif
