# Direct2D Display

一个使用Windows Direct2D API实现的Node.js原生模块，用于在指定显示器上渲染文字内容。

## 功能特性

- 支持多显示器选择
- 实时文字渲染
- 可自定义文字内容、位置、大小、颜色
- 透明背景显示
- 高性能渲染（~60 FPS）
- 线程安全的配置更新

## 安装

```bash
npm install direct2d-display
```

需要Windows系统和Node.js环境。

## 使用示例

```javascript
const { Direct2DDisplay } = require('direct2d-display');

// 创建实例
const display = new Direct2DDisplay();

// 启动显示，指定初始文字和显示器索引
display.start("Hello World", 0);

// 更新文字内容
display.updateText("New Text");

// 更新文字位置
display.updatePosition(100, 200);

// 更新文字样式
display.updateStyle(64, 700); // 字体大小64，加粗

// 停止显示
display.stop();
```

## API文档

### Direct2DDisplay

#### start(text: string, displayIndex?: number): boolean
启动文字显示
- `text`: 初始显示文字
- `displayIndex`: 显示器索引，默认为0（主显示器）

#### stop(): boolean
停止文字显示

#### updateText(text: string): void
更新显示文字内容
- `text`: 新的文字内容

#### updatePosition(x: number, y: number): void
更新文字显示位置
- `x`: X坐标
- `y`: Y坐标

#### updateStyle(fontSize: number, fontWeight: number): void
更新文字样式
- `fontSize`: 字体大小
- `fontWeight`: 字体粗细（100-900）

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
