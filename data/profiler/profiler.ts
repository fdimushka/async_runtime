import { StateSchema } from './async-runtime/state-schema';
//import CanvasJS from 'canvasjs';
import * as flatbuffers from 'flatbuffers';


function base64_to_array(base64) {
    let binary_string = window.atob(base64);
    let len = binary_string.length;
    let bytes = new Uint8Array(len);
    for (let i = 0; i < len; i++) {
        bytes[i] = binary_string.charCodeAt(i);
    }
    return bytes;
}


function to_ms(ts_ns) {
    return Math.round(Number(ts_ns)/1000.0)
}

function pad_to_2digits(num) {
    return num.toString().padStart(2, '0');
}

function format_timestamp(ts) {
    let date = new Date(ts);
    return (
        [
            pad_to_2digits(date.getHours()),
            pad_to_2digits(date.getMinutes()),
            pad_to_2digits(date.getSeconds()),
            pad_to_2digits(date.getMilliseconds()),
        ].join(':')
    );
}

function string_to_colour(str) {
    var hash = 0;
    for (var i = 0; i < str.length; i++) {
        hash = str.charCodeAt(i) + ((hash << 5) - hash);
    }
    var colour = '#';
    for (var i = 0; i < 3; i++) {
        var value = (hash >> (i * 8)) & 0xFF;
        colour += ('00' + value.toString(16)).substr(-2);
    }
    return colour;
}

function compute_profiler_state(data) {
    let buf = new flatbuffers.ByteBuffer(base64_to_array(data));
    let state = StateSchema.getRootAsStateSchema(buf);
    let threads = {}
    let strip_lines = [];
    let data_point = [];
    let start_time = Date.now();

    document.getElementById('system-info').innerText = state.systemInfo();
    document.getElementById('coro_stat_count').innerText = state.coroutinesCount().toString();

    for (let i = 0; i < state.threadsLength(); ++i) {
        threads[Number(state.threads(i))] = i+1

        let tr = document.createElement('tr');
        let td0 = document.createElement('td');
        let td1 = document.createElement('td');

        td0.innerText = (i+1).toString()
        td1.innerText = Number(state.threads(i)).toString();

        tr.appendChild(td0);
        tr.appendChild(td1);
        let table = document.getElementById('threads')
        table.appendChild(tr);

        strip_lines.push(
            {
                value:i+1,
                color:"#d8d8d8"
            }
        )
    }

    for(let i = 0; i < state.workGroundLength(); ++i) {
        let work = state.workGround(i)
        var color = string_to_colour(work.name());
        var avg_work_time = 0
        var max_work_time = 0

        for(let t = 0; t < work.workTimeLength(); ++t) {
            let work_time = work.workTime(t);
            start_time = Math.min(start_time, to_ms(work.workTime(0).begin()));

            if(work_time.end() > 0) {
                let thread_id = Number(work_time.thread())
                let thread = threads[thread_id];
                const time = to_ms(work_time.end() - work_time.begin());
                avg_work_time += time;
                max_work_time = Math.max(max_work_time, time)

                data_point.push({
                    x: thread,
                    y: [to_ms(work_time.begin()), to_ms(work_time.end())],
                    name: work.name(),
                    work_time: time,
                    thread: thread_id,
                    color: color
                })
            }
        }

        if(work.workTimeLength() > 0)
            avg_work_time /= work.workTimeLength();

        let tr = document.createElement('tr');
        let td0 = document.createElement('td');
        let td1 = document.createElement('td');
        let td2 = document.createElement('td');
        let td3 = document.createElement('td');
        let td4 = document.createElement('td');

        td0.setAttribute('style', 'background-color: ' + color);
        td1.innerText = work.name();
        td2.innerText = work.workTimeLength().toString();
        td3.innerText = max_work_time.toFixed(1).toString();
        td4.innerText = avg_work_time.toFixed(1).toString();

        tr.appendChild(td0);
        tr.appendChild(td1);
        tr.appendChild(td2);
        tr.appendChild(td3);
        tr.appendChild(td4);
        let table = document.getElementById('call_stat')
        table.appendChild(tr);
    }

    var options = {
      animationEnabled: false,
      exportEnabled: true,
      zoomEnabled: true,
      zoomType: "y",
      axisX: {
          labelFormatter: function ( e ) {
              if(e.value > 0 && e.value <= strip_lines.length)
                return e.value;
              else
                return ''
          },
          minimum: 0,
        maximum: strip_lines.length + 1,
        title: "Threads",
        interval: 1,
        stripLines: strip_lines
      },
      axisY: {
        minimum: start_time,
        interval: 60000,
        crosshair: {
          enabled: true,
          labelFormatter: function ( e ) {
            return format_timestamp(e.value);
          }
        },
        labelFormatter: function ( e ) {
            return format_timestamp(e.value);
        }
      },
      dataPointWidth: 10,
      data: [{
        type: "rangeBar",
        toolTipContent: "name: {name}, thread: {thread}, time: {work_time}ms",
        dataPoints: data_point
      }]
    };

    var chart = new CanvasJS.Chart("chart", options);
    chart.render();
}


fetch(`http://127.0.0.1:9002/profiler/state`)
    .then(response => {
        return response.text();
    })
    .then(data => {
        compute_profiler_state(data)
    });