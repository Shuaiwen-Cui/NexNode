# 太阳能/电池管理模块

太阳能能源管理模块A支持通过太阳能电板和USB接口为3.7V 18650锂电池充电，配备5V/3.1A电压调节输出。集成MPPT（最大功率点跟踪）和电池充放电保护功能。

<model-viewer
    src="../../../../static/models/bms.glb"
    alt="BMS 3D model"
    camera-controls
    auto-rotate
    auto-rotate-delay="3000"
    rotation-per-second="1deg"
    interaction-prompt="none"
    shadow-intensity="0.6"
    exposure="0.5"
    environment-image="legacy"
    orientation="0deg 0deg 30deg"
    ar
    ar-modes="webxr scene-viewer quick-look"
    style="display: block; width: 100%; height: 600px;">
</model-viewer>

*3D模型*

![](BMS.png)
*外观图*

![](bms-dim.png)
*尺寸图*

## 关键规格

| 参数 | 规格 |
|------|------|
| 太阳能输入 | 5-24V |
| USB输出 | 5V/3.1A |
| 电池 | 3.7V 18650锂电池 |
| 充电保护 | 4.2V ±1% |
| 放电保护 | 3.0V ±1% |
| 尺寸 | 61mm × 54mm |
| 重量 | 28g |

## 资源下载

<div class="grid cards" markdown>

-   :material-file:{ .lg .middle } __产品详情页__

    ---

    [:octicons-arrow-right-24: <a href="https://seengreat.com/wiki/161/" target="_blank"> 查看 </a>](#)

</div>

## 挂载设计

- **PCB开孔** 如上图所示，PCB开孔直径为3mm，使用于M2.5螺丝安装。

- **螺丝** 螺丝规格为M2.5x5，螺丝外径2.5mm，长度5mm，配合热熔螺母使用。

- **热熔螺母** 螺母选用热熔螺母，内嵌于3D打印件中，安装时将螺母加热后压入预留孔位，确保牢固固定。其规格为M2.5x4x4，即内径2.5mm，高度4mm，外径4mm。