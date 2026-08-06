// utils/ble.js
// 微信小程序蓝牙API Promise封装（小白友好：所有方法async/await直接调用）
// UUID约定：Service=FFE0, Notify=FFE2, Write=FFE1

const SERVICE_UUID_16 = 'FFE0';
const NOTIFY_UUID_16  = 'FFE2';
const WRITE_UUID_16   = 'FFE1';

function to128(uuid16) {
  return `0000${uuid16.toUpperCase()}-0000-1000-8000-00805F9B34FB`;
}

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

// ===== 全局连接状态（存于模块内，页面间共享） =====
let adapterInit = false;
let deviceId = null;
let serviceId = null;      // 实际匹配到的FFE0服务ID（完整128位或16位，按API返回）
let notifyCharId = null;   // FFE2
let writeCharId = null;    // FFE1
const NOTIFY_MTU_PAYLOAD = 20; // 单包最多写20字节（BLE限制）

// 1. 初始化蓝牙适配器 + 授权
export async function initBluetooth() {
  if (adapterInit) return true;
  // 1.1 打开适配器
  await new Promise((resolve, reject) => {
    wx.openBluetoothAdapter({
      success: resolve,
      fail: (err) => {
        console.error('[BLE] openBluetoothAdapter fail:', err);
        // iOS未开蓝牙时常见errCode=10001
        reject(err);
      }
    });
  });
  // 1.2 安卓必须：获取位置权限
  try {
    const sys = wx.getSystemInfoSync();
    if (sys.platform === 'android') {
      await new Promise((resolve) => {
        wx.authorize({
          scope: 'scope.userLocation',
          success: resolve,
          fail: () => {
            // 拒绝也不终止，让用户在系统设置里手动开
            console.warn('[BLE] 位置权限未授权，安卓可能扫描不到设备');
            resolve();
          }
        });
      });
    }
  } catch (e) { /* ignore */ }
  adapterInit = true;
  return true;
}

// 2. 开始扫描（按services过滤FFE0，减少无关设备）
//    onDeviceFound(dev) 每次发现设备时回调
export async function startScan(onDeviceFound) {
  if (!adapterInit) await initBluetooth();
  // 先停旧扫描（避免重复启动报错）
  try { wx.stopBluetoothDevicesDiscovery(); } catch(e) {}

  await new Promise((resolve, reject) => {
    wx.startBluetoothDevicesDiscovery({
      services: [to128(SERVICE_UUID_16)], // 只扫带有FFE0服务的设备（即ESP32）
      allowDuplicatesKey: false,
      success: resolve,
      fail: reject
    });
  });

  // 注册发现回调
  wx.onBluetoothDeviceFound((res) => {
    const devs = res.devices || [];
    for (const d of devs) {
      if (typeof onDeviceFound === 'function') {
        onDeviceFound({
          deviceId: d.deviceId,
          name: d.name || d.localName || d.advertisServiceUUIDs ? '未知设备' : '(无名称)',
          RSSI: d.RSSI,
          advertisData: d.advertisData,
          raw: d
        });
      }
    }
  });
}

// 3. 停止扫描
export async function stopScan() {
  try {
    await new Promise((resolve) => {
      wx.stopBluetoothDevicesDiscovery({
        complete: resolve
      });
    });
    wx.offBluetoothDeviceFound && wx.offBluetoothDeviceFound();
  } catch (e) { console.warn('[BLE] stopScan warn:', e); }
}

// 4. 连接指定deviceId
export async function connect(targetDeviceId) {
  deviceId = targetDeviceId;
  await new Promise((resolve, reject) => {
    wx.createBLEConnection({
      deviceId: targetDeviceId,
      timeout: 10000,
      success: resolve,
      fail: reject
    });
  });
  return true;
}

// 5. 发现服务 + 锁定 FFE0/FFE2/FFE1 三个UUID
export async function discoverServicesAndChars() {
  if (!deviceId) throw new Error('未连接设备');
  // 5.1 发现所有服务
  const srvRes = await new Promise((resolve, reject) => {
    wx.getBLEDeviceServices({
      deviceId,
      success: resolve,
      fail: reject
    });
  });
  const services = srvRes.services || [];
  // 找FFE0服务
  const targetService = services.find(s => {
    const u = (s.uuid || '').toUpperCase();
    return u === SERVICE_UUID_16 || u === to128(SERVICE_UUID_16);
  });
  if (!targetService) {
    // 兜底：如果严格匹配不到，取第一个服务
    console.warn('[BLE] 未精确找到FFE0服务，使用第一个服务');
    if (services.length === 0) throw new Error('设备无任何BLE服务');
    serviceId = services[0].uuid;
  } else {
    serviceId = targetService.uuid;
  }
  // 5.2 发现该服务下的特征
  const chRes = await new Promise((resolve, reject) => {
    wx.getBLEDeviceCharacteristics({
      deviceId,
      serviceId,
      success: resolve,
      fail: reject
    });
  });
  const chars = chRes.characteristics || [];
  notifyCharId = null;
  writeCharId = null;
  for (const ch of chars) {
    const u = (ch.uuid || '').toUpperCase();
    const isNotify = (u === NOTIFY_UUID_16) || (u === to128(NOTIFY_UUID_16));
    const isWrite  = (u === WRITE_UUID_16)  || (u === to128(WRITE_UUID_16));
    if (isNotify && ch.properties.notify) notifyCharId = ch.uuid;
    if (isWrite  && (ch.properties.write || ch.properties.writeNoResponse)) writeCharId = ch.uuid;
  }
  // 5.3 兜底：如果UUID精确匹配不到，按能力匹配
  if (!notifyCharId) {
    const fallback = chars.find(c => c.properties.notify);
    if (fallback) notifyCharId = fallback.uuid;
  }
  if (!writeCharId) {
    const fallback = chars.find(c => c.properties.write || c.properties.writeNoResponse);
    if (fallback) writeCharId = fallback.uuid;
  }
  if (!notifyCharId) console.warn('[BLE] Notify特征未找到');
  if (!writeCharId)  throw new Error('Write特征未找到（FFE1），无法发送手势数据');
  return { serviceId, notifyCharId, writeCharId };
}

// 6. 订阅FFE2 Notify（可选，用于反向验证ESP32回发的帧）
export async function subscribeNotify(onData) {
  if (!deviceId || !serviceId || !notifyCharId) {
    console.warn('[BLE] subscribeNotify: 条件不足，跳过');
    return false;
  }
  // 先启用通知
  await new Promise((resolve, reject) => {
    wx.notifyBLECharacteristicValueChange({
      deviceId,
      serviceId,
      characteristicId: notifyCharId,
      state: true,
      success: resolve,
      fail: reject
    });
  });
  // 注册接收回调
  wx.onBLECharacteristicValueChange((res) => {
    // 只关心我们订阅的Notify特征
    if (res.characteristicId.toUpperCase() === notifyCharId.toUpperCase() ||
        res.characteristicId.toUpperCase() === writeCharId.toUpperCase()) {
      if (typeof onData === 'function') {
        onData({
          value: res.value,   // ArrayBuffer
          characteristicId: res.characteristicId,
          timestamp: Date.now()
        });
      }
    }
  });
  return true;
}

// 7. 向FFE1写数据（自动分包>20字节，包之间间隔30ms）
export async function writeData(arrayBuffer) {
  if (!deviceId || !serviceId || !writeCharId) {
    throw new Error('蓝牙连接未就绪，无法发送');
  }
  const totalBytes = arrayBuffer.byteLength;
  let offset = 0;
  while (offset < totalBytes) {
    const end = Math.min(offset + NOTIFY_MTU_PAYLOAD, totalBytes);
    const chunk = arrayBuffer.slice(offset, end);
    await new Promise((resolve, reject) => {
      wx.writeBLECharacteristicValue({
        deviceId,
        serviceId,
        characteristicId: writeCharId,
        value: chunk,
        success: resolve,
        fail: reject
      });
    });
    offset = end;
    if (offset < totalBytes) await sleep(30); // 分包间隔：避免ESP32队列溢出
  }
  return true;
}

// 8. 断开连接 + 清理状态
export async function disconnect() {
  try {
    wx.offBLECharacteristicValueChange && wx.offBLECharacteristicValueChange();
  } catch (e) {}
  if (deviceId) {
    try {
      await new Promise((resolve) => {
        wx.closeBLEConnection({
          deviceId,
          complete: resolve
        });
      });
    } catch (e) { console.warn('[BLE] closeBLEConnection warn:', e); }
  }
  deviceId = null;
  serviceId = null;
  notifyCharId = null;
  writeCharId = null;
  return true;
}

// 查询当前连接状态（供页面判断）
export function isConnected() {
  return !!(deviceId && serviceId && writeCharId);
}

// 获取全局连接句柄（页面需要直接用的话）
export function getConnection() {
  return { deviceId, serviceId, notifyCharId, writeCharId };
}
