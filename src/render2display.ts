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
     * 启动渲染线程
     * @param displayIndex 显示器索引 (默认为0)
     */
    start(displayIndex?: number): void;

    /**
     * 停止渲染并清理资源
     */
    stop(): void;

    /**
     * 更新所有渲染参数
     * @param x 水平坐标
     * @param y 垂直坐标
     * @param text 显示文本
     */
    updateAll(x: number, y: number, text: string): void;
  }
}
