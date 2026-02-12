# WebSocket Chart Example

Real-time telemetry dashboard demonstrating WebSocket communication with live charting.

The ESP broadcasts memory stats (heap, max block) and WiFi RSSI every second. The web interface displays data in animated charts with theme selection and auto-reconnect.

## Charting Library

This example uses **[ECharts 5](https://echarts.apache.org/)** – a powerful, declarative JavaScript visualization library by Apache.

- **Lightweight**: Loaded via CDN (~900KB minified)
- **Responsive**: Auto-resizes on window changes
- **Smooth animations**: Built-in transitions for live data
- **Rich API**: [Full documentation](https://echarts.apache.org/en/option.html)

The dashboard implements real-time `line` charts with `time` axis and formatted tooltips. All chart options are in `data/index.htm`.
