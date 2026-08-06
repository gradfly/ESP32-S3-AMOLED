// app.js - 小程序入口
App({
  onLaunch() {
    console.log('[App] 小程序启动');
  },
  onShow() {
    console.log('[App] 小程序切前台');
  },
  onHide() {
    console.log('[App] 小程序切后台');
  },
  globalData: {
    // 全局蓝牙连接状态（供各页面共享）
    ble: {
      deviceId: null,
      serviceId: null,
      notifyCharId: null,
      writeCharId: null,
      connected: false
    }
  }
});
