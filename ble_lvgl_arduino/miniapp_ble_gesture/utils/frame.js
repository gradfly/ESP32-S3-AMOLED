// utils/frame.js
// 11路CH数值 → 逗号分隔+分号结尾的字符串帧 → ArrayBuffer
// 对应ESP32端 ble_manager_parse_frame() 所期望的格式

export function encode11chFrame(chArray) {
  if (!Array.isArray(chArray) || chArray.length !== 11) {
    throw new Error('encode11chFrame: 必须传入长度为11的数值数组');
  }
  const parts = chArray.map((v, idx) => {
    // 健壮性：0-9999，整型，补4位前导零
    let n = (v === null || v === undefined) ? 0 : Number(v) | 0;
    if (n < 0) n = 0;
    if (n > 9999) n = 9999;
    return n.toString().padStart(4, '0');
  });
  const str = parts.join(',') + ';';
  // 注意：微信小程序的蓝牙写接口需要 ArrayBuffer
  // TextEncoder 在微信基础库>=2.19支持；若不支持，走兼容路径
  if (typeof TextEncoder !== 'undefined') {
    const enc = new TextEncoder();
    const u8 = enc.encode(str);
    return u8.buffer.slice(u8.byteOffset, u8.byteOffset + u8.byteLength);
  }
  // 兼容：手写ASCII编码（所有字符都是0-9/,;，在ASCII范围内）
  const buf = new ArrayBuffer(str.length);
  const view = new Uint8Array(buf);
  for (let i = 0; i < str.length; i++) {
    view[i] = str.charCodeAt(i) & 0xFF;
  }
  return buf;
}

// 可选：ArrayBuffer → 字符串（用于调试打印发送/接收到的内容）
export function arrayBufferToString(ab) {
  if (!ab) return '';
  if (typeof TextDecoder !== 'undefined') {
    try { return new TextDecoder('utf-8').decode(new Uint8Array(ab)); } catch (e) {}
  }
  // 兼容
  const u8 = new Uint8Array(ab);
  let s = '';
  for (let i = 0; i < u8.length; i++) s += String.fromCharCode(u8[i]);
  return s;
}
