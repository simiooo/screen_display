declare module 'render2display_addon' {
  export enum FontWeight {
    Thin = 100,
    ExtraLight = 200,
    Light = 300,
    Normal = 400,
    Medium = 500,
    SemiBold = 600,
    Bold = 700,
    ExtraBold = 800,
    Black = 900,
    ExtraBlack = 950,
  }

  export class Direct2DDisplay {
    constructor();
    
    /**
     * 启动文本渲染
     * @param text 初始显示文本
     * @param displayIndex 可选显示器索引 (默认为 0)
     * @throws 当显示器索引无效时抛出错误
     */
    start(text: string, displayIndex?: number): boolean;

    /**
     * 停止渲染并清理资源
     */
    stop(): boolean;

    /**
     * 更新显示文本内容
     * @param text 新的文本内容 (UTF-8 格式)
     */
    updateText(text: string): void;

    /**
     * 更新文本位置
     * @param x 水平坐标 (相对于显示器左上角)
     * @param y 垂直坐标 (相对于显示器左上角)
     */
    updatePosition(x: number, y: number): void;

    /**
     * 更新文本样式
     * @param fontSize 字体大小（像素）
     * @param weight 字体粗细（使用 FontWeight 枚举）
     */
    updateStyle(fontSize: number, weight: FontWeight): void;
  }
}
