// pages/index/index.js
// 首页：蓝牙初始化 + 设备扫描 + 连接 + 跳手势页
import {
  initBluetooth,
  startScan,
  stopScan,
  connect,
  discoverServicesAndChars,
  subscribeNotify,
  disconnect,
  isConnected
} from '../../utils/ble.js';

Page({
  data: {
    scanning: false,
    connecting: false,
    deviceList: [],         // [{deviceId, name, RSSI, advertisData}]
    selectedIndex: -1,
    _deviceIdSet: {}        // 内部用：去重map，不渲染
  },

  // ===== 生命周期 =====
  async onLoad() {
    try {
      await initBluetooth();
      console.log('[Index] 蓝牙适配器初始化成功');
      wx.showToast({ title: '蓝牙就绪', icon: 'success', duration: 1500 });
    } catch (e) {
      console.error('[Index] 蓝牙初始化失败:', e);
      const msg = (e && e.errMsg) ? e.errMsg : '请确保手机蓝牙已开启';
      wx.showModal({
        title: '蓝牙不可用',
        content: msg + '\n\n请检查：\n1. 手机蓝牙是否已开启\n2. 安卓用户GPS是否已开启\n3. 小程序蓝牙/位置权限是否允许',
        showCancel: false,
        confirmText: '知道了'
      });
    }
  },

  onUnload() {
    // 页面卸载时若还在扫描，停止并断开（避免资源泄漏）
    if (this.data.scanning) this._stopScanSafe();
  },

  onHide() {
    if (this.data.scanning) this._stopScanSafe();
  },

  // ===== 扫描按钮 =====
  async onScanTap() {
    if (this.data.scanning) {
      this._stopScanSafe();
      return;
    }
    try {
      // 清空旧列表
      this.setData({ deviceList: [], selectedIndex: -1, _deviceIdSet: {} });
      await startScan((dev) => {
        // 去重：按deviceId合并（RSSI取最新）
        const set = this.data._deviceIdSet;
        if (set[dev.deviceId] !== undefined) {
          // 更新已有项的RSSI
          const idx = set[dev.deviceId];
          const list = this.data.deviceList.slice();
          list[idx] = Object.assign({}, list[idx], { RSSI: dev.RSSI });
          // 名字如果之前是空，补充上
          if (!list[idx].name && dev.name) list[idx].name = dev.name;
          this.setData({ deviceList: list });
        } else {
          // 新设备
          const list = this.data.deviceList.slice();
          list.push(dev);
          set[dev.deviceId] = list.length - 1;
          this.setData({ deviceList: list, _deviceIdSet: set });
        }
      });
      this.setData({ scanning: true });
      // 扫描10秒后自动停止（省电+安全）
      this._scanTimer = setTimeout(() => {
        if (this.data.scanning) {
          this._stopScanSafe();
          wx.showToast({ title: '扫描已停止', icon: 'none' });
        }
      }, 10000);
    } catch (e) {
      console.error('[Index] 启动扫描失败:', e);
      wx.showModal({
        title: '扫描失败',
        content: (e && e.errMsg) || '未知错误',
        showCancel: false
      });
    }
  },

  _stopScanSafe() {
    if (this._scanTimer) {
      clearTimeout(this._scanTimer);
      this._scanTimer = null;
    }
    stopScan().finally(() => {
      this.setData({ scanning: false });
    });
  },

  // ===== 选中设备 =====
  onDeviceTap(e) {
    const idx = e.currentTarget.dataset.index;
    if (idx === undefined || idx < 0) return;
    this.setData({ selectedIndex: idx });
  },

  // ===== 连接按钮 =====
  async onConnectTap() {
    const idx = this.data.selectedIndex;
    if (idx < 0 || !this.data.deviceList[idx]) {
      wx.showToast({ title: '请先选择设备', icon: 'none' });
      return;
    }
    const dev = this.data.deviceList[idx];
    this.setData({ connecting: true });
    try {
      // 1. 先停止扫描
      this._stopScanSafe();

      // 2. 连接
      await connect(dev.deviceId);
      console.log('[Index] BLE连接建立成功:', dev.deviceId);

      // 3. 延迟一小会儿（安卓部分机型需要等GATT稳定）
      await new Promise(r => setTimeout(r, 600));

      // 4. 发现服务 + 锁定FFE0/FFE2/FFE1
      const info = await discoverServicesAndChars();
      console.log('[Index] 服务与特征发现完成:', info);

      // 5. 可选：订阅Notify（用于反向验证ESP32回发的帧）
      try {
        await subscribeNotify((evt) => {
          // 如果页面注册了回调，这里不处理；留作调试扩展点
          console.log('[Index] 收到Notify（可选调试用）长度:', evt.value && evt.value.byteLength);
        });
      } catch (notifyErr) {
        console.warn('[Index] 订阅Notify失败（非致命，继续）:', notifyErr);
      }

      // 6. 跳转手势页
      wx.showToast({ title: '连接成功', icon: 'success', duration: 800 });
      setTimeout(() => {
        wx.navigateTo({
          url: '/pages/gesture/gesture?deviceName=' + encodeURIComponent(dev.name || 'ESP32')
        });
      }, 800);

    } catch (e) {
      console.error('[Index] 连接失败:', e);
      const msg = (e && e.errMsg) || '连接超时或服务未找到';
      wx.showModal({
        title: '连接失败',
        content: msg + '\n\n建议：\n1. 靠近设备重试\n2. 重启ESP32\n3. 关闭系统蓝牙再重新开启',
        showCancel: false
      });
      // 失败后释放连接句柄
      try { await disconnect(); } catch(_) {}
    } finally {
      this.setData({ connecting: false });
    }
  }
});
