// ImPlot (https://github.com/epezent/implot, MIT license, Copyright (c) 2020 Evan Pezent) as a
// few C functions for the Fels side: each item takes its colour and weight as plain arguments
// instead of an ImPlotSpec, a struct with defaults Fels would have to copy. Built into
// cimgui.dll with Dear ImGui (see build.sh); fels_app.cpp makes the ImPlot context.

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "implot.h"
#include "implot_internal.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define FELS_API extern "C" __declspec(dllexport)

// A colour whose alpha is below zero is ImPlot's "next colour of the colormap".
static ImPlotSpec spec_of(ImVec4 color, float weight, float fill_alpha, int flags) {
    ImPlotSpec s;
    if (color.w >= 0.0f) {
        s.LineColor = color;
        s.FillColor = color;
    }
    if (weight > 0.0f) s.LineWeight = weight;
    if (fill_alpha >= 0.0f) s.FillAlpha = fill_alpha;
    s.Flags = flags;
    return s;
}

FELS_API int fels_plot_begin(const char* title, float width, float height, int flags) {
    return ImPlot::BeginPlot(title, ImVec2(width, height), flags) ? 1 : 0;
}

FELS_API void fels_plot_end(void) { ImPlot::EndPlot(); }

FELS_API int fels_plot_subplots_begin(const char* title, int rows, int cols, float width, float height, int flags) {
    return ImPlot::BeginSubplots(title, rows, cols, ImVec2(width, height), flags) ? 1 : 0;
}

FELS_API void fels_plot_subplots_end(void) { ImPlot::EndSubplots(); }

FELS_API void fels_plot_axes(const char* x_label, const char* y_label, int x_flags, int y_flags) {
    ImPlot::SetupAxes(x_label, y_label, x_flags, y_flags);
}

FELS_API void fels_plot_axis(int axis, const char* label, int flags) { ImPlot::SetupAxis(axis, label, flags); }

// The axis counts seconds since 1970 and shows dates.
FELS_API void fels_plot_axis_time(int axis) { ImPlot::SetupAxisScale(axis, ImPlotScale_Time); }

// Days since 1970 from a date and back (Howard Hinnant's algorithms).
static long long days_from(long long y, int m, int d) {
    y -= m <= 2;
    long long era = (y >= 0 ? y : y - 399) / 400;
    long long yoe = y - era * 400;
    long long doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    long long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

static void date_of(long long days, long long* y, int* m, int* d) {
    long long z = days + 719468;
    long long era = (z >= 0 ? z : z - 146096) / 146097;
    long long doe = z - era * 146097;
    long long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    long long mp = (5 * doy + 2) / 153;
    *d = (int)(doy - (153 * mp + 2) / 5 + 1);
    *m = (int)(mp < 10 ? mp + 3 : mp - 9);
    *y = yoe + era * 400 + (*m <= 2);
}

static const char* MONTHS[12] = { "Jan", "Feb", "M\xc3\xa4r", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez" };

// Ticks of a date axis at whole days, months or years, as many as there is room for (a label
// every 70 pixels), written the German way: "15.01.", "Okt 25" or "2025".
static void german_dates(ImPlotTicker& ticker, const ImPlotRange& range, float pixels, bool, ImPlotFormatter, void*) {
    double lo = range.Min, hi = range.Max;
    if (!(hi > lo)) return;
    int room = (int)(pixels / 70.0f);
    if (room < 2) room = 2;
    double span = (hi - lo) / 86400.0;
    static const int day_steps[] = { 1, 2, 7, 14 };
    static const int month_steps[] = { 1, 2, 3, 6 };
    static const int year_steps[] = { 1, 2, 5, 10, 20, 50, 100 };
    char label[32];
    long long first = (long long)floor(lo / 86400.0);
    long long last = (long long)ceil(hi / 86400.0);
    for (int step : day_steps) {
        if (span / step > room) continue;
        for (long long day = (first / step) * step; day <= last; day += step) {
            double at = day * 86400.0;
            if (at < lo || at > hi) continue;
            long long y; int m, d;
            date_of(day, &y, &m, &d);
            snprintf(label, sizeof label, "%02d.%02d.", d, m);
            ticker.AddTick(at, true, 0, true, label);
        }
        return;
    }
    long long y0; int m0, d0;
    date_of(first, &y0, &m0, &d0);
    for (int step : month_steps) {
        if (span / (30.44 * step) > room) continue;
        long long y = y0;
        int m = m0;
        while ((m - 1) % step != 0) { if (++m > 12) { m = 1; y++; } }
        for (;;) {
            double at = days_from(y, m, 1) * 86400.0;
            if (at > hi) break;
            if (at >= lo) {
                snprintf(label, sizeof label, "%s %02lld", MONTHS[m - 1], ((y % 100) + 100) % 100);
                ticker.AddTick(at, true, 0, true, label);
            }
            m += step;
            while (m > 12) { m -= 12; y++; }
        }
        return;
    }
    for (int step : year_steps) {
        if (span / (365.25 * step) > room && step != 100) continue;
        long long y = y0 - (((y0 % step) + step) % step);
        for (;; y += step) {
            double at = days_from(y, 1, 1) * 86400.0;
            if (at > hi) break;
            if (at >= lo) {
                snprintf(label, sizeof label, "%lld", y);
                ticker.AddTick(at, true, 0, true, label);
            }
        }
        return;
    }
}

// The axis counts seconds since 1970 and shows dates the German way.
FELS_API void fels_plot_axis_dates(int axis) {
    ImPlot::SetupAxisScale(axis, ImPlotScale_Time);
    ImPlotAxis& a = ImPlot::GetCurrentPlot()->Axes[axis];
    a.Locator = german_dates;
    a.Ticker.Levels = 1;
}

// A step of 1, 2, 2.5 or 5 times a power of ten, at least `raw`.
static double nice_step(double raw) {
    double e = pow(10.0, floor(log10(raw)));
    double f = raw / e;
    double n = f <= 1.0 ? 1.0 : f <= 2.0 ? 2.0 : f <= 2.5 ? 2.5 : f <= 5.0 ? 5.0 : 10.0;
    return n * e;
}

// The digits after the comma that the multiples of `step` need (at most 6).
static int decimals_for(double step) {
    for (int d = 0; d < 6; d++) {
        double scaled = step * pow(10.0, d);
        if (fabs(scaled - round(scaled)) < 1e-6 * (scaled > 1.0 ? scaled : 1.0)) return d;
    }
    return 6;
}

// `v` the German way, with `decimals` digits after a comma, points between the thousands and
// `suffix` after it: "40.000 €", "-0,25 %". No minus before a zero.
static void german_number(double v, int decimals, const char* suffix, char* out, int size) {
    char raw[64];
    snprintf(raw, sizeof raw, "%.*f", decimals, v);
    const char* p = raw;
    bool negative = *p == '-';
    if (negative) p++;
    bool zero = true;
    for (const char* q = p; *q; q++) if (*q >= '1' && *q <= '9') zero = false;
    const char* point = strchr(p, '.');
    int whole = point ? (int)(point - p) : (int)strlen(p);
    char text[96];
    int n = 0;
    if (negative && !zero) text[n++] = '-';
    for (int i = 0; i < whole && n < 80; i++) {
        if (i > 0 && (whole - i) % 3 == 0) text[n++] = '.';
        text[n++] = p[i];
    }
    if (point) {
        text[n++] = ',';
        for (const char* q = point + 1; *q && n < 94; q++) text[n++] = *q;
    }
    text[n] = 0;
    snprintf(out, size, "%s%s", text, suffix);
}

// Ticks of a number axis at a round step, a label every 34 pixels up and down or every 100
// across, all with the digits the step needs; `data` is the suffix of each label.
static void german_numbers(ImPlotTicker& ticker, const ImPlotRange& range, float pixels, bool vertical, ImPlotFormatter, void* data) {
    double lo = range.Min, hi = range.Max;
    if (!(hi > lo)) return;
    const char* suffix = data ? (const char*)data : "";
    int room = (int)(pixels / (vertical ? 34.0f : 100.0f));
    if (room < 2) room = 2;
    double step = nice_step((hi - lo) / room);
    int decimals = decimals_for(step);
    char label[64];
    for (double k = ceil(lo / step - 1e-9); k * step <= hi + step * 1e-9; k += 1.0) {
        double at = k * step;
        if (fabs(at) < step * 1e-9) at = 0.0;
        german_number(at, decimals, suffix, label, sizeof label);
        ticker.AddTick(at, true, 0, true, label);
    }
}

// The value under the mouse, the German way with two digits.
static int german_value(double value, char* buff, int size, void* data) {
    german_number(value, 2, data ? (const char*)data : "", buff, size);
    return (int)strlen(buff);
}

static char g_suffix[ImAxis_COUNT][16];

// The axis writes its numbers the German way, `suffix` after each (" %", " €" or "").
FELS_API void fels_plot_axis_german(int axis, const char* suffix) {
    snprintf(g_suffix[axis], sizeof g_suffix[axis], "%s", suffix);
    ImPlotAxis& a = ImPlot::GetCurrentPlot()->Axes[axis];
    a.Locator = german_numbers;
    a.Formatter = german_value;
    a.FormatterData = g_suffix[axis];
}

FELS_API void fels_plot_axis_log(int axis) { ImPlot::SetupAxisScale(axis, ImPlotScale_Log10); }

FELS_API void fels_plot_axis_format(int axis, const char* fmt) { ImPlot::SetupAxisFormat(axis, fmt); }

FELS_API void fels_plot_axis_limits(int axis, double lo, double hi, int cond) { ImPlot::SetupAxisLimits(axis, lo, hi, cond); }

FELS_API void fels_plot_axis_ticks(int axis, const double* values, int count, const char* const* labels, int keep_default) {
    ImPlot::SetupAxisTicks(axis, values, count, labels, keep_default != 0);
}

FELS_API void fels_plot_legend(int location, int flags) { ImPlot::SetupLegend(location, flags); }

FELS_API void fels_plot_fit_next(void) { ImPlot::SetNextAxesToFit(); }

FELS_API void fels_plot_line(const char* label, const double* xs, const double* ys, int count, ImVec4 color, float weight, int flags) {
    ImPlot::PlotLine(label, xs, ys, count, spec_of(color, weight, -1.0f, flags));
}

// ImPlot multiplies the alpha of an area's fill by FillAlpha twice (when it stages the item and
// when it draws it): a colour of our own carries the alpha itself, an automatic one gets its
// square root.
static ImPlotSpec fill_spec(ImVec4 color, float alpha, int flags) {
    ImPlotSpec s = spec_of(color, -1.0f, -1.0f, flags);
    if (alpha < 0.0f) alpha = 1.0f;
    if (color.w >= 0.0f) {
        s.FillColor.w = alpha;
    } else {
        s.FillAlpha = sqrtf(alpha);
    }
    return s;
}

FELS_API void fels_plot_shaded(const char* label, const double* xs, const double* ys, int count, double y_ref, ImVec4 color, float alpha, int flags) {
    ImPlot::PlotShaded(label, xs, ys, count, y_ref, fill_spec(color, alpha, flags));
}

FELS_API void fels_plot_shaded_between(const char* label, const double* xs, const double* ys1, const double* ys2, int count, ImVec4 color, float alpha, int flags) {
    ImPlot::PlotShaded(label, xs, ys1, ys2, count, fill_spec(color, alpha, flags));
}

FELS_API void fels_plot_stairs(const char* label, const double* xs, const double* ys, int count, ImVec4 color, float weight, int flags) {
    ImPlot::PlotStairs(label, xs, ys, count, spec_of(color, weight, -1.0f, flags));
}

FELS_API void fels_plot_bars(const char* label, const double* xs, const double* ys, int count, double width, ImVec4 color, int flags) {
    ImPlot::PlotBars(label, xs, ys, count, width, spec_of(color, -1.0f, -1.0f, flags));
}

// `values` holds `items` rows of `groups` values.
FELS_API void fels_plot_bar_groups(const char* const* labels, const double* values, int items, int groups, double size, double shift, int flags) {
    ImPlotSpec s;
    s.Flags = flags;
    ImPlot::PlotBarGroups(labels, values, items, groups, size, shift, s);
}

FELS_API void fels_plot_scatter(const char* label, const double* xs, const double* ys, int count, ImVec4 color, int marker, float size, int flags) {
    ImPlotSpec s = spec_of(color, -1.0f, -1.0f, flags);
    s.Marker = marker;
    if (size > 0.0f) s.MarkerSize = size;
    if (color.w >= 0.0f) {
        s.MarkerFillColor = color;
        s.MarkerLineColor = color;
    }
    ImPlot::PlotScatter(label, xs, ys, count, s);
}

FELS_API void fels_plot_inf_lines(const char* label, const double* values, int count, ImVec4 color, float weight, int flags) {
    ImPlot::PlotInfLines(label, values, count, spec_of(color, weight, -1.0f, flags));
}

FELS_API void fels_plot_pie(const char* const* labels, const double* values, int count, double x, double y, double radius, const char* fmt, double angle0, int flags) {
    ImPlotSpec s;
    s.Flags = flags;
    ImPlot::PlotPieChart(labels, values, count, x, y, radius, fmt, angle0, s);
}

static char g_pie_suffix[16];

// A slice's value with one digit after the comma and the suffix of the pie.
static int german_slice(double value, char* buff, int size, void* data) {
    german_number(value, 1, data ? (const char*)data : "", buff, size);
    return (int)strlen(buff);
}

// As fels_plot_pie, each slice's value written the German way with one digit after the comma
// and `suffix`: "46,6 %".
FELS_API void fels_plot_pie_german(const char* const* labels, const double* values, int count, double x, double y, double radius, const char* suffix, double angle0, int flags) {
    snprintf(g_pie_suffix, sizeof g_pie_suffix, "%s", suffix);
    ImPlotSpec s;
    s.Flags = flags;
    ImPlot::PlotPieChart(labels, values, count, x, y, radius, german_slice, g_pie_suffix, angle0, s);
}

FELS_API void fels_plot_heatmap(const char* label, const double* values, int rows, int cols, double lo, double hi, const char* fmt,
                                double x0, double y0, double x1, double y1, int flags) {
    ImPlotSpec s;
    s.Flags = flags;
    ImPlot::PlotHeatmap(label, values, rows, cols, lo, hi, fmt, ImPlotPoint(x0, y0), ImPlotPoint(x1, y1), s);
}

FELS_API void fels_plot_text(const char* text, double x, double y, float off_x, float off_y, ImVec4 color) {
    ImPlotSpec s;
    if (color.w >= 0.0f) s.LineColor = color;
    ImPlot::PlotText(text, x, y, ImVec2(off_x, off_y), s);
}

FELS_API void fels_plot_tag_x(double x, ImVec4 color, const char* text) { ImPlot::TagX(x, color, "%s", text); }

FELS_API void fels_plot_tag_y(double y, ImVec4 color, const char* text) { ImPlot::TagY(y, color, "%s", text); }

FELS_API int fels_plot_hovered(void) { return ImPlot::IsPlotHovered() ? 1 : 0; }

FELS_API void fels_plot_mouse(double* x, double* y) {
    ImPlotPoint p = ImPlot::GetPlotMousePos();
    *x = p.x;
    *y = p.y;
}

// ImPlot's colours from those of Dear ImGui's style, `colors` (when `count` > 0) as the
// colours of the items, and dates the ISO way when `iso`.
FELS_API void fels_plot_style(const ImVec4* colors, int count, int iso) {
    static int made = 0;
    ImPlot::StyleColorsAuto();
    ImPlotStyle& style = ImPlot::GetStyle();
    style.UseISO8601 = iso != 0;
    style.Use24HourClock = true;
    if (count > 0) {
        // colormaps cannot be taken away, so a new one each time (the theme changes seldom)
        char name[32];
        snprintf(name, sizeof name, "fels%d", made++);
        style.Colormap = ImPlot::AddColormap(name, colors, count, true);
    }
}

// A colormap of `colors` (qualitative: one colour after the other; else a scale from the first
// to the last), to push; its index. Colormaps cannot be taken away.
FELS_API int fels_plot_add_colormap(const ImVec4* colors, int count, int qualitative) {
    static int made = 0;
    char name[32];
    snprintf(name, sizeof name, "fels-map%d", made++);
    return ImPlot::AddColormap(name, colors, count, qualitative != 0);
}

FELS_API void fels_plot_push_colormap(int cmap) { ImPlot::PushColormap(cmap); }

FELS_API void fels_plot_pop_colormap(void) { ImPlot::PopColormap(1); }

// A bar with the colours of `cmap` from `lo` to `hi`, as the legend of a heatmap.
FELS_API void fels_plot_colormap_scale(const char* label, double lo, double hi, float width, float height, const char* fmt, int cmap) {
    ImPlot::ColormapScale(label, lo, hi, ImVec2(width, height), fmt, 0, cmap);
}

static char g_scale_suffix[16];

// As fels_plot_colormap_scale, the numbers written the German way with `suffix` after each
// (ImPlot's ColormapScale with german_numbers for its ticks).
FELS_API void fels_plot_colormap_scale_german(const char* label, double lo, double hi, float width, float height, const char* suffix, int cmap) {
    ImGuiContext& G = *GImGui;
    ImGuiWindow* window = G.CurrentWindow;
    if (window->SkipItems) return;
    snprintf(g_scale_suffix, sizeof g_scale_suffix, "%s", suffix);
    const ImGuiID id = window->GetID(label);
    ImPlotContext& gp = *GImPlot;
    if (cmap < 0) cmap = gp.Style.Colormap;
    ImVec2 frame_size = ImGui::CalcItemSize(ImVec2(width, height), 0, gp.Style.PlotDefaultSize.y);
    ImPlotRange range(ImMin(lo, hi), ImMax(lo, hi));
    gp.CTicker.Reset();
    german_numbers(gp.CTicker, range, frame_size.y, true, nullptr, g_scale_suffix);
    const float txt_off = gp.Style.LabelPadding.x;
    const float pad = txt_off + gp.CTicker.MaxSize.x;
    float bar_w = 20;
    if (frame_size.x == 0) {
        frame_size.x = bar_w + pad + 2 * gp.Style.PlotPadding.x;
    } else {
        bar_w = frame_size.x - (pad + 2 * gp.Style.PlotPadding.x);
        if (bar_w < gp.Style.MajorTickLen.y) bar_w = gp.Style.MajorTickLen.y;
    }
    ImDrawList& draw = *window->DrawList;
    ImRect bb_frame(window->DC.CursorPos, window->DC.CursorPos + frame_size);
    ImGui::ItemSize(bb_frame);
    if (!ImGui::ItemAdd(bb_frame, id, &bb_frame)) return;
    ImGui::RenderFrame(bb_frame.Min, bb_frame.Max, ImPlot::GetStyleColorU32(ImPlotCol_FrameBg), true, G.Style.FrameRounding);
    ImRect bb_grad(bb_frame.Min + gp.Style.PlotPadding,
                   bb_frame.Min + ImVec2(bar_w + gp.Style.PlotPadding.x, frame_size.y - gp.Style.PlotPadding.y));
    ImGui::PushClipRect(bb_frame.Min, bb_frame.Max, true);
    const ImU32 text = ImGui::GetColorU32(ImGuiCol_Text);
    const bool reversed = lo > hi;
    const float y_min = reversed ? bb_grad.Max.y : bb_grad.Min.y;
    const float y_max = reversed ? bb_grad.Min.y : bb_grad.Max.y;
    ImPlot::RenderColorBar(gp.ColormapData.GetKeys(cmap), gp.ColormapData.GetKeyCount(cmap), draw, bb_grad, true, true, !gp.ColormapData.IsQual(cmap));
    for (int i = 0; i < gp.CTicker.TickCount(); ++i) {
        const double at = gp.CTicker.Ticks[i].PlotPos;
        const float y = ImRemap((float)at, (float)range.Max, (float)range.Min, y_min, y_max);
        const float t = (float)((at - lo) / (hi - lo));
        const ImU32 tick = ImPlot::CalcTextColor(gp.ColormapData.LerpTable(cmap, t));
        if (y < bb_grad.Max.y - 2 && y > bb_grad.Min.y + 2)
            draw.AddLine(ImVec2(bb_grad.Max.x - 1, y), ImVec2(bb_grad.Max.x - gp.Style.MajorTickLen.y, y), tick, gp.Style.MajorTickSize.y);
        draw.AddText(ImVec2(bb_grad.Max.x + txt_off, y - gp.CTicker.Ticks[i].LabelSize.y * 0.5f), text, gp.CTicker.GetText(i));
    }
    draw.AddRect(bb_grad.Min, bb_grad.Max, ImPlot::GetStyleColorU32(ImPlotCol_PlotBorder));
    ImGui::PopClipRect();
}

FELS_API void fels_plot_demo(bool* open) { ImPlot::ShowDemoWindow(open); }
