// pages/gesture/gesture.js
// 手势页核心：12手势 → 11路CH发送值 映射表 + 点击网格回调发送BLE Write
import { writeData, disconnect, isConnected, getConnection } from '../../utils/ble.js';
import { encode11chFrame, arrayBufferToString } from '../../utils/frame.js';

// ============================================================
// 🔑 核心映射表：12手势 → 11路CH输入值（1500=低档，1800=高档）
// 对应ESP32端修正后的 s_gestures 数组（ui_main.c L748~761）
// 映射规则：
//   PWM=1000/1400 → CH输入=1500（≤1650 → pwm_manager自动出低档）
//   PWM=2000/1750 → CH输入=1800（>1650  → pwm_manager自动出高档）
//   CH6 由CH1自动派生（不需小程序手动设），CH7~CH11 固定1500
// ============================================================
const GESTURES = [
  {
    name: '1',    image: '/images/1.png',
    // PWM目标: 1000 2000 1000 1000 1000 1400 → CH2=高档1800
    ch: [1500,1800,1500,1500,1500, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '1000 2000 1000 1000 1000 1400'
  },
  {
    name: '2',    image: '/images/2.png',
    // PWM目标: 1000 2000 2000 1000 1000 1400 → CH2/3=高档1800
    ch: [1500,1800,1800,1500,1500, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '1000 2000 2000 1000 1000 1400'
  },
  {
    name: '3',    image: '/images/3.png',
    // PWM目标: 1000 1000 2000 2000 2000 1400 → CH2/3/4=高档
    ch: [1500,1800,1800,1800,1500, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '1000 2000 2000 2000 1000 1400'
  },
  {
    name: '4',    image: '/images/4.png',
    // PWM目标: 1000 2000 2000 2000 2000 1400 → CH1=低档
    ch: [1500,1800,1800,1800,1800, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '1400 1000 2000 2000 2000 1400'
  },
  {
    name: '5',    image: '/images/5.png',
    // PWM目标: 2000 2000 2000 2000 2000 1750 → CH1~5=全高档
    ch: [1800,1800,1800,1800,1800, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '2000 2000 2000 2000 2000 1750'
  },
  {
    name: '6',    image: '/images/6.png',
    // PWM目标: 2000 2000 1000 1000 1000 1750 → CH1/5=高档, CH2/3/4=低档
    ch: [1800,1500,1500,1500,1800, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '2000 1000 1000 1000 2000 1750'
  },
  {
    name: '7',    image: '/images/7.png',
    // PWM目标: 2000 2000 2000 1000 1000 1750 → CH2/3/4=高档
    ch: [1800,1800,1800,1500,1500, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '2000 2000 2000 1000 1000 1750'
  },
  {
    name: '8',    image: '/images/8.png',
    // PWM目标: 2000 2000 1000 1000 1000 1750 → CH1/2=高档
    ch: [1800,1800,1500,1500,1500, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '2000 2000 1000 1000 1000 1750'
  },
  {
    name: '10',   image: '/images/10.png',
    // PWM目标: 1000 1000 1000 1000 1000 1400 → CH1~5=全低档
    ch: [1500,1500,1500,1500,1500, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '1000 1000 1000 1000 1000 1400'
  },
  {
    name: 'ok',   image: '/images/ok.png',
    // PWM目标: 1000 1000 2000 2000 2000 1400 → CH1/2=低档
    ch: [1500,1500,1800,1800,1800, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '1000 1000 2000 2000 2000 1400'
  },
  {
    name: 'good', image: '/images/good.png',
    // PWM目标: 1000 2000 2000 2000 2000 1400 → CH1=低档
    ch: [1500,1800,1800,1800,1800, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '1000 2000 2000 2000 2000 1400'
  },
  {
    name: 'love', image: '/images/love.png',
    // PWM目标: 2000 2000 1000 1000 2000 1750 → 同手势8
    ch: [1800,1800,1500,1500,1800, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '2000 2000 1000 1000 2000 1750'
  }
];

// 归零姿态（s_gesture_rest_pose 修正版：1750,2000,2000,2000,2000,2000 → 全高档CH1~5=1800）
const RESET_CH = [1800,1800,1800,1800,1800, 1500,1500,1500,1500,1500,1500];
const RESET_SUMMARY = '2000 2000 2000 2000 2000 1750';
const DEFAULT_SUMMARY = '---- ---- ---- ---- ---- ----';

Page({
  data: {
    gestures: GESTURES,
    selectedIndex: -1,
    summaryText: DEFAULT_SUMMARY,
    deviceName: 'ESP32 设备',
    resetting: false,
    disconnecting: false,
    _sending: false    // 内部发送锁：避免点击过快导致BLE分包重叠
  },

  // ===== 生命周期 =====
  onLoad(options) {
    const conn = getConnection();
    console.log('[Gesture] 进入手势页，当前连接状态:', isConnected(), conn);
    if (options && options.deviceName) {
      try {
        this.setData({ deviceName: decodeURIComponent(options.deviceName) });
      } catch (e) { this.setData({ deviceName: options.deviceName }); }
    }
    // 进入页面后默认先发一次「归零姿态」，确保舵机有初始位置
    // 不强制，避免连上就动吓到用户；用户可手动点归零按钮
  },

  onUnload() {
    // 页面被卸载（如用户按返回）时主动断开BLE
    // 不强制断：允许用户返回首页连其他设备；这里只记录
    console.log('[Gesture] 页面卸载');
  },

  // ============================================================
  // 🔑 核心回调：点击手势网格 → 组装11路CH帧 → BLE Write发送
  // ============================================================
  async onGestureTap(e) {
    if (this.data._sending) {
      wx.showToast({ title: '请稍候…', icon: 'none', duration: 600 });
      return;
    }
    const idx = e.currentTarget.dataset.index;
    if (idx === undefined || idx < 0 || idx >= GESTURES.length) return;
    if (!isConnected()) {
      wx.showModal({
        title: '蓝牙未连接',
        content: '请返回首页重新连接ESP32设备',
        showCancel: false,
        success: () => wx.navigateBack()
      });
      return;
    }
    const g = GESTURES[idx];
    this.setData({ _sending: true });
    try {
      wx.showLoading({ title: `发送 ${g.name}`, mask: true });

      // 1. 组装帧：11路CH → ArrayBuffer
      const ab = encode11chFrame(g.ch);
      // 调试日志：打印将要发送的字符串（真机调试时看Console）
      console.log(`[Gesture] 发送手势[${g.name}] 帧: ${arrayBufferToString(ab)} 字节=${ab.byteLength}`);

      // 2. BLE Write（自动分包>20字节，包之间间隔30ms）
      await writeData(ab);
      wx.hideLoading();

      // 3. 发送成功 → UI反馈：选中高亮 + 底部PWM更新 + Toast
      this.setData({
        selectedIndex: idx,
        summaryText: g.pwmSummary
      });
      wx.showToast({
        title: `${g.name} 已发送`,
        icon: 'success',
        duration: 900
      });
    } catch (err) {
      wx.hideLoading();
      console.error('[Gesture] 发送失败:', err);
      const msg = (err && err.errMsg) ? err.errMsg : (err ? String(err) : '未知错误');
      wx.showToast({
        title: '发送失败',
        icon: 'none',
        duration: 2000
      });
      // 连接异常：提示返回首页
      if (msg.indexOf('not found') >= 0 || msg.indexOf('disconnect') >= 0 || msg.indexOf('fail') >= 0) {
        setTimeout(() => {
          wx.showModal({
            title: '连接异常',
            content: '蓝牙链路可能已断开，是否返回首页重新连接？',
            confirmText: '返回首页',
            success: (r) => { if (r.confirm) wx.navigateBack(); }
          });
        }, 300);
      }
    } finally {
      this.setData({ _sending: false });
    }
  },

  // ===== 归零姿态按钮 =====
  async onResetTap() {
    if (this.data._sending) return;
    if (!isConnected()) {
      wx.showToast({ title: '蓝牙未连接', icon: 'none' });
      return;
    }
    this.setData({ _sending: true, resetting: true });
    try {
      wx.showLoading({ title: '归零中', mask: true });
      const ab = encode11chFrame(RESET_CH);
      console.log(`[Gesture] 归零帧: ${arrayBufferToString(ab)} 字节=${ab.byteLength}`);
      await writeData(ab);
      wx.hideLoading();
      this.setData({
        selectedIndex: -1,
        summaryText: RESET_SUMMARY
      });
      wx.showToast({ title: '已归零', icon: 'success', duration: 900 });
    } catch (err) {
      wx.hideLoading();
      console.error('[Gesture] 归零失败:', err);
      wx.showToast({ title: '归零失败', icon: 'none' });
    } finally {
      this.setData({ _sending: false, resetting: false });
    }
  },

  // ===== 断开连接 → 返回首页 =====
  async onDisconnectTap() {
    if (this.data.disconnecting) return;
    this.setData({ disconnecting: true });
    try {
      wx.showLoading({ title: '断开中', mask: true });
      await disconnect();
      wx.hideLoading();
      wx.showToast({ title: '已断开', icon: 'success', duration: 600 });
      setTimeout(() => wx.navigateBack(), 500);
    } catch (e) {
      wx.hideLoading();
      // 断开失败也返回首页
      wx.navigateBack();
    } finally {
      this.setData({ disconnecting: false });
    }
  }
});
