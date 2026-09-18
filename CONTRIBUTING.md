# Contributing

欢迎改进兼容性、辅助功能、地图交互与文案。

1. 按 README 安装固定依赖；在 WSL 使用 MinGW-w64 编译界面。
2. 后台 stdout 仅允许匿名 JSON 事件。不要添加原始异常、UDID、设备名或完整命令日志。
3. 不要将「命令已发送」写成「物理位置已验证」；恢复失败必须在界面可见。
4. 修改设备逻辑后运行 `python -m unittest discover -s tests -v`。冻结后运行 `engine/meridian-engine.exe --self-test`。
5. 用 `Meridian.exe --render-preview output.png` 检查界面，末尾加 `1` / `2` 检查指南 / 关于开源页。
6. 用 `scripts/package.ps1` 生成源码与发布包；不要直接压缩整个工作目录。

问题报告请只包含 iOS 版本、机型类别、应用版本、匿名错误码与复现步骤。不要提交配对记录、Apple ID、序列号或个人定位信息。
