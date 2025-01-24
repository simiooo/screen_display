const { Direct2DDisplay } = require('../dist/main.cjs');

// 创建实例
const display = new Direct2DDisplay();

// 启动渲染
display.start(0); // 使用第一个显示器

// 初始显示
display.updateAll(100, 200, "Direct2D 演示");

// 动态更新
let angle = 0;
setInterval(() => {
  angle += 0.1;
  const x = 500 + Math.sin(angle) * 200;
  const y = 300 + Math.cos(angle) * 100;
  display.updateAll(x, y, `坐标: (${x.toFixed(1)}, ${y.toFixed(1)})`);
}, 16);

// 安全退出
process.on('SIGINT', () => {
  display.stop();
  process.exit();
});
