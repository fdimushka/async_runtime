(() => {
  // node_modules/flatbuffers/mjs/constants.js
  var SIZEOF_INT = 4;
  var FILE_IDENTIFIER_LENGTH = 4;
  var SIZE_PREFIX_LENGTH = 4;

  // node_modules/flatbuffers/mjs/utils.js
  var int32 = new Int32Array(2);
  var float32 = new Float32Array(int32.buffer);
  var float64 = new Float64Array(int32.buffer);
  var isLittleEndian = new Uint16Array(new Uint8Array([1, 0]).buffer)[0] === 1;

  // node_modules/flatbuffers/mjs/encoding.js
  var Encoding;
  (function(Encoding2) {
    Encoding2[Encoding2["UTF8_BYTES"] = 1] = "UTF8_BYTES";
    Encoding2[Encoding2["UTF16_STRING"] = 2] = "UTF16_STRING";
  })(Encoding || (Encoding = {}));

  // node_modules/flatbuffers/mjs/byte-buffer.js
  var ByteBuffer = class {
    constructor(bytes_) {
      this.bytes_ = bytes_;
      this.position_ = 0;
    }
    static allocate(byte_size) {
      return new ByteBuffer(new Uint8Array(byte_size));
    }
    clear() {
      this.position_ = 0;
    }
    bytes() {
      return this.bytes_;
    }
    position() {
      return this.position_;
    }
    setPosition(position) {
      this.position_ = position;
    }
    capacity() {
      return this.bytes_.length;
    }
    readInt8(offset) {
      return this.readUint8(offset) << 24 >> 24;
    }
    readUint8(offset) {
      return this.bytes_[offset];
    }
    readInt16(offset) {
      return this.readUint16(offset) << 16 >> 16;
    }
    readUint16(offset) {
      return this.bytes_[offset] | this.bytes_[offset + 1] << 8;
    }
    readInt32(offset) {
      return this.bytes_[offset] | this.bytes_[offset + 1] << 8 | this.bytes_[offset + 2] << 16 | this.bytes_[offset + 3] << 24;
    }
    readUint32(offset) {
      return this.readInt32(offset) >>> 0;
    }
    readInt64(offset) {
      return BigInt.asIntN(64, BigInt(this.readUint32(offset)) + (BigInt(this.readUint32(offset + 4)) << BigInt(32)));
    }
    readUint64(offset) {
      return BigInt.asUintN(64, BigInt(this.readUint32(offset)) + (BigInt(this.readUint32(offset + 4)) << BigInt(32)));
    }
    readFloat32(offset) {
      int32[0] = this.readInt32(offset);
      return float32[0];
    }
    readFloat64(offset) {
      int32[isLittleEndian ? 0 : 1] = this.readInt32(offset);
      int32[isLittleEndian ? 1 : 0] = this.readInt32(offset + 4);
      return float64[0];
    }
    writeInt8(offset, value) {
      this.bytes_[offset] = value;
    }
    writeUint8(offset, value) {
      this.bytes_[offset] = value;
    }
    writeInt16(offset, value) {
      this.bytes_[offset] = value;
      this.bytes_[offset + 1] = value >> 8;
    }
    writeUint16(offset, value) {
      this.bytes_[offset] = value;
      this.bytes_[offset + 1] = value >> 8;
    }
    writeInt32(offset, value) {
      this.bytes_[offset] = value;
      this.bytes_[offset + 1] = value >> 8;
      this.bytes_[offset + 2] = value >> 16;
      this.bytes_[offset + 3] = value >> 24;
    }
    writeUint32(offset, value) {
      this.bytes_[offset] = value;
      this.bytes_[offset + 1] = value >> 8;
      this.bytes_[offset + 2] = value >> 16;
      this.bytes_[offset + 3] = value >> 24;
    }
    writeInt64(offset, value) {
      this.writeInt32(offset, Number(BigInt.asIntN(32, value)));
      this.writeInt32(offset + 4, Number(BigInt.asIntN(32, value >> BigInt(32))));
    }
    writeUint64(offset, value) {
      this.writeUint32(offset, Number(BigInt.asUintN(32, value)));
      this.writeUint32(offset + 4, Number(BigInt.asUintN(32, value >> BigInt(32))));
    }
    writeFloat32(offset, value) {
      float32[0] = value;
      this.writeInt32(offset, int32[0]);
    }
    writeFloat64(offset, value) {
      float64[0] = value;
      this.writeInt32(offset, int32[isLittleEndian ? 0 : 1]);
      this.writeInt32(offset + 4, int32[isLittleEndian ? 1 : 0]);
    }
    getBufferIdentifier() {
      if (this.bytes_.length < this.position_ + SIZEOF_INT + FILE_IDENTIFIER_LENGTH) {
        throw new Error("FlatBuffers: ByteBuffer is too short to contain an identifier.");
      }
      let result = "";
      for (let i = 0; i < FILE_IDENTIFIER_LENGTH; i++) {
        result += String.fromCharCode(this.readInt8(this.position_ + SIZEOF_INT + i));
      }
      return result;
    }
    __offset(bb_pos, vtable_offset) {
      const vtable = bb_pos - this.readInt32(bb_pos);
      return vtable_offset < this.readInt16(vtable) ? this.readInt16(vtable + vtable_offset) : 0;
    }
    __union(t, offset) {
      t.bb_pos = offset + this.readInt32(offset);
      t.bb = this;
      return t;
    }
    __string(offset, opt_encoding) {
      offset += this.readInt32(offset);
      const length = this.readInt32(offset);
      let result = "";
      let i = 0;
      offset += SIZEOF_INT;
      if (opt_encoding === Encoding.UTF8_BYTES) {
        return this.bytes_.subarray(offset, offset + length);
      }
      while (i < length) {
        let codePoint;
        const a = this.readUint8(offset + i++);
        if (a < 192) {
          codePoint = a;
        } else {
          const b = this.readUint8(offset + i++);
          if (a < 224) {
            codePoint = (a & 31) << 6 | b & 63;
          } else {
            const c = this.readUint8(offset + i++);
            if (a < 240) {
              codePoint = (a & 15) << 12 | (b & 63) << 6 | c & 63;
            } else {
              const d = this.readUint8(offset + i++);
              codePoint = (a & 7) << 18 | (b & 63) << 12 | (c & 63) << 6 | d & 63;
            }
          }
        }
        if (codePoint < 65536) {
          result += String.fromCharCode(codePoint);
        } else {
          codePoint -= 65536;
          result += String.fromCharCode((codePoint >> 10) + 55296, (codePoint & (1 << 10) - 1) + 56320);
        }
      }
      return result;
    }
    __union_with_string(o, offset) {
      if (typeof o === "string") {
        return this.__string(offset);
      }
      return this.__union(o, offset);
    }
    __indirect(offset) {
      return offset + this.readInt32(offset);
    }
    __vector(offset) {
      return offset + this.readInt32(offset) + SIZEOF_INT;
    }
    __vector_len(offset) {
      return this.readInt32(offset + this.readInt32(offset));
    }
    __has_identifier(ident) {
      if (ident.length != FILE_IDENTIFIER_LENGTH) {
        throw new Error("FlatBuffers: file identifier must be length " + FILE_IDENTIFIER_LENGTH);
      }
      for (let i = 0; i < FILE_IDENTIFIER_LENGTH; i++) {
        if (ident.charCodeAt(i) != this.readInt8(this.position() + SIZEOF_INT + i)) {
          return false;
        }
      }
      return true;
    }
    createScalarList(listAccessor, listLength) {
      const ret = [];
      for (let i = 0; i < listLength; ++i) {
        if (listAccessor(i) !== null) {
          ret.push(listAccessor(i));
        }
      }
      return ret;
    }
    createObjList(listAccessor, listLength) {
      const ret = [];
      for (let i = 0; i < listLength; ++i) {
        const val = listAccessor(i);
        if (val !== null) {
          ret.push(val.unpack());
        }
      }
      return ret;
    }
  };

  // async-runtime/work-step-schema.ts
  var WorkStepSchema = class {
    constructor() {
      this.bb = null;
      this.bb_pos = 0;
    }
    __init(i, bb) {
      this.bb_pos = i;
      this.bb = bb;
      return this;
    }
    thread() {
      return this.bb.readUint64(this.bb_pos);
    }
    begin() {
      return this.bb.readInt64(this.bb_pos + 8);
    }
    end() {
      return this.bb.readInt64(this.bb_pos + 16);
    }
    static sizeOf() {
      return 24;
    }
    static createWorkStepSchema(builder, thread, begin, end) {
      builder.prep(8, 24);
      builder.writeInt64(end);
      builder.writeInt64(begin);
      builder.writeInt64(thread);
      return builder.offset();
    }
  };

  // async-runtime/work-schema.ts
  var WorkSchema = class {
    constructor() {
      this.bb = null;
      this.bb_pos = 0;
    }
    __init(i, bb) {
      this.bb_pos = i;
      this.bb = bb;
      return this;
    }
    static getRootAsWorkSchema(bb, obj) {
      return (obj || new WorkSchema()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
    }
    static getSizePrefixedRootAsWorkSchema(bb, obj) {
      bb.setPosition(bb.position() + SIZE_PREFIX_LENGTH);
      return (obj || new WorkSchema()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
    }
    id() {
      const offset = this.bb.__offset(this.bb_pos, 4);
      return offset ? this.bb.readUint64(this.bb_pos + offset) : BigInt("0");
    }
    name(optionalEncoding) {
      const offset = this.bb.__offset(this.bb_pos, 6);
      return offset ? this.bb.__string(this.bb_pos + offset, optionalEncoding) : null;
    }
    workTime(index, obj) {
      const offset = this.bb.__offset(this.bb_pos, 8);
      return offset ? (obj || new WorkStepSchema()).__init(this.bb.__vector(this.bb_pos + offset) + index * 24, this.bb) : null;
    }
    workTimeLength() {
      const offset = this.bb.__offset(this.bb_pos, 8);
      return offset ? this.bb.__vector_len(this.bb_pos + offset) : 0;
    }
    beginAt() {
      const offset = this.bb.__offset(this.bb_pos, 10);
      return offset ? this.bb.readInt64(this.bb_pos + offset) : BigInt("0");
    }
    updatedAt() {
      const offset = this.bb.__offset(this.bb_pos, 12);
      return offset ? this.bb.readInt64(this.bb_pos + offset) : BigInt("0");
    }
    static startWorkSchema(builder) {
      builder.startObject(5);
    }
    static addId(builder, id) {
      builder.addFieldInt64(0, id, BigInt("0"));
    }
    static addName(builder, nameOffset) {
      builder.addFieldOffset(1, nameOffset, 0);
    }
    static addWorkTime(builder, workTimeOffset) {
      builder.addFieldOffset(2, workTimeOffset, 0);
    }
    static startWorkTimeVector(builder, numElems) {
      builder.startVector(24, numElems, 8);
    }
    static addBeginAt(builder, beginAt) {
      builder.addFieldInt64(3, beginAt, BigInt("0"));
    }
    static addUpdatedAt(builder, updatedAt) {
      builder.addFieldInt64(4, updatedAt, BigInt("0"));
    }
    static endWorkSchema(builder) {
      const offset = builder.endObject();
      return offset;
    }
    static createWorkSchema(builder, id, nameOffset, workTimeOffset, beginAt, updatedAt) {
      WorkSchema.startWorkSchema(builder);
      WorkSchema.addId(builder, id);
      WorkSchema.addName(builder, nameOffset);
      WorkSchema.addWorkTime(builder, workTimeOffset);
      WorkSchema.addBeginAt(builder, beginAt);
      WorkSchema.addUpdatedAt(builder, updatedAt);
      return WorkSchema.endWorkSchema(builder);
    }
  };

  // async-runtime/state-schema.ts
  var StateSchema = class {
    constructor() {
      this.bb = null;
      this.bb_pos = 0;
    }
    __init(i, bb) {
      this.bb_pos = i;
      this.bb = bb;
      return this;
    }
    static getRootAsStateSchema(bb, obj) {
      return (obj || new StateSchema()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
    }
    static getSizePrefixedRootAsStateSchema(bb, obj) {
      bb.setPosition(bb.position() + SIZE_PREFIX_LENGTH);
      return (obj || new StateSchema()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
    }
    systemInfo(optionalEncoding) {
      const offset = this.bb.__offset(this.bb_pos, 4);
      return offset ? this.bb.__string(this.bb_pos + offset, optionalEncoding) : null;
    }
    createdAt() {
      const offset = this.bb.__offset(this.bb_pos, 6);
      return offset ? this.bb.readInt64(this.bb_pos + offset) : BigInt("0");
    }
    coroutinesCount() {
      const offset = this.bb.__offset(this.bb_pos, 8);
      return offset ? this.bb.readInt64(this.bb_pos + offset) : BigInt("0");
    }
    workGround(index, obj) {
      const offset = this.bb.__offset(this.bb_pos, 10);
      return offset ? (obj || new WorkSchema()).__init(this.bb.__indirect(this.bb.__vector(this.bb_pos + offset) + index * 4), this.bb) : null;
    }
    workGroundLength() {
      const offset = this.bb.__offset(this.bb_pos, 10);
      return offset ? this.bb.__vector_len(this.bb_pos + offset) : 0;
    }
    threads(index) {
      const offset = this.bb.__offset(this.bb_pos, 12);
      return offset ? this.bb.readUint64(this.bb.__vector(this.bb_pos + offset) + index * 8) : BigInt(0);
    }
    threadsLength() {
      const offset = this.bb.__offset(this.bb_pos, 12);
      return offset ? this.bb.__vector_len(this.bb_pos + offset) : 0;
    }
    static startStateSchema(builder) {
      builder.startObject(5);
    }
    static addSystemInfo(builder, systemInfoOffset) {
      builder.addFieldOffset(0, systemInfoOffset, 0);
    }
    static addCreatedAt(builder, createdAt) {
      builder.addFieldInt64(1, createdAt, BigInt("0"));
    }
    static addCoroutinesCount(builder, coroutinesCount) {
      builder.addFieldInt64(2, coroutinesCount, BigInt("0"));
    }
    static addWorkGround(builder, workGroundOffset) {
      builder.addFieldOffset(3, workGroundOffset, 0);
    }
    static createWorkGroundVector(builder, data) {
      builder.startVector(4, data.length, 4);
      for (let i = data.length - 1; i >= 0; i--) {
        builder.addOffset(data[i]);
      }
      return builder.endVector();
    }
    static startWorkGroundVector(builder, numElems) {
      builder.startVector(4, numElems, 4);
    }
    static addThreads(builder, threadsOffset) {
      builder.addFieldOffset(4, threadsOffset, 0);
    }
    static createThreadsVector(builder, data) {
      builder.startVector(8, data.length, 8);
      for (let i = data.length - 1; i >= 0; i--) {
        builder.addInt64(data[i]);
      }
      return builder.endVector();
    }
    static startThreadsVector(builder, numElems) {
      builder.startVector(8, numElems, 8);
    }
    static endStateSchema(builder) {
      const offset = builder.endObject();
      return offset;
    }
    static createStateSchema(builder, systemInfoOffset, createdAt, coroutinesCount, workGroundOffset, threadsOffset) {
      StateSchema.startStateSchema(builder);
      StateSchema.addSystemInfo(builder, systemInfoOffset);
      StateSchema.addCreatedAt(builder, createdAt);
      StateSchema.addCoroutinesCount(builder, coroutinesCount);
      StateSchema.addWorkGround(builder, workGroundOffset);
      StateSchema.addThreads(builder, threadsOffset);
      return StateSchema.endStateSchema(builder);
    }
  };

  // profiler.ts
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
    return Math.round(Number(ts_ns) / 1e3);
  }
  function pad_to_2digits(num) {
    return num.toString().padStart(2, "0");
  }
  function format_timestamp(ts) {
    let date = new Date(ts);
    return [
      pad_to_2digits(date.getHours()),
      pad_to_2digits(date.getMinutes()),
      pad_to_2digits(date.getSeconds()),
      pad_to_2digits(date.getMilliseconds())
    ].join(":");
  }
  function string_to_colour(str) {
    var hash = 0;
    for (var i = 0; i < str.length; i++) {
      hash = str.charCodeAt(i) + ((hash << 5) - hash);
    }
    var colour = "#";
    for (var i = 0; i < 3; i++) {
      var value = hash >> i * 8 & 255;
      colour += ("00" + value.toString(16)).substr(-2);
    }
    return colour;
  }
  function compute_profiler_state(data) {
    let buf = new ByteBuffer(base64_to_array(data));
    let state = StateSchema.getRootAsStateSchema(buf);
    let threads = {};
    let strip_lines = [];
    let data_point = [];
    let start_time = Date.now();
    document.getElementById("system-info").innerText = state.systemInfo();
    document.getElementById("coro_stat_count").innerText = state.coroutinesCount().toString();
    for (let i = 0; i < state.threadsLength(); ++i) {
      threads[Number(state.threads(i))] = i + 1;
      let tr = document.createElement("tr");
      let td0 = document.createElement("td");
      let td1 = document.createElement("td");
      td0.innerText = (i + 1).toString();
      td1.innerText = Number(state.threads(i)).toString();
      tr.appendChild(td0);
      tr.appendChild(td1);
      let table = document.getElementById("threads");
      table.appendChild(tr);
      strip_lines.push({
        value: i + 1,
        color: "#d8d8d8"
      });
    }
    for (let i = 0; i < state.workGroundLength(); ++i) {
      let work = state.workGround(i);
      var color = string_to_colour(work.name());
      var avg_work_time = 0;
      var max_work_time = 0;
      for (let t = 0; t < work.workTimeLength(); ++t) {
        let work_time = work.workTime(t);
        start_time = Math.min(start_time, to_ms(work.workTime(0).begin()));
        if (work_time.end() > 0) {
          let thread_id = Number(work_time.thread());
          let thread = threads[thread_id];
          const time = to_ms(work_time.end() - work_time.begin());
          avg_work_time += time;
          max_work_time = Math.max(max_work_time, time);
          data_point.push({
            x: thread,
            y: [to_ms(work_time.begin()), to_ms(work_time.end())],
            name: work.name(),
            work_time: time,
            thread: thread_id,
            color
          });
        }
      }
      if (work.workTimeLength() > 0)
        avg_work_time /= work.workTimeLength();
      let tr = document.createElement("tr");
      let td0 = document.createElement("td");
      let td1 = document.createElement("td");
      let td2 = document.createElement("td");
      let td3 = document.createElement("td");
      let td4 = document.createElement("td");
      td0.setAttribute("style", "background-color: " + color);
      td1.innerText = work.name();
      td2.innerText = work.workTimeLength().toString();
      td3.innerText = max_work_time.toFixed(1).toString();
      td4.innerText = avg_work_time.toFixed(1).toString();
      tr.appendChild(td0);
      tr.appendChild(td1);
      tr.appendChild(td2);
      tr.appendChild(td3);
      tr.appendChild(td4);
      let table = document.getElementById("call_stat");
      table.appendChild(tr);
    }
    var options = {
      animationEnabled: false,
      exportEnabled: true,
      zoomEnabled: true,
      zoomType: "y",
      axisX: {
        labelFormatter: function(e) {
          if (e.value > 0 && e.value <= strip_lines.length)
            return e.value;
          else
            return "";
        },
        minimum: 0,
        maximum: strip_lines.length + 1,
        title: "Threads",
        interval: 1,
        stripLines: strip_lines
      },
      axisY: {
        minimum: start_time,
        interval: 6e4,
        crosshair: {
          enabled: true,
          labelFormatter: function(e) {
            return format_timestamp(e.value);
          }
        },
        labelFormatter: function(e) {
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
  fetch(`http://127.0.0.1:9002/profiler/state`).then((response) => {
    return response.text();
  }).then((data) => {
    compute_profiler_state(data);
  });
})();
