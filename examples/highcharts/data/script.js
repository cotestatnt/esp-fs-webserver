const timezone = new Date().getTimezoneOffset();

let chart = Highcharts.chart('container', {
  time: {
    timezoneOffset: timezone
  },
	chart: {
		zoomType: 'x'
	},
	title: {
		text: 'Total heap size and max free block of RAM'
	},
	subtitle: {
		text: 'click and drag in the plot area to zoom in'
	},
	xAxis: {
		type: 'datetime'
	},
	yAxis: {
		title: {
			text: 'Bytes'
		}
	},
	legend: {
		enabled: false
	},
	plotOptions: {
		area: {
			fillColor: {
				linearGradient: {
					x1: 0,
					y1: 0,
					x2: 0,
					y2: 1
				},
				stops: [
					[0, Highcharts.getOptions().colors[0]],
					[1, Highcharts.color(Highcharts.getOptions().colors[0]).setOpacity(0).get('rgba')]
				]
			},
			marker: {
				radius: 2
			},
			lineWidth: 1,
			states: {
				hover: {
					lineWidth: 1
				}
			},
			threshold: null
		}
	},

	series: [{
			type: 'area',
			name: 'Total heap size',
			data: []
		},
		{
			type: 'area',
			name: 'Max contiguos RAM block',
			data: []
		},
	]
});


function addPoint(timestamp, total, max) {
	chart.series[0].addPoint([timestamp, total]);
	chart.series[1].addPoint([timestamp, max]);
}


// WebSocket handling
function ws_connect() {
  var ws = new WebSocket('ws://'+document.location.host+'/ws',['arduino']);
  ws.onopen = function() { ws.send('Connected - ' + new Date());};
  ws.onmessage = function(e) {
    parseMessage(e.data);
  };
  ws.onclose = function(e) {
      setTimeout(function() {
      ws_connect();
      }, 1000);
  };
  ws.onerror = function(err) {
      ws.close();
  };
}

function parseMessage(msg) {
	try {
		const obj = JSON.parse(msg);
		if (typeof obj === 'object' && obj !== null) {
			// Add new point to chart message
			if (obj.addPoint !== null) {
			  
			  // Updated the ESP timedate
				var date = new Date(obj.timestamp*1000);   
				document.getElementById("esp-time").innerHTML = date;
				
				// Add the new point
				addPoint(obj.timestamp*1000, obj.totalHeap, obj.maxBlock);
			}
		}
	} catch {
		console.log('Error on parse message ' + msg);
	}
}

ws_connect();
