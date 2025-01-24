# Direct2D Display

一个使用Windows Direct2D API实现的Node.js原生模块，用于在指定显示器上渲染文字内容。

## 功能特性

- 支持多显示器选择
- 实时文字渲染
- 可自定义文字内容、位置、大小、粗细
- 透明背景显示
- 高性能渲染（~60 FPS）
- 线程安全的配置更新
- 获取显示器信息

## 安装

```bash
npm install direct2d-display
```

需要Windows系统和Node.js环境。

## 使用示例

```javascript
const { Direct2DDisplay, FontWeight } = require('direct2d-display');

// 创建实例
const display = new Direct2DDisplay();

// 获取显示器信息
const displays = display.getDisplays();
console.log('可用显示器:', displays);

// 启动渲染
display.start(0); // 使用第一个显示器

// 更新所有参数
display.updateAll(100, 200, "Hello Direct2D");

// 动态更新
let angle = 0;
setInterval(() => {
  angle += 0.1;
  const x = 500 + Math.sin(angle) * 200;
  const y = 300 + Math.cos(angle) * 100;
  display.updateAll(x, y, `坐标: (${x.toFixed(1)}, ${y.toFixed(1)})`);
}, 16);

// 修改样式
setTimeout(() => {
  display.updateStyle(32, FontWeight.Bold);
}, 5000);

// 安全退出
process.on('SIGINT', () => {
  display.stop();
  process.exit();
});
```

## API文档

### FontWeight 枚举
定义字体粗细级别：
- Thin = 100
- ExtraLight = 200
- Light = 300
- Normal = 400
- Medium = 500
- SemiBold = 600
- Bold = 700
- ExtraBold = 800
- Black = 900
- ExtraBlack = 950

### Direct2DDisplay

#### constructor()
创建Direct2DDisplay实例

#### start(displayIndex?: number): void
启动渲染线程
- `displayIndex`: 显示器索引，默认为0（主显示器）

#### stop(): void
停止渲染并清理资源

#### updateAll(x: number, y: number, text: string): void
更新所有渲染参数
- `x`: 水平坐标
- `y`: 垂直坐标
- `text`: 显示文本

## 注意事项

1. 需要Windows 7及以上版本
2. 需要安装Visual C++ Redistributable
3. 文字渲染使用Arial字体
4. 显示窗口为全屏无边框，可通过Alt+F4关闭
5. 建议在主线程调用API，渲染在独立线程进行

## 开发

```bash
# 安装依赖
npm install

# 编译
npm run build

# 测试
npm test
```

## License

MIT
