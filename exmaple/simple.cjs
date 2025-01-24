const { Direct2DDisplay } = require('../dist/main.cjs') ;
console.log(Direct2DDisplay)
const display = new Direct2DDisplay();

// 启动渲染
display.start("Direct2D 演示", 0);

// 动态更新位置和内容
let angle = 0;
setInterval(() => {
  angle += 0.1;
  const x = 500 + Math.sin(angle) * 200;
  const y = 300 + Math.cos(angle) * 100;
  
  display.updatePosition(x, y);
  display.updateText(`坐标: (${x.toFixed(1)}, ${y.toFixed(1)})`);
}, 16);

// 5秒后修改样式
setTimeout(() => {
  display.updateStyle(24, 400); // 恢复普通样式
}, 5000);

// 安全退出
process.on('SIGINT', () => {
  display.stop();
  process.exit();
});