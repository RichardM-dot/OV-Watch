# OV-Watch（STM32 智能手表）学习导读

> 目录：`04-项目实战/OV-Watch-STM32智能手表`
> 来源：https://github.com/No-Chicken/OV-Watch （约 2.5k star）

## 这是什么

OV-Watch 是国内开发者 No-Chicken 开源的**STM32F411 + FreeRTOS + LVGL 智能手表**：计步、心率、天气、蓝牙、抬腕亮屏、表盘切换……**中文注释、中文文档（作者官网 no-chicken.com 有全套说明手册）**，从硬件（立创 EDA 开源）到软件全开源，是**中文社区最适合完整复刻的 RTOS+GUI 综合项目**——本学习目录里的 FreeRTOS、LVGL、FlashDB 全部能用上。

## 硬件清单（V2.x，摘自 README/硬件开源工程）

- MCU：**STM32F411CEU6**（黑药丸板同款）
- 屏幕：**ST7789 240x240 SPI 彩屏**（`BSP/LCD` 驱动可查）
- 传感器：**MPU6050**（计步/抬腕）、**AHT21**（温湿度）、**EM7028**（心率）、**SPL06_001**（气压，部分版本）
- 存储：**BL24C02**（EEPROM）
- 蓝牙：**KT6328A/KT6368A** 模块（无线传数据/升级）
- 电源：TPS63020 DCDC + 锂电管理（低功耗三模式：运行/睡眠/关机，README 实测运行 70-80mA、待机 ~1mA）
- 触摸：电容触摸（BSP/TOUCH）

## 目录导读

| 目录/文件 | 内容 |
| --- | --- |
| `Software/OV_Watch/` | 主固件（Keil MDK 工程 + CubeMX `OV_Watch.ioc`） |
| `Software/OV_Watch/User/` | **业务代码**：`Tasks/`（FreeRTOS 任务）、`Func/`（页面管理器 PageManager、pubsub 发布订阅）、`GUI_App/`（LVGL 应用） |
| `Software/OV_Watch/BSP/` | 板级驱动：LCD、MPU6050、AHT21、EM7028、KEY、TOUCH、POWER、IIC（软件 I2C 实现） |
| `Software/IAP_F411/` | BootLoader/IAP（串口/蓝牙 OTA 升级） |
| `Hardware/` | 立创 EDA 硬件工程链接与说明 |
| `3D Modle/` | 手表外壳 3D 模型 |
| `lv_sim_vscode_win/` | **Windows LVGL 模拟器工程**（不买硬件先在 PC 上开发 UI！） |
| `Firmware/` | 现成固件（bin/hex）可直接烧 |

## 学习路径（建议）

1. **先跑模拟器**：`lv_sim_vscode_win/` 用 VSCode 打开编译，零硬件先做 UI/页面逻辑。
2. 读 README + 作者官网手册，对照 `Software/OV_Watch` 工程。
3. 核心主线：`User/Tasks/`（各 RTOS 任务）→ `Func/PageManager.c`（页面状态机——**本项目最精华**）→ `BSP/`（逐个外设驱动）。
4. 研究低功耗：STOP 模式进出、RTC 定时唤醒查手势（抬腕检测）、MPU6050 低功耗改造（README 特别说明 DMP 库直接用功耗会很高）。
5. 有条件就画板/买开发板复刻；没条件用 **FriPi 炸鸡派**（作者为想学代码的人做的 F411 开发板）或自焊"黑药丸+模块"平替。

## 学习要点

1. **页面管理器**：PageManager + 注册表让"表盘/菜单/应用"解耦，切换带动画——比裸写 switch-case 优雅一个量级。
2. **发布订阅（pubsub.c）**：传感器数据到 UI 的解耦传输，学事件驱动架构。
3. **低功耗三板斧**：STOP 模式 + 外设中断唤醒 + 传感器低功耗配置；用实测功耗（README 的 70-80mA vs 1mA）倒推代码哪里在耗电。
4. **软 I2C 实现**（BSP/IIC）：任何 GPIO 都能当 I2C 用，调试期神器。
5. **IAP/BootLoader**：App 与 Bootloader 的分区、跳转与升级流程。

## 建议购买/准备（三档）

- **0 元档**：Windows LVGL 模拟器 + 读代码（UI/架构全学会）。
- **平替开发板档（¥100-200，推荐）**：
  - STM32F411CEU6 黑药丸板（¥30-50，与手表主控同型号！）
  - 1.54/1.69 寸 ST7789 SPI 彩屏（¥20-40）
  - MPU6050 模块（¥5-10）、AHT20/21 模块（¥5-10）、MAX30102 心率模块（¥8-15，与 EM7028 同类）
  - 杜邦线+面包板 ¥10 —— 可以把大半外设跑起来
- **完整复刻档（¥300-500）**：按立创开源工程（README 里的 oshwhub 链接）打板 + 贴片（立创 SMT 服务），或直接买作者配套的 FryPi 开发板；3D 打印表壳。
- 工具：ST-Link V2（¥15-25）。

## 学习目标（自测）

- [ ] 模拟器里加一个自己的手表页面
- [ ] 说出 PageManager 如何管理页面栈与动画
- [ ] 解释抬腕亮屏的实现链路（MPU 中断→手势判断→点亮）
- [ ] 在黑药丸+彩屏上跑起表盘并测一次睡眠电流
