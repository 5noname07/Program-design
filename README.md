# 食堂自动化管理系统

对应《1 B250122_课题》课题15。业务核心、文件读写和字符菜单采用C99；可选图形界面采用Qt Widgets与C++，通过`extern "C"`调用C核心。无需数据库、Web服务或第三方业务库。CMake仅用于构建。本次没有编写或修改实习报告。

## 直接运行

本机图形版已经编译并打包，可双击项目根目录的`启动图形版.cmd`，或运行`dist/canteen_gui.exe`。请保留整个`dist`目录及其中的DLL、插件和`data`文件夹。图形版默认使用`dist/data/canteen.txt`，字符版默认使用项目的`data/canteen.txt`；指定同一个绝对路径可共用存档，同一时刻只允许一个界面打开该文件。

在项目目录打开PowerShell，运行：

```powershell
.\canteen.exe
```

管理员演示密码为`admin123`。可在启动前用环境变量更改：

```powershell
$env:CANTEEN_ADMIN_PASSWORD='你自己的密码'
.\canteen.exe
```

学生按卡号登录：1001张三、1002李四、1003王五（已挂失）。当前演示文件包含35张学生卡、35种菜品和40笔消费，余额以实际文件及界面显示为准。所有成功变更立即写入`data/canteen.txt`，重新启动会恢复数据。演示密码是教学用途的简化认证，程序没有网络服务。

可指定独立数据文件，建议演示前复制一份，保留初始数据：

```powershell
Copy-Item data/canteen.txt data/demo.txt
.\canteen.exe data/demo.txt
```

必须从项目目录启动，或传入数据文件的绝对路径。找不到文件或文件损坏会拒绝启动，避免意外覆盖。不要同时打开多个程序操作同一个数据文件。

## 编译与验证

已有MinGW GCC时，无需安装其他依赖：

```powershell
gcc -std=c99 -Wall -Wextra -Wpedantic -Werror -I include src/card.c src/dish.c src/consumption.c src/statistics.c src/storage.c src/validation.c src/console.c src/main.c -o canteen.exe
gcc -std=c99 -Wall -Wextra -Wpedantic -Werror -I include src/card.c src/dish.c src/consumption.c src/statistics.c src/storage.c src/validation.c tests/test_core.c -o core_tests.exe
.\core_tests.exe
```

也可使用CMake：

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
.\build\canteen.exe
```

使用Visual Studio时，采用其CMake工具链，省略`-G "MinGW Makefiles"`。本实现使用C99和现代Qt工具链，未承诺兼容VC++6.0等旧编译器；课题允许选择熟悉的开发平台。

## Qt图形界面

`gui/main.cpp`负责启动，`gui/window.h/.cpp`实现界面。包含身份登录、办卡、充值、挂失解挂、菜单管理与搜索、购买确认、历史流水、销售排行及学生卡使用统计。图形界面的业务和存档格式与字符版共用。表格只读，操作通过按钮及输入框完成。

安装Qt5或Qt6的Widgets开发组件后，在Qt Creator中打开`CMakeLists.txt`，选择与Qt匹配的编译器Kit，并把`BUILD_QT_GUI`设为`ON`。Windows下MinGW版Qt必须配套对应MinGW，MSVC版Qt须用MSVC。

```powershell
cmake -S . -B build-qt -G "MinGW Makefiles" -DBUILD_QT_GUI=ON -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/mingw_64"
cmake --build build-qt
.\build-qt\canteen_gui.exe
```

上面的Qt路径需要改为实际安装位置。本机已安装匹配的MSYS2 Qt6.10，实际路径为`D:/msys64/mingw64`，可将`CMAKE_PREFIX_PATH`设为此值。图形版已实际编译，自动测试覆盖错误密码、挂失拒绝登录、充值、取消购买、购买扣款、流水、销售排行和关闭后重开；Windows原生界面测试与截图检查通过。

```powershell
ctest --test-dir build-qt --output-on-failure
./tools/package_windows.ps1
```

发布脚本复制Qt插件和非Qt运行库，保留发布目录中已有的存档。已在移除开发工具PATH的环境下验证发布目录的运行库完整性。`dist`和编译结果不纳入Git，其他电脑克隆后需要编译和打包；可直接复制本机整个`dist`文件夹运行。

## 源码阅读顺序及三人协作建议

1. `include/canteen_types.h`定义公共数据类型；`card.h/.c`负责学生卡，`dish.h/.c`负责菜单。建议一位成员负责这些业务模块。
2. `consumption.h/.c`负责扣款与流水，`statistics.h/.c`负责排名和消费汇总，`storage.h/.c`负责文件存取，`validation.h/.c`负责输入校验。建议一位成员负责消费、统计和存储。
3. `console.h`与`src/console.c`负责字符交互；`gui/window.h`声明Qt窗口，`gui/window.cpp`实现窗口方法；两个`main`文件仅负责启动。建议一位成员负责界面、测试和集成。

每个模块的`.h`声明接口、参数和返回值，`.c`实现函数。调用方按需包含头文件，所有实现文件分别编译后链接，不通过包含`.c`拼接代码。`include/canteen.h`仅保留为兼容原调用的聚合头文件。统计计算由C模块统一提供，字符版和Qt版只展示结果。
以上只是后续协作建议，不代表三个人已经完成的贡献。课程要求每位成员实际参与编程，后续按真实代码与工作量记录贡献比例。

## 功能与实现说明

| 课题要求 | 实现位置与行为 |
| --- | --- |
| 管理员密码登录 | 两种界面均校验密码；学生看不到管理员操作 |
| 办卡、充值、挂失 | 卡号唯一，正整数编号；挂失卡拒绝登录和消费；可解挂 |
| 菜单增删改查 | 新增禁止重号；修改保持编号；按名称关键字查询 |
| 销售排行 | 销量降序，同销量按编号升序；包含停用菜品 |
| 学生查询余额与购买 | 登录后显示余额；一笔购买一份，余额不足不扣款 |
| 消费历史 | 保存卡号、时间、菜品编号、成交金额及成交菜名 |
| 学生卡使用统计 | 余额、状态、消费笔数与累计消费 |
| 数据文件持久保存 | 单个文本快照同时保存三类数据；临时文件写完后替换原文件 |

金额按整数分计算，输入最多两位小数。流水保留成交时信息，后续改价不影响历史收入。菜品删除为停用，编号不能复用，历史流水仍可查询。程序不提供删除消费记录功能。

为便于课程理解，使用结构体数组与顺序查找：最多500张卡、200个菜品（含停用）、10000笔消费。达到容量拒绝新增，不覆盖旧流水。名称为UTF-8，最多63字节且不允许空格。时间使用本机当地时间，格式`YYYY-MM-DDTHH:MM:SS`。系统限本地单进程教学使用；同一数据文件有进程独占锁，字符版与Qt版不能同时操作同一存档，后启动的实例会提示关闭另一实例；不保证断电时文件系统的物理落盘。

`CANTEEN1`文件头后依次是卡数、菜品数、流水数。随后每张卡一行（卡号 姓名 余额分 状态），每种菜一行（编号 名称 单价分 销量 在售标记），每条流水一行（卡号 菜品编号 成交菜名 金额分 时间）。加载时校验容量、编号重复、关联对象和销量与流水的一致性。损坏文件应从备份恢复，不建议手工修改。

## 后续人工验收步骤

1. 复制初始数据；管理员登录，新增9001学生卡并充值20元。查看保存文件，重启确认仍存在。
2. 学生9001购买红烧肉，检查余额8元、销量比操作前增加1、消费记录新增12元；重启复核。
3. 再买一份红烧肉，确认余额不足，文件和销量不变。
4. 管理员挂失9001，确认拒绝学生登录；解挂后可登录。
5. 将红烧肉改名并调价，确认旧流水仍是原名和12元；删除后确认不可购买且排行保留。
6. 测试重复卡号、重复菜品编号、负数、非数字、三位小数、错误密码及不存在编号。
7. 在另一个文件副本中修改计数或流水关联编号，确认启动报错且原文件不被覆盖。
8. Qt安装完成后，以相同数据副本在图形界面重复以上流程，并检查窗口缩放、输入取消、确认对话框及中文显示。

核心自动测试覆盖金额解析、重复办卡、正常扣款、挂失拒绝、余额不足、历史成交信息、软删除、保存重载、损坏文件隔离和无效保存路径。报告后续应按样稿要求记录操作前后文件、操作过程和截图，现阶段不生成报告正文。

## 老师新增要求核查

- 功能同步：同一进程共用Canteen状态，扣款、销量、流水共同保存，失败恢复原状态；跨进程使用独占锁，避免旧状态覆盖。
- 实际场景：挂失与解挂、余额不足、重复编号、删除后的历史记录、成交价格快照均有处理；购买前展示菜名及金额。
- 控制台优先：可不安装Qt独立编译和测试；三人分工仅为建议，至少两位同学应实际参与核心编程，贡献按真实情况记录。
- 数据规模：data/canteen.txt有35张卡、35种菜、40笔流水，均以txt保存。tools/seed_demo.c可生成新的独立演示文件，禁止覆盖已有文件。
- 电脑时间：purchase每次读取time(NULL)和localtime；不使用固定日期。Qt底部时钟按电脑时间每秒刷新；读档校验真实日期和闰年。
- 稳健性：整数与金额在运算前检查溢出；损坏存档拒绝加载，消费记录容量满时拒绝扣款。

生成全新演示存档：./build/seed_demo.exe data/new_demo.txt。消费记录的时间就是执行该命令时的电脑时间，生成文件不用于伪造过去的消费。
