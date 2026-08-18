// pages/gesture/gesture.js
// 手势页核心：12手势 → 11路CH发送值 映射表 + 点击网格回调发送BLE Write
import { writeData, disconnect, isConnected, getConnection } from '../../utils/ble.js';
import { encode11chFrame, arrayBufferToString } from '../../utils/frame.js';

// ============================================================
// 🔑 核心映射表：12手势 → 11路CH输入值（500=高档，1000=低档）
// 对应ESP32端修正后的 s_gestures 数组（ui_main.c L748~761）
// 映射规则：
//   PWM=1000/1400 → CH输入=500（<650 → pwm_manager自动出高档）
//   PWM=2000/1750 → CH输入=1000（>650 → pwm_manager自动出低档）
//   CH6 由CH1自动派生（不需小程序手动设），CH7~CH11 固定1500
// ============================================================
const GESTURES = [
  {
    name: '1',    image: '/images/1.png',
    // PWM目标: 1000 2000 1000 1000 1000 1400 → CH2=高档1800
    ch: [1000,500,1000,1000,1000, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '1000 2000 1000 1000 1000 1400'
  },
  {
    name: '2',    image: '/images/2.png',
    // PWM目标: 1000 2000 2000 1000 1000 1400 → CH2/3=高档1800
    ch: [1000,500,500,1000,1000, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '1000 2000 2000 1000 1000 1400'
  },
  {
    name: '3',    image: '/images/3.png',
    // PWM目标: 1000 1000 2000 2000 2000 1400 → CH2/3/4=高档
    ch: [1000,500,500,500,1000, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '1000 2000 2000 2000 1000 1400'
  },
  {
    name: '4',    image: '/images/4.png',
    // PWM目标: 1000 2000 2000 2000 2000 1400 → CH1=低档
    ch: [1000,500,500,500,500, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '1400 1000 2000 2000 2000 1400'
  },
  {
    name: '5',    image: '/images/5.png',
    // PWM目标: 2000 2000 2000 2000 2000 1750 → CH1~5=全高档
    ch: [500,500,500,500,500, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '2000 2000 2000 2000 2000 1750'
  },
  {
    name: '6',    image: '/images/6.png',
    // PWM目标: 2000 2000 1000 1000 1000 1750 → CH1/5=高档
    ch: [500,1000,1000,1000,500, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '2000 1000 1000 1000 2000 1750'
  },
  {
    name: '7',    image: '/images/7.png',
    // PWM目标: 2000 2000 2000 1000 1000 1750 → CH3/4=低档
    ch: [500,500,500,1000,1000, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '2000 2000 2000 1000 1000 1750'
  },
  {
    name: '8',    image: '/images/8.png',
    // PWM目标: 2000 2000 1000 1000 1000 1750 → CH1/2=高档
    ch: [500,500,1000,1000,1000, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '2000 2000 1000 1000 1000 1750'
  },
  {
    name: '10',   image: '/images/10.png',
    // PWM目标: 1000 1000 1000 1000 1000 1400 → CH1~5=全低档
    ch: [1000,1000,1000,1000,1000, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '1000 1000 1000 1000 1000 1400'
  },
  {
    name: 'ok',   image: '/images/ok.png',
    // PWM目标: 1000 1000 2000 2000 2000 1400 → CH1/2=低档
    ch: [1000,1000,500,500,500, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '1000 1000 2000 2000 2000 1400'
  },
  {
    name: 'good', image: '/images/good.png',
    // PWM目标: 2000 1000 1000 1000 1000 1750 → CH1=高档
    ch: [500,1000,1000,1000,1000, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '2000 1000 1000 1000 1000 1750'
  },
  {
    name: 'love', image: '/images/love.png',
    // PWM目标: 2000 2000 1000 1000 2000 1750 → CH3/4=低档
    ch: [500,500,1000,1000,500, 1500,1500,1500,1500,1500,1500],
    pwmSummary: '2000 2000 1000 1000 2000 1750'
  }
];

// 初始姿态（s_gesture_rest_pose 修正版：1750,2000,2000,2000,2000,2000 → 全高档CH1~5=2000）
const RESET_CH = [500,500,500,500,500, 1500,1500,1500,1500,1500,1500];
const RESET_SUMMARY = '500 500 500 500 500 1750';
const DEFAULT_SUMMARY = '---- ---- ---- ---- ---- ----';
// 急停数据帧：CH1~5=650（恰好=阈值650 → ESP32 输出中档1500us），CH6由CH1派生→1400us
const ESTOP_CH = [650,650,650,650,650, 1500,1500,1500,1500,1500,1500];
const ESTOP_SUMMARY = '1500 1500 1500 1500 1500 1500';

Page({
  data: {
    gestures: GESTURES,
    selectedIndex: -1,
    summaryText: DEFAULT_SUMMARY,
    deviceName: 'ESP32 设备',
    resetting: false,
    disconnecting: false,
    _sending: false,    // 内部发送锁：避免点击过快导致BLE分包重叠
    estopOn: false      // 急停开关状态：与 ESP32 端 pwm_manager_get_estop() 同步
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
    // _savedCH：急停开启前保存的 CH 数组，恢复按钮直接发送它（正常数据帧）
    // ESP32 收到后自动关闭急停并按此 CH 值刷新输出
    this._savedCH = RESET_CH.slice();  // 默认初始姿态
  },

  onUnload() {
    // 页面被卸载（如用户按返回）时主动断开BLE
    // 不强制断：允许用户返回首页连其他设备；这里只记录
    console.log('[Gesture] 页面卸载');
  },

  // ============================================================
  // 🔑 核心回调：点击手势网格 → 组装11路CH帧 → BLE Write发送
  // 急停状态下点手势：直接发送手势CH帧（ESP32收到正常帧自动解除急停），
  //   本地 estopOn 同步置 false
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

      // 组装帧：11路CH → ArrayBuffer
      const ab = encode11chFrame(g.ch);
      console.log(`[Gesture] 发送手势[${g.name}] 帧: ${arrayBufferToString(ab)} 字节=${ab.byteLength}`);

      // BLE Write（自动分包>20字节，包之间间隔30ms）
      await writeData(ab);
      wx.hideLoading();

      // 发送成功 → UI反馈：选中高亮 + 底部PWM更新 + Toast
      this._savedCH = g.ch.slice();  // 保存当前 CH 供急停恢复使用
      this.setData({
        selectedIndex: idx,
        summaryText: g.pwmSummary,
        estopOn: false  // 急停状态下点手势 → 自动解除急停
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

  // ===== 初始姿态按钮 =====
  // 急停状态下点初始姿态：直接发送 RESET_CH 帧（ESP32收到正常帧自动解除急停）
  async onResetTap() {
    if (this.data._sending) return;
    if (!isConnected()) {
      wx.showToast({ title: '蓝牙未连接', icon: 'none' });
      return;
    }
    this.setData({ _sending: true, resetting: true });
    try {
      wx.showLoading({ title: '初始中', mask: true });
      const ab = encode11chFrame(RESET_CH);
      console.log(`[Gesture] 初始姿态帧: ${arrayBufferToString(ab)} 字节=${ab.byteLength}`);
      await writeData(ab);
      wx.hideLoading();
      this._savedCH = RESET_CH.slice();  // 保存当前 CH 供急停恢复使用
      this.setData({
        selectedIndex: -1,
        summaryText: RESET_SUMMARY,
        estopOn: false  // 解除急停
      });
      wx.showToast({ title: '已恢复初始姿态', icon: 'success', duration: 900 });
    } catch (err) {
      wx.hideLoading();
      console.error('[Gesture] 初始姿态失败:', err);
      wx.showToast({ title: '初始姿态失败', icon: 'none' });
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
  },

  // ============================================================
  // 🔑 急停按钮：
  //   开启 → 发送 ESTOP_CH 数据帧 [650,650,650,650,650,1500,...]
  //           ESP32 按正常CH映射输出（650≤阈值→CH1~5=1000us, CH6=1400us）
  //           本地按钮变深红+黄边，文字"恢复按钮"
  //   恢复 → 发送 _savedCH（急停前保存的CH数组，正常数据帧）
  //           ESP32 按此 CH 值刷新输出，本地按钮回亮红，文字"急停按钮"
  // ============================================================
  async onEstopTap() {
    if (this.data._sending) {
      wx.showToast({ title: '请稍候…', icon: 'none', duration: 600 });
      return;
    }
    if (!isConnected()) {
      wx.showModal({
        title: '蓝牙未连接',
        content: '请返回首页重新连接ESP32设备',
        showCancel: false,
        success: () => wx.navigateBack()
      });
      return;
    }
    const nextOn = !this.data.estopOn;
    this.setData({ _sending: true });
    try {
      if (nextOn) {
        // ===== 开启急停：发送 ESTOP_CH 数据帧 =====
        wx.showLoading({ title: '急停中', mask: true });
        const ab = encode11chFrame(ESTOP_CH);
        console.log(`[Gesture] 急停帧: ${arrayBufferToString(ab)}`);
        await writeData(ab);
        wx.hideLoading();
        this.setData({
          estopOn: true,
          summaryText: ESTOP_SUMMARY
        });
        wx.showToast({ title: '已急停', icon: 'none', duration: 900 });
      } else {
        // ===== 恢复：发送急停前保存的 CH 数据帧 =====
        wx.showLoading({ title: '恢复中', mask: true });
        const resumeCH = this._savedCH || RESET_CH.slice();
        const ab = encode11chFrame(resumeCH);
        console.log(`[Gesture] 恢复帧(急停前CH): ${arrayBufferToString(ab)}`);
        await writeData(ab);
        wx.hideLoading();

        // 恢复汇总显示：根据选中项或初始姿态
        let resumeSummary = RESET_SUMMARY;
        if (this.data.selectedIndex >= 0 && this.data.selectedIndex < GESTURES.length) {
          resumeSummary = GESTURES[this.data.selectedIndex].pwmSummary;
        }
        this.setData({
          estopOn: false,
          summaryText: resumeSummary
        });
        wx.showToast({ title: '已恢复', icon: 'success', duration: 900 });
      }
    } catch (err) {
      wx.hideLoading();
      console.error('[Gesture] 急停命令发送失败:', err);
      const msg = (err && err.errMsg) ? err.errMsg : (err ? String(err) : '未知错误');
      wx.showToast({
        title: '发送失败',
        icon: 'none',
        duration: 2000
      });
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
  }
});
