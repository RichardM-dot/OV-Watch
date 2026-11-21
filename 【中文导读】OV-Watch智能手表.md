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

## 工程代码阅读顺序（按文件）

> 原则：**沿启动链读主干，外设驱动当字典查**。不要从 `Drivers/`、`Middlewares/LVGL/` 开始——那是 CubeMX 和 LVGL 自动生成的"地基"，用到再查。
> 下面的编号是建议顺序，每一步都可独立停下做实验；主干 17 个导读文件约 3600 行，一周可读完。

### 第 0 步：模拟器热身（可选，约 1 小时）

先建立"UI 代码怎么跑起来"的直觉，再进固件：

| 文件 | 看什么 |
| --- | --- |
| `lv_sim_vscode_win/main.c` | SDL 建窗口 + 调 `ui_init()`，PC 上跑 LVGL 的最小闭环 |
| `lv_sim_vscode_win/user_test/GUI_App/ui.c` | 与固件共用同一份 UI 代码的入口 |
| `lv_sim_vscode_win/user_test/Func/Src/PageManager.c` | 页面管理器的仿真副本（`HW_USE_HARDWARE=0` 时脱离硬件） |

### 第 1 步：启动链——谁在什么时候跑（4 个文件，约 850 行）

| # | 文件 | 阅读重点 |
| --- | --- | --- |
| 1 | `Software/OV_Watch/Core/Src/main.c` | `HAL_Init` → `MX_GPIO/DMA/ADC/RTC/SPI/TIM/USART_Init` → `osKernelInitialize` → `osKernelStart`，只负责"上电到调度器" |
| 2 | `Core/Src/freertos.c` | `MX_FREERTOS_Init()` 里只调 `User_Tasks_Init()`；`vApplicationTickHook` → `TaskTickHook()` 给 LVGL 喂 tick |
| 3 | `User/Tasks/Src/user_TasksInit.c` | **全项目任务地图**：13 个任务的栈大小/优先级/句柄一屏看完；文件末尾还有 `LvHandlerTask`、`WDOGFeedTask` |
| 4 | `User/Tasks/Src/user_HardwareInitTask.c` | 一次性引导任务：RTC/UART/PWM → 电源 → 按键 → 传感器 → EEPROM → 蓝牙 → 触摸/屏 → LVGL/`ui_init()`，完成后 `vTaskDelete(NULL)` 自尽 |

### 第 2 步：任务层——数据怎么流动（7 个文件，约 840 行）

| # | 文件 | 入口函数 / 作用 |
| --- | --- | --- |
| 5 | `user_KeyTask.c` | `KeyTask`：按键扫描 + 消抖，事件送页面 |
| 6 | `user_ScrRenewTask.c` | `ScrRenewTask`：消费按键队列 → `Page_Back()`/`Page_Back_Bottom()`，顺带让传感器休眠；真正的 LVGL 刷新在 `LvHandlerTask`（`user_TasksInit.c` 末尾） |
| 7 | `user_SensUpdateTask.c` | `SensorDataUpdateTask`（首页温湿度/步数、环境页、指南针、气压）、`HRDataUpdateTask`（心率/血氧）、`MPUCheckTask`（抬腕检测） |
| 8 | `user_ChargCheckTask.c` | `ChargPageEnterTask`：等充电中断事件，插入弹充电页、拔出返回 |
| 9 | `user_DataSaveTask.c` | `DataSaveTask`：队列驱动，把计步/设置写 EEPROM（地址表见文件头注释） |
| 10 | `user_MessageSendTask.c` | `MessageSendTask`：等 UART 中断事件，响应 `OV+SEND` 上报时间/温湿度/心率/血氧/步数，`OV+ST=` 对时（经 KT6328 蓝牙透传） |
| 11 | `user_RunModeTasks.c` | `IdleEnterTask` / `StopEnterTask`：空闲计时 → STOP 模式 → RTC/MPU 中断唤醒，**低功耗主线** |

### 第 3 步：架构精华（Func 层，4 个文件约 1000 行）

| # | 文件 | 阅读重点 |
| --- | --- | --- |
| 12 | `User/Func/Src/PageManager.c` | **本项目最精华**：`Page_t` 函数指针 + 页面栈 push/pop/deinit，147 行看懂页面路由 |
| 13 | `User/Func/Src/pubsub.c` | 52 行发布/订阅，传感器 → UI 解耦的关键 |
| 14 | `User/Func/Src/HWDataAccess.c` | BSP 与 UI 的隔离层；`HW_USE_HARDWARE` 开关让同一份 UI 跑在 PC 上 |
| 15 | `User/Func/Src/StrCalculate.c` | 字符串栈/数值转字符串等显示层小工具，无硬件依赖，适合练手改 |

### 第 4 步：GUI 层——先骨架，再一个页面（Screens 下 19 个源文件基本同构，读 3 个即可）

| # | 文件 | 阅读重点 |
| --- | --- | --- |
| 16 | `User/GUI_App/ui.c` | `ui_init()`：建主题 → `Pages_init()` 压入首页 → 建 1s 定时器 |
| 17 | `User/GUI_App/Screens/Src/ui_HomePage.c` | 表盘页：`Page_t` 的 `init/initGUI/show/hide/deinit` 到底长什么样（751 行，可跳读） |
| 18 | `.../ui_MenuPage.c` | 菜单页：页面切换与列表交互 |
| 19 | `.../ui_SetPage.c` 或任选一页 | 设置/游戏/心率页，挑感兴趣的读；其余页面套路完全相同 |
| — | `ui_helpers.c/.h` | SquareLine Studio 生成的公共函数，不必细读 |

### 第 5 步：BSP 驱动——按依赖顺序，不必全读

| # | 文件 | 阅读重点 |
| --- | --- | --- |
| 20 | `BSP/IIC/iic_hal.c` | **先读它**：软件 I2C，所有 I2C 传感器的地基（任何 GPIO 都能当 I2C） |
| 21 | `BSP/LCD/lcd.c` + `lcd_init.c` | ST7789 SPI 屏：初始化序列 + 刷屏/画点函数 |
| 22 | `BSP/MPU6050/mpu6050.c` | 计步/抬腕数据源；`eMPL/` 是 InvenSense DMP 库，先不看 |
| 23 | `BSP/TOUCH/CST816.c` | 电容触摸：坐标读取 + 中断 |
| 24 | `BSP/KEY/key.c` | 按键扫描（`KeyTask` 的下层） |
| 25 | `BSP/POWER/power.c` | 电源使能、电量 ADC、关机 |
| 26 | `BSP/BL24C02/DataSave.c` | EEPROM 数据布局（文件头有地址表），配合第 9 项读 |
| 27 | `BSP/AHT21`、`EM7028`、`SPL06_001`、`KT6328`、`OWDG` | 按需查：温湿度 / 心率 / 气压 / 蓝牙 / 看门狗 |

### 第 6 步（进阶）：IAP / BootLoader

| # | 文件 | 阅读重点 |
| --- | --- | --- |
| 28 | `Software/IAP_F411/Core/Src/main.c` | 上电查 `0x08008000` 处的 `APP FLAG`：合法则跳 `APPLICATION_ADDRESS`，否则留升级菜单 |
| 29 | `IAP_F411/Ymodem/ymodem.c` | Ymodem 协议收包 |
| 30 | `IAP_F411/Ymodem/flash_if.c` | Flash 擦写与 App 分区 |
| 31 | `IAP_F411/Ymodem/menu.c` | 串口升级菜单交互 |

### 速查：已带【中文导读】注释的文件

仓库里 17 个核心文件顶部已加 `【中文导读】` 注释块，按第 1~3 步顺序读它们即可覆盖主干；`grep -rl "【中文导读】" Software` 可列出全部：

- 启动/任务（9）：`user_TasksInit.c`、`user_HardwareInitTask.c`、`user_RunModeTasks.c`、`user_ScrRenewTask.c`、`user_SensUpdateTask.c`、`user_KeyTask.c`、`user_ChargCheckTask.c`、`user_DataSaveTask.c`、`user_MessageSendTask.c`
- 架构（4）：`PageManager.c`、`pubsub.c`、`HWDataAccess.c`、`StrCalculate.c`
- 驱动（4）：`lcd.c`、`mpu6050.c`、`key.c`、`power.c`

## 学习路径（建议）

1. **先跑模拟器**：`lv_sim_vscode_win/` 用 VSCode 打开编译，零硬件先做 UI/页面逻辑。
2. 读 README + 作者官网手册，对照 `Software/OV_Watch` 工程。
3. 核心主线：按上面「工程代码阅读顺序」第 1 → 3 → 5 步走：`User/Tasks/`（各 RTOS 任务）→ `Func/PageManager.c`（页面状态机——**本项目最精华**）→ `BSP/`（逐个外设驱动）。
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

---

## 详细源码文档（`Docs/`，共 10 篇）

本仓库的 `Docs/` 目录是对固件源码的逐文件详解，建议配合本导读的阅读顺序使用：

| 篇号 | 文件 | 内容 |
|---|---|---|
| 00 | `Docs/00-总览与硬件架构.md` | 硬件清单/引脚表、软件分层、Flash 布局、任务总表、数据流、阅读路线 |
| 01 | `Docs/01-启动流程与时钟配置.md` | IAP→APP 启动链、VTOR、100 MHz 时钟树、HAL 时基 TIM1、初始化任务时序 |
| 02 | `Docs/02-FreeRTOS任务系统详解.md` | 14 个任务、优先级映射、16 KB 堆预算、队列/事件、逐任务精读、调度场景 |
| 03 | `Docs/03-页面管理器与GUI架构.md` | Page_t/页面栈、Page_Load/Back、22 个页面清单、LVGL 接入、页面生命周期 |
| 04 | `Docs/04-数据流与发布订阅.md` | HWInterface 接口表、HW_USE_HARDWARE、pubsub 实现与使用范围、数据流 |
| 05 | `Docs/05-中断与通信协议.md` | NVIC 优先级约束、USART DMA+IDLE、命令协议、EXTI、RTC 唤醒 |
| 06 | `Docs/06-外设驱动详解.md` | 软件 I2C、ST7789、CST816、MPU6050、AHT21、EM7028、SPL06、LSM303、EEPROM、按键、电源、看门狗 |
| 07 | `Docs/07-低功耗与运行模式.md` | 空闲计时、STOP 进出全流程、唤醒源、时钟恢复、功耗优化与已知隐患 |
| 08 | `Docs/08-数据存储与IAP升级.md` | EEPROM 地址表、每日步数、Ymodem 升级、APP FLAG、Flash 分区 |
| 09 | `Docs/09-调试构建与实验指南.md` | Keil 构建/烧录、模拟器、调试手段、7 个动手实验、已知问题清单、自测题 |

